#include "DroneFleetManager.h"

#include "DroneActor.h"
#include "PathActor.h"

#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ADroneFleetManager::ADroneFleetManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADroneFleetManager::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoRegisterOnBeginPlay)
	{
		RegisterSceneActors();
	}
}

void ADroneFleetManager::RegisterSceneActors()
{
	DroneActorMap.Empty();
	PathActorMap.Empty();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> FoundDroneActors;
	UGameplayStatics::GetAllActorsOfClass(World, ADroneActor::StaticClass(), FoundDroneActors);

	for (AActor* Actor : FoundDroneActors)
	{
		ADroneActor* DroneActor = Cast<ADroneActor>(Actor);
		if (!DroneActor)
		{
			continue;
		}

		const FString DroneId = DroneActor->GetDroneId();

		if (DroneId.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("DroneFleetManager: Found DroneActor with empty DroneId."));
			continue;
		}

		if (DroneActorMap.Contains(DroneId))
		{
			UE_LOG(LogTemp, Warning, TEXT("DroneFleetManager: Duplicate DroneId found: %s"), *DroneId);
		}

		DroneActorMap.Add(DroneId, DroneActor);
	}

	TArray<AActor*> FoundPathActors;
	UGameplayStatics::GetAllActorsOfClass(World, APathActor::StaticClass(), FoundPathActors);

	for (AActor* Actor : FoundPathActors)
	{
		APathActor* PathActor = Cast<APathActor>(Actor);
		if (!PathActor)
		{
			continue;
		}

		const FString OwnerDroneId = PathActor->GetOwnerDroneId();

		if (OwnerDroneId.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("DroneFleetManager: Found PathActor with empty OwnerDroneId."));
			continue;
		}

		if (PathActorMap.Contains(OwnerDroneId))
		{
			UE_LOG(LogTemp, Warning, TEXT("DroneFleetManager: Duplicate PathActor for DroneId: %s"), *OwnerDroneId);
		}

		PathActorMap.Add(OwnerDroneId, PathActor);
	}

	UE_LOG(LogTemp, Log, TEXT("DroneFleetManager: Registered %d drones and %d paths."),
		DroneActorMap.Num(),
		PathActorMap.Num());

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Cyan,
			FString::Printf(TEXT("Fleet registered: %d drones, %d paths"), DroneActorMap.Num(), PathActorMap.Num()));
	}
}

ADroneActor* ADroneFleetManager::GetDroneActorById(const FString& DroneId) const
{
	ADroneActor* const* FoundDrone = DroneActorMap.Find(DroneId);
	return FoundDrone ? *FoundDrone : nullptr;
}

APathActor* ADroneFleetManager::GetPathActorById(const FString& DroneId) const
{
	APathActor* const* FoundPath = PathActorMap.Find(DroneId);
	return FoundPath ? *FoundPath : nullptr;
}

void ADroneFleetManager::UpdateDronePositionById(const FString& DroneId, const FVector& NewPosition)
{
	ADroneActor* DroneActor = GetDroneActorById(DroneId);

	if (!DroneActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("DroneFleetManager: DroneActor not found for DroneId: %s"), *DroneId);
		return;
	}

	DroneActor->UpdateDronePosition(NewPosition);

	UE_LOG(LogTemp, Verbose, TEXT("DroneFleetManager: Updated drone %s position to (%.1f, %.1f, %.1f)."),
		*DroneId,
		NewPosition.X,
		NewPosition.Y,
		NewPosition.Z);
}

void ADroneFleetManager::UpdatePathPointsById(const FString& DroneId, const TArray<FVector>& PathPoints)
{
	if (PathPoints.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("DroneFleetManager: Path for %s has less than 2 points."), *DroneId);
		return;
	}

	APathActor* PathActor = GetPathActorById(DroneId);

	if (!PathActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("DroneFleetManager: PathActor not found for DroneId: %s"), *DroneId);
		return;
	}

	PathActor->UpdatePathPoints(PathPoints);

	UE_LOG(LogTemp, Log, TEXT("DroneFleetManager: Updated path for %s with %d points."),
		*DroneId,
		PathPoints.Num());
}

