#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DroneFleetTestManager.generated.h"

class ADroneFleetManager;

UCLASS()
class ZGD_DK_API ADroneFleetTestManager : public AActor
{
	GENERATED_BODY()

public:
	ADroneFleetTestManager();

protected:
	virtual void BeginPlay() override;

private:
	void FindFleetManagerIfNeeded();
	void StartFleetTest();

	void InitDroneIds();
	void InitPositionTestData();
	void InitPathTestData();

	void SendNextFleetPositions();
	void SendInitialFleetPaths();
	void SendUpdatedFleetPaths();

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Refs", meta = (AllowPrivateAccess = "true"))
	ADroneFleetManager* FleetManagerRef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Switch", meta = (AllowPrivateAccess = "true"))
	bool bEnableFleetPositionTest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Switch", meta = (AllowPrivateAccess = "true"))
	bool bEnableFleetPathTest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Switch", meta = (AllowPrivateAccess = "true"))
	bool bEnableUpdatedFleetPathTest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Timing", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float PositionUpdateInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Timing", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float UpdatedPathDelay = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Timing", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float StartDelay = 0.3f;

private:
	TArray<FString> DroneIds;

	TMap<FString, TArray<FVector>> DronePositionTestData;
	TMap<FString, int32> CurrentPositionIndexMap;

	TMap<FString, TArray<FVector>> InitialPathData;
	TMap<FString, TArray<FVector>> UpdatedPathData;

	FTimerHandle StartDelayTimerHandle;
	FTimerHandle PositionUpdateTimerHandle;
	FTimerHandle UpdatedPathTimerHandle;
};