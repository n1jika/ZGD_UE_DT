#include "DronePlayerController.h"

#include "DroneCommandCoordinator.h"
#include "TargetSelectionWidget.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

ADronePlayerController::ADronePlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void ADronePlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	FindCoordinatorIfNeeded();
	CreateSelectionWidgetIfNeeded();

	PendingTargetZ = FMath::Clamp(PendingTargetZ, MinTargetZ, MaxTargetZ);
	UpdateSelectionWidgetState();
}

void ADronePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ADronePlayerController::HandleLeftMouseClick);
	}
}

void ADronePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdatePreviewTargetPoint();
}

void ADronePlayerController::FindCoordinatorIfNeeded()
{
	if (CoordinatorRef)
	{
		return;
	}

	AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), ADroneCommandCoordinator::StaticClass());
	CoordinatorRef = Cast<ADroneCommandCoordinator>(FoundActor);
}

bool ADronePlayerController::GetMouseIntersectionWithHorizontalPlane(float PlaneZ, FVector& OutWorldPoint) const
{
	FVector WorldOrigin;
	FVector WorldDirection;

	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return false;
	}

	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	const float T = (PlaneZ - WorldOrigin.Z) / WorldDirection.Z;
	if (T < 0.0f)
	{
		return false;
	}

	OutWorldPoint = WorldOrigin + T * WorldDirection;
	return true;
}

bool ADronePlayerController::GetBaseSelectionPoint(FVector& OutBasePoint)
{
	// 方案 1：优先使用鼠标命中的真实表面位置
	if (bPreferSurfaceHitForXY)
	{
		FHitResult HitResult;
		const bool bHit = GetHitResultUnderCursorByChannel(
			UEngineTypes::ConvertToTraceType(ECC_Visibility),
			bTraceComplexOnClick,
			HitResult);

		if (bHit)
		{
			OutBasePoint = HitResult.ImpactPoint;
			OutBasePoint.Z = BaseSelectionPlaneZ;
			return true;
		}
	}

	// 方案 2：如果没有命中任何表面，再退回水平平面求交
	return GetMouseIntersectionWithHorizontalPlane(BaseSelectionPlaneZ, OutBasePoint);
}

bool ADronePlayerController::GetCurrentViewIntersectionWithPlane(float PlaneZ, FVector& OutWorldPoint) const
{
	FVector CameraLocation;
	FRotator CameraRotation;
	GetPlayerViewPoint(CameraLocation, CameraRotation);

	const FVector ViewDirection = CameraRotation.Vector();

	if (FMath::IsNearlyZero(ViewDirection.Z))
	{
		return false;
	}

	const float T = (PlaneZ - CameraLocation.Z) / ViewDirection.Z;
	if (T < 0.0f)
	{
		return false;
	}

	OutWorldPoint = CameraLocation + T * ViewDirection;
	return true;
}

void ADronePlayerController::CreateSelectionWidgetIfNeeded()
{
	if (TargetSelectionWidget)
	{
		return;
	}

	TargetSelectionWidget = CreateWidget<UTargetSelectionWidget>(this, UTargetSelectionWidget::StaticClass());
	if (!TargetSelectionWidget)
	{
		return;
	}

	TargetSelectionWidget->AddToViewport(100);

	TargetSelectionWidget->OnStartTargetSelectionRequested.AddDynamic(this, &ADronePlayerController::HandleStartTargetSelectionRequested);
	TargetSelectionWidget->OnConfirmTargetSelectionRequested.AddDynamic(this, &ADronePlayerController::HandleConfirmTargetSelectionRequested);
	TargetSelectionWidget->OnCancelTargetSelectionRequested.AddDynamic(this, &ADronePlayerController::HandleCancelTargetSelectionRequested);
	TargetSelectionWidget->OnTargetHeightNormalizedChanged.AddDynamic(this, &ADronePlayerController::HandleTargetHeightNormalizedChanged);
}

void ADronePlayerController::EnterTargetSelectionMode()
{
	if (bIsInTargetSelectionMode)
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		StoredPawnLocation = ControlledPawn->GetActorLocation();
		bHasStoredView = true;
	}
	else
	{
		bHasStoredView = false;
	}

	StoredControlRotation = GetControlRotation();

	FVector FocusPoint = FVector::ZeroVector;
	if (!GetCurrentViewIntersectionWithPlane(BaseSelectionPlaneZ, FocusPoint))
	{
		if (ControlledPawn)
		{
			FocusPoint = ControlledPawn->GetActorLocation();
			FocusPoint.Z = BaseSelectionPlaneZ;
		}
	}

	if (ControlledPawn)
	{
		ControlledPawn->SetActorLocation(FVector(FocusPoint.X, FocusPoint.Y, BaseSelectionPlaneZ + TopDownCameraHeight));
	}

	// 完全俯视
	const float YawToKeep = StoredControlRotation.Yaw;
	SetControlRotation(FRotator(-89.9f, YawToKeep, 0.0f));

	bIsInTargetSelectionMode = true;
	bHasSelectedBasePoint = false;
	PendingBasePoint = FVector::ZeroVector;
	PendingTargetZ = FMath::Clamp(PendingTargetZ, MinTargetZ, MaxTargetZ);

	UpdateSelectionWidgetState();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Yellow,
			TEXT("Selection mode entered: left click in top view to choose XY position."));
	}
}

