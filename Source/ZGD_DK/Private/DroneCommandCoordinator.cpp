#include "DroneCommandCoordinator.h"

#include "DroneActor.h"
#include "PathActor.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ADroneCommandCoordinator::ADroneCommandCoordinator()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADroneCommandCoordinator::BeginPlay()
{
	Super::BeginPlay();
	FindReferencesIfNeeded();
}

void ADroneCommandCoordinator::FindReferencesIfNeeded()
{
	if (!PathActorRef)
	{
		AActor* FoundPathActor = UGameplayStatics::GetActorOfClass(GetWorld(), APathActor::StaticClass());
		PathActorRef = Cast<APathActor>(FoundPathActor);
	}

	if (!DroneActorRef)
	{
		AActor* FoundDroneActor = UGameplayStatics::GetActorOfClass(GetWorld(), ADroneActor::StaticClass());
		DroneActorRef = Cast<ADroneActor>(FoundDroneActor);
	}
}

void ADroneCommandCoordinator::SubmitTargetPoint(const FVector& TargetWorldPosition)
{
	FindReferencesIfNeeded();

	LastTargetPoint = TargetWorldPosition;
	PendingRequestedTargetPoint = TargetWorldPosition;
	bHasValidTargetPoint = true;
	bWaitingForPathResponse = true;

	UE_LOG(LogTemp, Log, TEXT("DroneCommandCoordinator: Target point submitted: X=%.2f Y=%.2f Z=%.2f"),
		TargetWorldPosition.X, TargetWorldPosition.Y, TargetWorldPosition.Z);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Yellow,
			FString::Printf(
				TEXT("Target submitted: (%.1f, %.1f, %.1f)"),
				TargetWorldPosition.X,
				TargetWorldPosition.Y,
				TargetWorldPosition.Z));
	}

	if (bDrawDebugSelectedTarget)
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

	SendTargetPointToDroneSystem(TargetWorldPosition);
}

void ADroneCommandCoordinator::SendTargetPointToDroneSystem(const FVector& TargetWorldPosition)
{
	UE_LOG(LogTemp, Log, TEXT("DroneCommandCoordinator: Simulating sending target point to drone system..."));

	if (!bEnableMockPathResponse)
	{
		return;
	}

	// 如果已有等待中的模拟响应，先取消，避免连续点击时乱序返回
	if (GetWorldTimerManager().IsTimerActive(MockResponseTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(MockResponseTimerHandle);
	}

	GetWorldTimerManager().SetTimer(
		MockResponseTimerHandle,
		this,
		&ADroneCommandCoordinator::HandleMockPathResponse,
		MockResponseDelay,
		false);
}

void ADroneCommandCoordinator::HandleMockPathResponse()
{
	FindReferencesIfNeeded();

	FVector StartPoint = PendingRequestedTargetPoint;

	if (DroneActorRef)
	{
		StartPoint = DroneActorRef->GetActorLocation();
	}
	else
	{
		// 如果当前场景里没有 DroneActor，就用一个退化起点，至少保证路径能生成
		StartPoint = PendingRequestedTargetPoint + FVector(-500.0f, -300.0f, 0.0f);
	}

	const TArray<FVector> MockPathPoints = BuildMockPathPoints(StartPoint, PendingRequestedTargetPoint);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Green,
			TEXT("Mock path response received."));
	}

	OnPathResponseReceived(MockPathPoints);
}

TArray<FVector> ADroneCommandCoordinator::BuildMockPathPoints(const FVector& StartPoint, const FVector& EndPoint) const
{
	TArray<FVector> Result;

	// 巡航高度：取起点、终点中较高者，再加一个抬高量
	const float CruiseZ = FMath::Max(StartPoint.Z, EndPoint.Z) + MockCruiseHeightOffset;

	// 起点
	Result.Add(StartPoint);

	// 起飞抬升点
	FVector LiftPoint = StartPoint;
	LiftPoint.Z = CruiseZ;
	Result.Add(LiftPoint);

	// 中间点
	const int32 Count = FMath::Max(0, MockIntermediatePointCount);
	for (int32 Index = 1; Index <= Count; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(Count + 1);

		FVector MidPoint = FMath::Lerp(StartPoint, EndPoint, Alpha);
		MidPoint.Z = CruiseZ;

		// 给一点轻微弯曲，避免所有线段过直，看起来更像“规划路径”
		const float SideOffset = (Index % 2 == 0) ? 120.0f : -120.0f;
		const FVector Direction = (EndPoint - StartPoint).GetSafeNormal2D();
		const FVector RightVector(-Direction.Y, Direction.X, 0.0f);

		MidPoint += RightVector * SideOffset;
		Result.Add(MidPoint);
	}

	// 降落前点
	FVector PreTargetPoint = EndPoint;
	PreTargetPoint.Z = CruiseZ;
	Result.Add(PreTargetPoint);

	// 终点
	Result.Add(EndPoint);

	return Result;
}

void ADroneCommandCoordinator::OnPathResponseReceived(const TArray<FVector>& NewPathPoints)
{
	bWaitingForPathResponse = false;

	if (PathActorRef)
	{
		PathActorRef->UpdatePathPoints(NewPathPoints);

		UE_LOG(LogTemp, Log, TEXT("DroneCommandCoordinator: Path response received, %d points updated."), NewPathPoints.Num());

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.0f,
				FColor::Cyan,
				FString::Printf(TEXT("Path updated: %d points"), NewPathPoints.Num()));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DroneCommandCoordinator: PathActorRef is null, cannot update path."));
	}
}