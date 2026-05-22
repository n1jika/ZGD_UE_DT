#include "DroneFleetTestManager.h"

#include "DroneActor.h"
#include "DroneFleetManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

ADroneFleetTestManager::ADroneFleetTestManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADroneFleetTestManager::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(
		StartDelayTimerHandle,
		this,
		&ADroneFleetTestManager::StartFleetTest,
		StartDelay,
		false);
}

void ADroneFleetTestManager::FindFleetManagerIfNeeded()
{
	if (FleetManagerRef)
	{
		return;
	}

	AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), ADroneFleetManager::StaticClass());
	FleetManagerRef = Cast<ADroneFleetManager>(FoundActor);
}

void ADroneFleetTestManager::StartFleetTest()
{
	FindFleetManagerIfNeeded();

	if (!FleetManagerRef)
	{
		UE_LOG(LogTemp, Warning, TEXT("DroneFleetTestManager: FleetManagerRef is null."));

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.0f,
				FColor::Red,
				TEXT("Fleet test failed: DroneFleetManager not found."));
		}

		return;
	}

	FleetManagerRef->RegisterSceneActors();

	InitDroneIds();
	InitPositionTestData();
	InitPathTestData();

	if (bEnableFleetPathTest)
	{
		SendInitialFleetPaths();

		if (bEnableUpdatedFleetPathTest)
		{
			GetWorldTimerManager().SetTimer(
				UpdatedPathTimerHandle,
				this,
				&ADroneFleetTestManager::SendUpdatedFleetPaths,
				UpdatedPathDelay,
				false);
		}
	}

	if (bEnableFleetPositionTest)
	{
		SendNextFleetPositions();

		GetWorldTimerManager().SetTimer(
			PositionUpdateTimerHandle,
			this,
			&ADroneFleetTestManager::SendNextFleetPositions,
			PositionUpdateInterval,
			true);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Green,
			TEXT("Fleet test started: 3 drones and 3 paths."));
	}
}

void ADroneFleetTestManager::InitDroneIds()
{
	DroneIds.Empty();

	DroneIds.Add(TEXT("UAV_001"));
	DroneIds.Add(TEXT("UAV_002"));
	DroneIds.Add(TEXT("UAV_003"));
}

void ADroneFleetTestManager::InitPositionTestData()
{
	DronePositionTestData.Empty();
	CurrentPositionIndexMap.Empty();

	if (!FleetManagerRef)
	{
		return;
	}

	// 相对运动步长，单位是 UE 默认单位 cm
	const float Step = 250.0f;

	// 三架无人机分别朝三个不同方向运动
	const FVector DirUAV001 = FVector(1.0f, 0.0f, 0.0f);      // 向 +X 方向
	const FVector DirUAV002 = FVector(0.0f, 1.0f, 0.0f);      // 向 +Y 方向
	const FVector DirUAV003 = FVector(-1.0f, -1.0f, 0.0f).GetSafeNormal(); // 向 -X -Y 斜向

	TMap<FString, FVector> DirectionMap;
	DirectionMap.Add(TEXT("UAV_001"), DirUAV001);
	DirectionMap.Add(TEXT("UAV_002"), DirUAV002);
	DirectionMap.Add(TEXT("UAV_003"), DirUAV003);

	for (const FString& DroneId : DroneIds)
	{
		ADroneActor* DroneActor = FleetManagerRef->GetDroneActorById(DroneId);
		if (!DroneActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("DroneFleetTestManager: DroneActor not found for %s."), *DroneId);
			continue;
		}

		const FVector StartLocation = DroneActor->GetActorLocation();
		const FVector* MoveDirection = DirectionMap.Find(DroneId);

		if (!MoveDirection)
		{
			continue;
		}

		TArray<FVector> Positions;

		// 从当前所在位置开始，沿各自方向做相对运动
		Positions.Add(StartLocation);
		Positions.Add(StartLocation + (*MoveDirection) * Step * 1.0f);
		Positions.Add(StartLocation + (*MoveDirection) * Step * 2.0f);
		Positions.Add(StartLocation + (*MoveDirection) * Step * 3.0f);
		Positions.Add(StartLocation + (*MoveDirection) * Step * 4.0f);

		DronePositionTestData.Add(DroneId, Positions);
		CurrentPositionIndexMap.Add(DroneId, 0);
	}
}

