#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DronePlayerController.generated.h"

class ADroneCommandCoordinator;
class ADroneFleetManager;
class UTargetSelectionWidget;

UCLASS()
class ZGD_DK_API ADronePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADronePlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	void HandleLeftMouseClick();

	void FindCoordinatorIfNeeded();
	void FindFleetManagerIfNeeded();

	bool GetMouseIntersectionWithHorizontalPlane(float PlaneZ, FVector& OutWorldPoint) const;
	bool GetCurrentViewIntersectionWithPlane(float PlaneZ, FVector& OutWorldPoint) const;
	bool GetBaseSelectionPoint(FVector& OutBasePoint);

	void CreateSelectionWidgetIfNeeded();
	void EnterTargetSelectionMode();
	void ExitTargetSelectionMode(bool bRestoreView);

	void RefreshDroneSelectionOptions();
	void UpdateSelectionWidgetState();
	void UpdatePreviewTargetPoint();

	float HeightNormalizedToWorldZ(float InValue) const;
	float WorldZToHeightNormalized(float InWorldZ) const;

	UFUNCTION()
	void HandleStartTargetSelectionRequested();

	UFUNCTION()
	void HandleConfirmTargetSelectionRequested();

	UFUNCTION()
	void HandleCancelTargetSelectionRequested();

	UFUNCTION()
	void HandleTargetHeightNormalizedChanged(float NewValue);

	UFUNCTION()
	void HandleSelectedDroneChanged(FString DroneId);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (AllowPrivateAccess = "true"))
	bool bTraceComplexOnClick = false;

	UPROPERTY()
	ADroneCommandCoordinator* CoordinatorRef = nullptr;

	UPROPERTY()
	ADroneFleetManager* FleetManagerRef = nullptr;

	UPROPERTY()
	UTargetSelectionWidget* TargetSelectionWidget = nullptr;

	// 当前 UI 中选择的无人机编号
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (AllowPrivateAccess = "true"))
	FString SelectedDroneId = TEXT("UAV_001");

	// 选点模式是否开启
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection", meta = (AllowPrivateAccess = "true"))
	bool bIsInTargetSelectionMode = false;

	// 是否已经选定了平面位置（X/Y）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection", meta = (AllowPrivateAccess = "true"))
	bool bHasSelectedBasePoint = false;

	// 第一步选定的平面点
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection", meta = (AllowPrivateAccess = "true"))
	FVector PendingBasePoint = FVector::ZeroVector;

	// 当前滑块对应的目标高度
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection", meta = (AllowPrivateAccess = "true"))
	float PendingTargetZ = 300.0f;

	// 第一步选点参考平面（决定 X/Y）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (AllowPrivateAccess = "true"))
	float BaseSelectionPlaneZ = 0.0f;

	// 可调高度范围
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (AllowPrivateAccess = "true"))
	float MinTargetZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (AllowPrivateAccess = "true"))
	float MaxTargetZ = 3000.0f;

	// 进入选点模式后相机抬高到多少
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (AllowPrivateAccess = "true"))
	float TopDownCameraHeight = 4000.0f;

	// 调试显示
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	bool bDrawSelectionDebug = true;

	// 0.0f 表示每帧刷新显示，适合实时预览红色小球
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	float DebugDisplayTime = 0.0f;

	// 恢复视角所需
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection", meta = (AllowPrivateAccess = "true"))
	bool bHasStoredView = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection", meta = (AllowPrivateAccess = "true"))
	FVector StoredPawnLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection", meta = (AllowPrivateAccess = "true"))
	FRotator StoredControlRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (AllowPrivateAccess = "true"))
	bool bPreferSurfaceHitForXY = true;
};