void ADronePlayerController::ExitTargetSelectionMode(bool bRestoreView)
{
	bIsInTargetSelectionMode = false;
	bHasSelectedBasePoint = false;
	PendingBasePoint = FVector::ZeroVector;

	if (bRestoreView && bHasStoredView)
	{
		if (APawn* ControlledPawn = GetPawn())
		{
			ControlledPawn->SetActorLocation(StoredPawnLocation);
		}
		SetControlRotation(StoredControlRotation);
	}

	UpdateSelectionWidgetState();
}

void ADronePlayerController::UpdateSelectionWidgetState()
{
	if (!TargetSelectionWidget)
	{
		return;
	}

	TargetSelectionWidget->SetSelectionMode(bIsInTargetSelectionMode, bHasSelectedBasePoint);
	TargetSelectionWidget->SetHeightNormalized(WorldZToHeightNormalized(PendingTargetZ));
	TargetSelectionWidget->SetHeightValue(PendingTargetZ);

	if (!bIsInTargetSelectionMode)
	{
		TargetSelectionWidget->SetHintMessage(TEXT("Click Start"));
	}
	else if (!bHasSelectedBasePoint)
	{
		TargetSelectionWidget->SetHintMessage(TEXT("Please select an XY position first."));
	}
	else
	{
		TargetSelectionWidget->SetHintMessage(TEXT("Selected XY position"));
	}
}

void ADronePlayerController::UpdatePreviewTargetPoint()
{
	if (!bDrawSelectionDebug || !bIsInTargetSelectionMode || !bHasSelectedBasePoint)
	{
		return;
	}

	const FVector FinalPreviewPoint(PendingBasePoint.X, PendingBasePoint.Y, PendingTargetZ);

	DrawDebugSphere(
		GetWorld(),
		FinalPreviewPoint,
		30.0f,
		16,
		FColor::Red,
		false,
		DebugDisplayTime,
		0,
		2.5f);

	DrawDebugLine(
		GetWorld(),
		FVector(PendingBasePoint.X, PendingBasePoint.Y, BaseSelectionPlaneZ),
		FinalPreviewPoint,
		FColor::Cyan,
		false,
		DebugDisplayTime,
		0,
		2.0f);

	DrawDebugSphere(
		GetWorld(),
		FVector(PendingBasePoint.X, PendingBasePoint.Y, BaseSelectionPlaneZ),
		20.0f,
		12,
		FColor::Yellow,
		false,
		DebugDisplayTime,
		0,
		1.8f);
}

float ADronePlayerController::HeightNormalizedToWorldZ(float InValue) const
{
	return FMath::Lerp(MinTargetZ, MaxTargetZ, FMath::Clamp(InValue, 0.0f, 1.0f));
}

float ADronePlayerController::WorldZToHeightNormalized(float InWorldZ) const
{
	if (FMath::IsNearlyEqual(MinTargetZ, MaxTargetZ))
	{
		return 0.0f;
	}

	return (FMath::Clamp(InWorldZ, MinTargetZ, MaxTargetZ) - MinTargetZ) / (MaxTargetZ - MinTargetZ);
}

void ADronePlayerController::HandleStartTargetSelectionRequested()
{
	EnterTargetSelectionMode();
}

void ADronePlayerController::HandleConfirmTargetSelectionRequested()
{
	if (!bIsInTargetSelectionMode || !bHasSelectedBasePoint)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Please select an XY position first."));
		}
		return;
	}

	FindCoordinatorIfNeeded();

	if (!CoordinatorRef)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("DroneCommandCoordinator not found."));
		}
		return;
	}

	const FVector FinalTargetPoint(PendingBasePoint.X, PendingBasePoint.Y, PendingTargetZ);
	CoordinatorRef->SubmitTargetPoint(FinalTargetPoint);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Cyan,
			FString::Printf(
				TEXT("Target confirmed: (%.1f, %.1f, %.1f)"),
				FinalTargetPoint.X,
				FinalTargetPoint.Y,
				FinalTargetPoint.Z));
	}

	ExitTargetSelectionMode(true);
}

void ADronePlayerController::HandleCancelTargetSelectionRequested()
{
	if (!bIsInTargetSelectionMode)
	{
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Target selection canceled."));
	}

	ExitTargetSelectionMode(true);
}

void ADronePlayerController::HandleTargetHeightNormalizedChanged(float NewValue)
{
	PendingTargetZ = HeightNormalizedToWorldZ(NewValue);
	UpdateSelectionWidgetState();
}

void ADronePlayerController::HandleLeftMouseClick()
{
	if (!bIsInTargetSelectionMode)
	{
		return;
	}

	FVector BasePoint;
	if (!GetBaseSelectionPoint(BasePoint))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Failed to select XY position."));
		}
		return;
	}

	PendingBasePoint = BasePoint;
	bHasSelectedBasePoint = true;

	UpdateSelectionWidgetState();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.5f,
			FColor::Yellow,
			FString::Printf(
				TEXT("XY position selected:(%.1f, %.1f, %.1f)"),
				PendingBasePoint.X,
				PendingBasePoint.Y,
				PendingBasePoint.Z));
	}
}