void ADroneFleetTestManager::InitPathTestData()
{
	InitialPathData.Empty();
	UpdatedPathData.Empty();

	if (!FleetManagerRef)
	{
		return;
	}

	const float Step = 250.0f;
	const float PathHeightOffset = 80.0f;

	const FVector DirUAV001 = FVector(1.0f, 0.0f, 0.0f);
	const FVector DirUAV002 = FVector(0.0f, 1.0f, 0.0f);
	const FVector DirUAV003 = FVector(-1.0f, -1.0f, 0.0f).GetSafeNormal();

	TMap<FString, FVector> DirectionMap;
	DirectionMap.Add(TEXT("UAV_001"), DirUAV001);
	DirectionMap.Add(TEXT("UAV_002"), DirUAV002);
	DirectionMap.Add(TEXT("UAV_003"), DirUAV003);

	for (const FString& DroneId : DroneIds)
	{
		ADroneActor* DroneActor = FleetManagerRef->GetDroneActorById(DroneId);
		if (!DroneActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("DroneFleetTestManager: Cannot build path, DroneActor not found for %s."), *DroneId);
			continue;
		}

		const FVector StartLocation = DroneActor->GetActorLocation();
		const FVector* MoveDirection = DirectionMap.Find(DroneId);

		if (!MoveDirection)
		{
			continue;
		}

		// 路径稍微抬高一点，避免和地面或模型重叠
		const FVector BasePoint = StartLocation + FVector(0.0f, 0.0f, PathHeightOffset);

		TArray<FVector> InitialPath;
		InitialPath.Add(BasePoint);
		InitialPath.Add(BasePoint + (*MoveDirection) * Step * 1.0f);
		InitialPath.Add(BasePoint + (*MoveDirection) * Step * 2.0f);
		InitialPath.Add(BasePoint + (*MoveDirection) * Step * 3.0f);
		InitialPath.Add(BasePoint + (*MoveDirection) * Step * 4.0f);

		InitialPathData.Add(DroneId, InitialPath);

		// 更新路径从初始路径末端继续向前延伸一点，方便观察路径更新效果
		const FVector UpdatedStart = BasePoint + (*MoveDirection) * Step * 4.0f;

		TArray<FVector> UpdatedPath;
		UpdatedPath.Add(UpdatedStart);
		UpdatedPath.Add(UpdatedStart + (*MoveDirection) * Step * 1.0f + FVector(0.0f, 0.0f, 40.0f));
		UpdatedPath.Add(UpdatedStart + (*MoveDirection) * Step * 2.0f + FVector(0.0f, 0.0f, 80.0f));
		UpdatedPath.Add(UpdatedStart + (*MoveDirection) * Step * 3.0f + FVector(0.0f, 0.0f, 120.0f));

		UpdatedPathData.Add(DroneId, UpdatedPath);
	}
}

void ADroneFleetTestManager::SendNextFleetPositions()
{
	if (!FleetManagerRef)
	{
		return;
	}

	for (const FString& DroneId : DroneIds)
	{
		TArray<FVector>* Positions = DronePositionTestData.Find(DroneId);
		int32* CurrentIndex = CurrentPositionIndexMap.Find(DroneId);

		if (!Positions || !CurrentIndex || Positions->Num() == 0)
		{
			continue;
		}

		const FVector NewPosition = (*Positions)[*CurrentIndex];

		FleetManagerRef->UpdateDronePositionById(DroneId, NewPosition);

		*CurrentIndex = (*CurrentIndex + 1) % Positions->Num();
	}
}

void ADroneFleetTestManager::SendInitialFleetPaths()
{
	if (!FleetManagerRef)
	{
		return;
	}

	for (const FString& DroneId : DroneIds)
	{
		TArray<FVector>* PathPoints = InitialPathData.Find(DroneId);

		if (PathPoints && PathPoints->Num() >= 2)
		{
			FleetManagerRef->UpdatePathPointsById(DroneId, *PathPoints);
		}
	}
}

void ADroneFleetTestManager::SendUpdatedFleetPaths()
{
	if (!FleetManagerRef)
	{
		return;
	}

	for (const FString& DroneId : DroneIds)
	{
		TArray<FVector>* PathPoints = UpdatedPathData.Find(DroneId);

		if (PathPoints && PathPoints->Num() >= 2)
		{
			FleetManagerRef->UpdatePathPointsById(DroneId, *PathPoints);
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			4.0f,
			FColor::Yellow,
			TEXT("Fleet paths updated for UAV_001 / UAV_002 / UAV_003."));
	}
}