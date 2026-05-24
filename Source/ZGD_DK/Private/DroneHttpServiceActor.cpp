#include "DroneHttpServiceActor.h"

#include "DroneFleetManager.h"

#include "Async/Async.h"
#include "Containers/StringConv.h"
#include "HttpServerModule.h"
#include "IHttpRouter.h"
#include "Json.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

ADroneHttpServiceActor::ADroneHttpServiceActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADroneHttpServiceActor::BeginPlay()
{
	Super::BeginPlay();

	FindFleetManagerIfNeeded();

	if (bAutoStart)
	{
		StartHttpService();
	}
}

void ADroneHttpServiceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopHttpService();

	Super::EndPlay(EndPlayReason);
}

void ADroneHttpServiceActor::FindFleetManagerIfNeeded()
{
	if (FleetManagerRef)
	{
		return;
	}

	AActor* FoundActor = UGameplayStatics::GetActorOfClass(
		GetWorld(),
		ADroneFleetManager::StaticClass());

	FleetManagerRef = Cast<ADroneFleetManager>(FoundActor);
}

void ADroneHttpServiceActor::StartHttpService()
{
	if (bServiceStarted)
	{
		return;
	}

	Router = FHttpServerModule::Get().GetHttpRouter(ListenPort);

	if (!Router.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("DroneHttpServiceActor: Failed to get HTTP router on port %d."), ListenPort);
		return;
	}

	RegisterRoutes();

	FHttpServerModule::Get().StartAllListeners();

	bServiceStarted = true;

	UE_LOG(LogTemp, Log, TEXT("DroneHttpServiceActor: HTTP service started on port %d."), ListenPort);
}

void ADroneHttpServiceActor::StopHttpService()
{
	if (!bServiceStarted)
	{
		return;
	}

	if (Router.IsValid())
	{
		for (const FHttpRouteHandle& Handle : RouteHandles)
		{
			if (Handle.IsValid())
			{
				Router->UnbindRoute(Handle);
			}
		}
	}

	RouteHandles.Empty();

	FHttpServerModule::Get().StopAllListeners();

	bServiceStarted = false;

	UE_LOG(LogTemp, Log, TEXT("DroneHttpServiceActor: HTTP service stopped."));
}

void ADroneHttpServiceActor::RegisterRoutes()
{
	if (!Router.IsValid())
	{
		return;
	}

	RouteHandles.Add(Router->BindRoute(
		FHttpPath(TEXT("/api/v1/health")),
		EHttpServerRequestVerbs::VERB_GET,
		[this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			return HandleHealth(Request, OnComplete);
		}));

	RouteHandles.Add(Router->BindRoute(
		FHttpPath(TEXT("/api/v1/drones")),
		EHttpServerRequestVerbs::VERB_GET,
		[this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			return HandleGetDrones(Request, OnComplete);
		}));

	RouteHandles.Add(Router->BindRoute(
		FHttpPath(TEXT("/api/v1/drone/position")),
		EHttpServerRequestVerbs::VERB_POST,
		[this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			return HandleUpdateDronePosition(Request, OnComplete);
		}));

	RouteHandles.Add(Router->BindRoute(
		FHttpPath(TEXT("/api/v1/drone/positions")),
		EHttpServerRequestVerbs::VERB_POST,
		[this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			return HandleBatchUpdateDronePositions(Request, OnComplete);
		}));

	RouteHandles.Add(Router->BindRoute(
		FHttpPath(TEXT("/api/v1/drone/path")),
		EHttpServerRequestVerbs::VERB_POST,
		[this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			return HandleUpdateDronePath(Request, OnComplete);
		}));
}

bool ADroneHttpServiceActor::HandleHealth(
	const FHttpServerRequest& Request,
	const FHttpResultCallback& OnComplete)
{
	TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();

	ResponseObject->SetNumberField(TEXT("code"), 0);
	ResponseObject->SetStringField(TEXT("message"), TEXT("ok"));
	ResponseObject->SetStringField(TEXT("service"), TEXT("ZGD_UE_DT"));
	ResponseObject->SetStringField(TEXT("version"), TEXT("1.0"));

	OnComplete(MakeJsonResponseFromObject(ResponseObject));
	return true;
}

