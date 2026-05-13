#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DroneCommandCoordinator.generated.h"

class APathActor;
class ADroneActor;

UCLASS()
class ZGD_DK_API ADroneCommandCoordinator : public AActor
{
	GENERATED_BODY()

public:
	ADroneCommandCoordinator();

	UFUNCTION(BlueprintCallable, Category = "Command")
	void SubmitTargetPoint(const FVector& TargetWorldPosition);

	UFUNCTION(BlueprintCallable, Category = "Command")
	void OnPathResponseReceived(const TArray<FVector>& NewPathPoints);

	UFUNCTION(BlueprintPure, Category = "Command")
	bool HasValidTargetPoint() const { return bHasValidTargetPoint; }

	UFUNCTION(BlueprintPure, Category = "Command")
	FVector GetLastTargetPoint() const { return LastTargetPoint; }

	UFUNCTION(BlueprintPure, Category = "Command")
	bool IsWaitingForPathResponse() const { return bWaitingForPathResponse; }

protected:
	virtual void BeginPlay() override;

private:
	void FindReferencesIfNeeded();

	// 当前阶段：本地模拟“发送给无人机系统”
	void SendTargetPointToDroneSystem(const FVector& TargetWorldPosition);

	// 延时后模拟“无人机系统返回路径”
	void HandleMockPathResponse();

	// 根据当前无人机位置和目标点，构造一条测试路径
	TArray<FVector> BuildMockPathPoints(const FVector& StartPoint, const FVector& EndPoint) const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Refs", meta = (AllowPrivateAccess = "true"))
	APathActor* PathActorRef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Refs", meta = (AllowPrivateAccess = "true"))
	ADroneActor* DroneActorRef = nullptr;

	// 是否启用本地模拟路径响应
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mock Response", meta = (AllowPrivateAccess = "true"))
	bool bEnableMockPathResponse = true;

	// 模拟返回延迟（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mock Response", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MockResponseDelay = 1.0f;

	// 构造路径时额外抬高的巡航高度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mock Response", meta = (AllowPrivateAccess = "true"))
	float MockCruiseHeightOffset = 300.0f;

	// 中间采样点数量（不含起点终点）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mock Response", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "10"))
	int32 MockIntermediatePointCount = 3;

	// 调试显示
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	bool bDrawDebugSelectedTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float DebugSphereRadius = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float DebugDisplayTime = 5.0f;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bHasValidTargetPoint = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bWaitingForPathResponse = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	FVector LastTargetPoint = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	FVector PendingRequestedTargetPoint = FVector::ZeroVector;

	FTimerHandle MockResponseTimerHandle;
};