void ADroneFleetManager::SubmitTargetPointById(const FString& DroneId, const FVector& TargetWorldPosition)
{
	if (DroneId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("DroneFleetManager: SubmitTargetPointById failed because DroneId is empty."));
		return;
	}

	PendingTargetMap.Add(DroneId, TargetWorldPosition);

	if (bDrawDebugTarget)
	{
		DrawDebugSphere(
			GetWorld(),
			TargetWorldPosition,
			DebugSphereRadius,
			16,
			FColor::Red,
			false,
			DebugDisplayTime,
			0,
			2.0f);
	}

	UE_LOG(LogTemp, Log, TEXT("DroneFleetManager: Target submitted for %s: (%.1f, %.1f, %.1f)."),
		*DroneId,
		TargetWorldPosition.X,
		TargetWorldPosition.Y,
		TargetWorldPosition.Z);

	if (!bEnableMockPathResponse)
	{
		// 后续真实联调时，可以在这里发送 DroneId + TargetWorldPosition 给真实无人机系统
		return;
	}

	FTimerHandle& TimerHandle = MockResponseTimerMap.FindOrAdd(DroneId);

	if (GetWorldTimerManager().IsTimerActive(TimerHandle))
	{
		GetWorldTimerManager().ClearTimer(TimerHandle);
	}

	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &ADroneFleetManager::HandleMockPathResponse, DroneId);

	GetWorldTimerManager().SetTimer(
		TimerHandle,
		TimerDelegate,
		MockResponseDelay,
		false);
}

void ADroneFleetManager::HandleMockPathResponse(FString DroneId)
{
	FVector* TargetPointPtr = PendingTargetMap.Find(DroneId);
	if (!TargetPointPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("DroneFleetManager: No pending target for %s."), *DroneId);
		return;
	}

	const FVector TargetPoint = *TargetPointPtr;

	ADroneActor* DroneActor = GetDroneActorById(DroneId);

	FVector StartPoint = TargetPoint + FVector(-500.0f, -300.0f, 0.0f);
	if (DroneActor)
	{
		StartPoint = DroneActor->GetActorLocation();
	}

	const TArray<FVector> MockPath = BuildMockPathPoints(StartPoint, TargetPoint);

	OnPathResponseReceivedById(DroneId, MockPath);
}

TArray<FVector> ADroneFleetManager::BuildMockPathPoints(const FVector& StartPoint, const FVector& EndPoint) const
{
	TArray<FVector> Result;

	const float CruiseZ = FMath::Max(StartPoint.Z, EndPoint.Z) + MockCruiseHeightOffset;

	Result.Add(StartPoint);

	FVector LiftPoint = StartPoint;
	LiftPoint.Z = CruiseZ;
	Result.Add(LiftPoint);

	const int32 Count = FMath::Max(0, MockIntermediatePointCount);

	for (int32 Index = 1; Index <= Count; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(Count + 1);

		FVector MidPoint = FMath::Lerp(StartPoint, EndPoint, Alpha);
		MidPoint.Z = CruiseZ;

		const FVector Direction = (EndPoint - StartPoint).GetSafeNormal2D();
		const FVector RightVector(-Direction.Y, Direction.X, 0.0f);

		const float SideOffset = (Index % 2 == 0) ? 120.0f : -120.0f;
		MidPoint += RightVector * SideOffset;

		Result.Add(MidPoint);
	}

	FVector PreTargetPoint = EndPoint;
	PreTargetPoint.Z = CruiseZ;
	Result.Add(PreTargetPoint);

	Result.Add(EndPoint);

	return Result;
}

void ADroneFleetManager::OnPathResponseReceivedById(const FString& DroneId, const TArray<FVector>& NewPathPoints)
{
	if (NewPathPoints.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("DroneFleetManager: Received invalid path for %s."), *DroneId);
		return;
	}

	UpdatePathPointsById(DroneId, NewPathPoints);

	PendingTargetMap.Remove(DroneId);

	UE_LOG(LogTemp, Log, TEXT("DroneFleetManager: Path response handled for %s."), *DroneId);
}

TArray<FString> ADroneFleetManager::GetRegisteredDroneIds() const
{
	TArray<FString> Result;
	DroneActorMap.GetKeys(Result);
	Result.Sort();
	return Result;
}