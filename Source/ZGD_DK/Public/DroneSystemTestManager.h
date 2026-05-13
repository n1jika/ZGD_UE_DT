#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "DroneSystemTestManager.generated.h"

class USceneComponent;
class ADroneActor;
class APathActor;

UCLASS()
class ZGD_DK_API ADroneSystemTestManager : public AActor
{
    GENERATED_BODY()

public:
    ADroneSystemTestManager();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test")
    USceneComponent* Root;

    // ---------- Drone ----------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Refs")
    ADroneActor* DroneActorRef = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Drone")
    bool bEnableDronePositionTest = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Drone", meta = (ClampMin = "0.01"))
    float DronePositionTestInterval = 1.0f;

    // ---------- Path ----------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Refs")
    APathActor* PathActorRef = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Path")
    bool bEnablePathTest = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Path")
    bool bEnableUpdatedPathTest = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Path", meta = (ClampMin = "0.1"))
    float UpdatedPathDelay = 5.0f;

private:
    // Drone test data
    TArray<FVector> DroneTestPositions;
    int32 CurrentDroneTestIndex = 0;
    FTimerHandle DroneFeedTimerHandle;

    // Path test data
    TArray<FVector> InitialPathPoints;
    TArray<FVector> UpdatedPathPoints;
    FTimerHandle UpdatedPathTimerHandle;

    // Drone helpers
    void FindDroneActorIfNeeded();
    void InitDroneTestPositions();
    void StartDronePositionTest();
    void SendNextDronePosition();

    // Path helpers
    void FindPathActorIfNeeded();
    void InitPathTestData();
    void SendInitialPath();
    void SendUpdatedPath();
};