bool ADroneHttpServiceActor::HandleGetDrones(
	const FHttpServerRequest& Request,
	const FHttpResultCallback& OnComplete)
{
	FHttpResultCallback Complete = OnComplete;

	AsyncTask(ENamedThreads::GameThread, [this, Complete]()
		{
			FindFleetManagerIfNeeded();

			if (!FleetManagerRef)
			{
				Complete(MakeJsonResponse(1001, TEXT("DroneFleetManager not found")));
				return;
			}

			FleetManagerRef->RegisterSceneActors();

			const TArray<FString> DroneIds = FleetManagerRef->GetRegisteredDroneIds();

			TArray<TSharedPtr<FJsonValue>> DroneArray;

			for (const FString& DroneId : DroneIds)
			{
				DroneArray.Add(MakeShared<FJsonValueString>(DroneId));
			}

			TSharedPtr<FJsonObject> DataObject = MakeShared<FJsonObject>();
			DataObject->SetArrayField(TEXT("drones"), DroneArray);

			TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
			ResponseObject->SetNumberField(TEXT("code"), 0);
			ResponseObject->SetStringField(TEXT("message"), TEXT("ok"));
			ResponseObject->SetObjectField(TEXT("data"), DataObject);

			Complete(MakeJsonResponseFromObject(ResponseObject));
		});

	return true;
}

bool ADroneHttpServiceActor::HandleUpdateDronePosition(
	const FHttpServerRequest& Request,
	const FHttpResultCallback& OnComplete)
{
	TSharedPtr<FJsonObject> JsonObject;

	if (!ParseRequestJson(Request, JsonObject))
	{
		OnComplete(MakeJsonResponse(4001, TEXT("invalid json"), EHttpServerResponseCodes::BadRequest));
		return true;
	}

	FString DroneId;

	if (!JsonObject->TryGetStringField(TEXT("drone_id"), DroneId) || DroneId.IsEmpty())
	{
		OnComplete(MakeJsonResponse(4002, TEXT("missing drone_id"), EHttpServerResponseCodes::BadRequest));
		return true;
	}

	FVector Position;

	if (!TryGetVectorField(JsonObject, TEXT("position"), Position))
	{
		OnComplete(MakeJsonResponse(4003, TEXT("missing or invalid position"), EHttpServerResponseCodes::BadRequest));
		return true;
	}

	FHttpResultCallback Complete = OnComplete;

	AsyncTask(ENamedThreads::GameThread, [this, Complete, DroneId, Position]()
		{
			FindFleetManagerIfNeeded();

			if (!FleetManagerRef)
			{
				Complete(MakeJsonResponse(1001, TEXT("DroneFleetManager not found")));
				return;
			}

			FleetManagerRef->UpdateDronePositionById(DroneId, Position);

			TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
			ResponseObject->SetNumberField(TEXT("code"), 0);
			ResponseObject->SetStringField(TEXT("message"), TEXT("position updated"));
			ResponseObject->SetStringField(TEXT("drone_id"), DroneId);

			Complete(MakeJsonResponseFromObject(ResponseObject));
		});

	return true;
}

