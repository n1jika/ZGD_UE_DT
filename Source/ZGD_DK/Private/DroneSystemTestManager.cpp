#include "DroneSystemTestManager.h"

#include "DroneActor.h"
#include "PathActor.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

ADroneSystemTestManager::ADroneSystemTestManager()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

void ADroneSystemTestManager::BeginPlay()
{
    Super::BeginPlay();

    FindDroneActorIfNeeded();
    FindPathActorIfNeeded();

    // ---------- Path test ----------
    if (bEnablePathTest && PathActorRef)
    {
        InitPathTestData();
        SendInitialPath();

        if (bEnableUpdatedPathTest)
        {
            GetWorldTimerManager().SetTimer(
                UpdatedPathTimerHandle,
                this,
                &ADroneSystemTestManager::SendUpdatedPath,
                UpdatedPathDelay,
                false
            );
        }
    }
    else if (bEnablePathTest && !PathActorRef)
    {
        UE_LOG(LogTemp, Warning, TEXT("DroneSystemTestManager: No PathActor found in scene."));

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                5.0f,
                FColor::Red,
                TEXT("DroneSystemTestManager: No PathActor found.")
            );
        }
    }

    // ---------- Drone test ----------
    if (bEnableDronePositionTest && DroneActorRef)
    {
        InitDroneTestPositions();
        StartDronePositionTest();
    }
    else if (bEnableDronePositionTest && !DroneActorRef)
    {
        UE_LOG(LogTemp, Warning, TEXT("DroneSystemTestManager: No DroneActor found in scene."));

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                5.0f,
                FColor::Red,
                TEXT("DroneSystemTestManager: No DroneActor found.")
            );
        }
    }
}

void ADroneSystemTestManager::FindDroneActorIfNeeded()
{
    if (DroneActorRef)
    {
        return;
    }

    AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), ADroneActor::StaticClass());
    DroneActorRef = Cast<ADroneActor>(FoundActor);
}

void ADroneSystemTestManager::FindPathActorIfNeeded()
{
    if (PathActorRef)
    {
        return;
    }

    AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), APathActor::StaticClass());
    PathActorRef = Cast<APathActor>(FoundActor);
}

void ADroneSystemTestManager::InitDroneTestPositions()
{
    DroneTestPositions.Empty();
    CurrentDroneTestIndex = 0;

    const FVector StartPos = DroneActorRef->GetActorLocation();

    DroneTestPositions.Add(StartPos + FVector(0.0f, 0.0f, 300.0f));
    DroneTestPositions.Add(StartPos + FVector(300.0f, 0.0f, 350.0f));
    DroneTestPositions.Add(StartPos + FVector(600.0f, 100.0f, 400.0f));
    DroneTestPositions.Add(StartPos + FVector(900.0f, 250.0f, 450.0f));
    DroneTestPositions.Add(StartPos + FVector(1200.0f, 400.0f, 500.0f));
    DroneTestPositions.Add(StartPos + FVector(1500.0f, 600.0f, 550.0f));
}

void ADroneSystemTestManager::StartDronePositionTest()
{
    if (DroneTestPositions.Num() == 0)
    {
        return;
    }

    SendNextDronePosition();

    GetWorldTimerManager().SetTimer(
        DroneFeedTimerHandle,
        this,
        &ADroneSystemTestManager::SendNextDronePosition,
        DronePositionTestInterval,
        true
    );
}

void ADroneSystemTestManager::SendNextDronePosition()
{
    if (!DroneActorRef)
    {
        GetWorldTimerManager().ClearTimer(DroneFeedTimerHandle);
        return;
    }

    if (CurrentDroneTestIndex >= DroneTestPositions.Num())
    {
        GetWorldTimerManager().ClearTimer(DroneFeedTimerHandle);

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                3.0f,
                FColor::Yellow,
                TEXT("Drone position test finished.")
            );
        }
        return;
    }

    DroneActorRef->UpdateDronePosition(DroneTestPositions[CurrentDroneTestIndex]);
    CurrentDroneTestIndex++;
}

void ADroneSystemTestManager::InitPathTestData()
{
    InitialPathPoints.Empty();
    UpdatedPathPoints.Empty();

    FVector BasePos = GetActorLocation();

    if (DroneActorRef)
    {
        BasePos = DroneActorRef->GetActorLocation();
    }
    else if (PathActorRef)
    {
        BasePos = PathActorRef->GetActorLocation();
    }

    // 初始路径
    InitialPathPoints.Add(BasePos + FVector(0.0f, 0.0f, 250.0f));
    InitialPathPoints.Add(BasePos + FVector(400.0f, 0.0f, 300.0f));
    InitialPathPoints.Add(BasePos + FVector(800.0f, 150.0f, 350.0f));
    InitialPathPoints.Add(BasePos + FVector(1200.0f, 350.0f, 400.0f));
    InitialPathPoints.Add(BasePos + FVector(1600.0f, 650.0f, 450.0f));

    // 更新后的路径（模拟重规划）
    UpdatedPathPoints.Add(BasePos + FVector(0.0f, 0.0f, 250.0f));
    UpdatedPathPoints.Add(BasePos + FVector(300.0f, -200.0f, 320.0f));
    UpdatedPathPoints.Add(BasePos + FVector(700.0f, -350.0f, 380.0f));
    UpdatedPathPoints.Add(BasePos + FVector(1100.0f, -100.0f, 430.0f));
    UpdatedPathPoints.Add(BasePos + FVector(1600.0f, 250.0f, 500.0f));
}

void ADroneSystemTestManager::SendInitialPath()
{
    if (!PathActorRef || InitialPathPoints.Num() < 2)
    {
        return;
    }

    PathActorRef->UpdatePathPoints(InitialPathPoints);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            3.0f,
            FColor::Cyan,
            TEXT("Initial path sent.")
        );
    }
}

void ADroneSystemTestManager::SendUpdatedPath()
{
    if (!PathActorRef || UpdatedPathPoints.Num() < 2)
    {
        return;
    }

    PathActorRef->UpdatePathPoints(UpdatedPathPoints);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            3.0f,
            FColor::Orange,
            TEXT("Updated path sent.")
        );
    }
}