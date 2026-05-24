#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "HttpPath.h"
#include "HttpResultCallback.h"
#include "HttpRouteHandle.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"

#include "DroneHttpServiceActor.generated.h"

class ADroneFleetManager;
class IHttpRouter;

UCLASS()
class ZGD_DK_API ADroneHttpServiceActor : public AActor
{
	GENERATED_BODY()

public:
	ADroneHttpServiceActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Drone HTTP Service")
	void StartHttpService();

	UFUNCTION(BlueprintCallable, Category = "Drone HTTP Service")
	void StopHttpService();

private:
	void FindFleetManagerIfNeeded();
	void RegisterRoutes();

	bool HandleHealth(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleGetDrones(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleUpdateDronePosition(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleBatchUpdateDronePositions(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleUpdateDronePath(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);

private:
	static FString RequestBodyToString(const FHttpServerRequest& Request);

	static bool ParseRequestJson(
		const FHttpServerRequest& Request,
		TSharedPtr<FJsonObject>& OutJsonObject);

	static bool TryGetVectorField(
		const TSharedPtr<FJsonObject>& JsonObject,
		const FString& FieldName,
		FVector& OutVector);

	static bool TryParseVectorObject(
		const TSharedPtr<FJsonObject>& JsonObject,
		FVector& OutVector);

	static TUniquePtr<FHttpServerResponse> MakeJsonResponse(
		int32 Code,
		const FString& Message,
		EHttpServerResponseCodes ResponseCode = EHttpServerResponseCodes::Ok);

	static TUniquePtr<FHttpServerResponse> MakeJsonResponseFromObject(
		const TSharedPtr<FJsonObject>& JsonObject,
		EHttpServerResponseCodes ResponseCode = EHttpServerResponseCodes::Ok);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone HTTP Service", meta = (AllowPrivateAccess = "true"))
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone HTTP Service", meta = (AllowPrivateAccess = "true", ClampMin = "1", ClampMax = "65535"))
	int32 ListenPort = 18080;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone HTTP Service", meta = (AllowPrivateAccess = "true"))
	ADroneFleetManager* FleetManagerRef = nullptr;

private:
	TSharedPtr<IHttpRouter> Router;
	TArray<FHttpRouteHandle> RouteHandles;

	bool bServiceStarted = false;
};