bool ADroneHttpServiceActor::HandleBatchUpdateDronePositions(
	const FHttpServerRequest& Request,
	const FHttpResultCallback& OnComplete)
{
	TSharedPtr<FJsonObject> JsonObject;

	if (!ParseRequestJson(Request, JsonObject))
	{
		OnComplete(MakeJsonResponse(4001, TEXT("invalid json"), EHttpServerResponseCodes::BadRequest));
		return true;
	}

	const TArray<TSharedPtr<FJsonValue>>* ItemsPtr = nullptr;

	if (!JsonObject->TryGetArrayField(TEXT("items"), ItemsPtr))
	{
		OnComplete(MakeJsonResponse(4004, TEXT("missing items array"), EHttpServerResponseCodes::BadRequest));
		return true;
	}

	struct FParsedPositionItem
	{
		FString DroneId;
		FVector Position;
	};

	TArray<FParsedPositionItem> ParsedItems;

	for (const TSharedPtr<FJsonValue>& ItemValue : *ItemsPtr)
	{
		if (!ItemValue.IsValid())
		{
			continue;
		}

		TSharedPtr<FJsonObject> ItemObject = ItemValue->AsObject();

		if (!ItemObject.IsValid())
		{
			continue;
		}

		FString DroneId;
		FVector Position;

		if (ItemObject->TryGetStringField(TEXT("drone_id"), DroneId)
			&& !DroneId.IsEmpty()
			&& TryGetVectorField(ItemObject, TEXT("position"), Position))
		{
			FParsedPositionItem ParsedItem;
			ParsedItem.DroneId = DroneId;
			ParsedItem.Position = Position;
			ParsedItems.Add(ParsedItem);
		}
	}

	if (ParsedItems.Num() == 0)
	{
		OnComplete(MakeJsonResponse(4005, TEXT("no valid position items"), EHttpServerResponseCodes::BadRequest));
		return true;
	}

	FHttpResultCallback Complete = OnComplete;

	AsyncTask(ENamedThreads::GameThread, [this, Complete, ParsedItems]()
		{
			FindFleetManagerIfNeeded();

			if (!FleetManagerRef)
			{
				Complete(MakeJsonResponse(1001, TEXT("DroneFleetManager not found")));
				return;
			}

			for (const FParsedPositionItem& Item : ParsedItems)
			{
				FleetManagerRef->UpdateDronePositionById(Item.DroneId, Item.Position);
			}

			TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
			ResponseObject->SetNumberField(TEXT("code"), 0);
			ResponseObject->SetStringField(TEXT("message"), TEXT("positions updated"));
			ResponseObject->SetNumberField(TEXT("updated_count"), ParsedItems.Num());

			Complete(MakeJsonResponseFromObject(ResponseObject));
		});

	return true;
}

bool ADroneHttpServiceActor::HandleUpdateDronePath(
	const FHttpServerRequest& Request,
	const FHttpResultCallback& OnComplete)
{
	TSharedPtr<FJsonObject> JsonObject;

	if (!ParseRequestJson(Request, JsonObject))
	{
		OnComplete(MakeJsonResponse(4001, TEXT("invalid json"), EHttpServerResponseCodes::BadRequest));
		return true;
	}

	FString DroneId;

	if (!JsonObject->TryGetStringField(TEXT("drone_id"), DroneId) || DroneId.IsEmpty())
	{
		OnComplete(MakeJsonResponse(4002, TEXT("missing drone_id"), EHttpServerResponseCodes::BadRequest));
		return true;
	}

	const TArray<TSharedPtr<FJsonValue>>* PathArrayPtr = nullptr;

	if (!JsonObject->TryGetArrayField(TEXT("path"), PathArrayPtr))
	{
		OnComplete(MakeJsonResponse(4006, TEXT("missing path array"), EHttpServerResponseCodes::BadRequest));
		return true;
	}

	TArray<FVector> PathPoints;

	for (const TSharedPtr<FJsonValue>& PointValue : *PathArrayPtr)
	{
		if (!PointValue.IsValid())
		{
			continue;
		}

		TSharedPtr<FJsonObject> PointObject = PointValue->AsObject();

		if (!PointObject.IsValid())
		{
			continue;
		}

		FVector Point;

		if (TryParseVectorObject(PointObject, Point))
		{
			PathPoints.Add(Point);
		}
	}

	if (PathPoints.Num() < 2)
	{
		OnComplete(MakeJsonResponse(4007, TEXT("path requires at least 2 valid points"), EHttpServerResponseCodes::BadRequest));
		return true;
	}

	FHttpResultCallback Complete = OnComplete;

	AsyncTask(ENamedThreads::GameThread, [this, Complete, DroneId, PathPoints]()
		{
			FindFleetManagerIfNeeded();

			if (!FleetManagerRef)
			{
				Complete(MakeJsonResponse(1001, TEXT("DroneFleetManager not found")));
				return;
			}

			FleetManagerRef->UpdatePathPointsById(DroneId, PathPoints);

			TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
			ResponseObject->SetNumberField(TEXT("code"), 0);
			ResponseObject->SetStringField(TEXT("message"), TEXT("path updated"));
			ResponseObject->SetStringField(TEXT("drone_id"), DroneId);
			ResponseObject->SetNumberField(TEXT("point_count"), PathPoints.Num());

			Complete(MakeJsonResponseFromObject(ResponseObject));
		});

	return true;
}

FString ADroneHttpServiceActor::RequestBodyToString(const FHttpServerRequest& Request)
{
	if (Request.Body.Num() == 0)
	{
		return FString();
	}

	const ANSICHAR* BodyData = reinterpret_cast<const ANSICHAR*>(Request.Body.GetData());
	const int32 BodySize = Request.Body.Num();

	FUTF8ToTCHAR Converter(BodyData, BodySize);
	FString BodyString(Converter.Length(), Converter.Get());

	UE_LOG(LogTemp, Log, TEXT("DroneHttpServiceActor: Request body = %s"), *BodyString);

	return BodyString;
}

bool ADroneHttpServiceActor::ParseRequestJson(
	const FHttpServerRequest& Request,
	TSharedPtr<FJsonObject>& OutJsonObject)
{
	const FString BodyString = RequestBodyToString(Request);

	if (BodyString.IsEmpty())
	{
		return false;
	}

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(BodyString);

	return FJsonSerializer::Deserialize(Reader, OutJsonObject) && OutJsonObject.IsValid();
}

bool ADroneHttpServiceActor::TryGetVectorField(
	const TSharedPtr<FJsonObject>& JsonObject,
	const FString& FieldName,
	FVector& OutVector)
{
	if (!JsonObject.IsValid())
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* VectorObjectPtr = nullptr;

	if (!JsonObject->TryGetObjectField(FieldName, VectorObjectPtr))
	{
		return false;
	}

	if (!VectorObjectPtr || !VectorObjectPtr->IsValid())
	{
		return false;
	}

	return TryParseVectorObject(*VectorObjectPtr, OutVector);
}

bool ADroneHttpServiceActor::TryParseVectorObject(
	const TSharedPtr<FJsonObject>& JsonObject,
	FVector& OutVector)
{
	if (!JsonObject.IsValid())
	{
		return false;
	}

	double X = 0.0;
	double Y = 0.0;
	double Z = 0.0;

	if (!JsonObject->TryGetNumberField(TEXT("x"), X))
	{
		return false;
	}

	if (!JsonObject->TryGetNumberField(TEXT("y"), Y))
	{
		return false;
	}

	if (!JsonObject->TryGetNumberField(TEXT("z"), Z))
	{
		return false;
	}

	OutVector = FVector(
		static_cast<float>(X),
		static_cast<float>(Y),
		static_cast<float>(Z));

	return true;
}

TUniquePtr<FHttpServerResponse> ADroneHttpServiceActor::MakeJsonResponse(
	int32 Code,
	const FString& Message,
	EHttpServerResponseCodes ResponseCode)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();

	JsonObject->SetNumberField(TEXT("code"), Code);
	JsonObject->SetStringField(TEXT("message"), Message);

	return MakeJsonResponseFromObject(JsonObject, ResponseCode);
}

TUniquePtr<FHttpServerResponse> ADroneHttpServiceActor::MakeJsonResponseFromObject(
	const TSharedPtr<FJsonObject>& JsonObject,
	EHttpServerResponseCodes ResponseCode)
{
	FString OutputString;

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(
		OutputString,
		TEXT("application/json"));

	Response->Code = ResponseCode;

	return Response;
}