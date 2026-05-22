#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DroneFleetManager.generated.h"

class ADroneActor;
class APathActor;

UCLASS()
class ZGD_DK_API ADroneFleetManager : public AActor
{
	GENERATED_BODY()

public:
	ADroneFleetManager();

protected:
	virtual void BeginPlay() override;

public:
	// 重新扫描场景中的 DroneActor 和 PathActor
	UFUNCTION(BlueprintCallable, Category = "Fleet")
	void RegisterSceneActors();

	// 根据无人机编号更新对应无人机位置
	UFUNCTION(BlueprintCallable, Category = "Fleet")
	void UpdateDronePositionById(const FString& DroneId, const FVector& NewPosition);

	// 根据无人机编号更新对应路径
	UFUNCTION(BlueprintCallable, Category = "Fleet")
	void UpdatePathPointsById(const FString& DroneId, const TArray<FVector>& PathPoints);

	// 根据无人机编号提交目标点
	UFUNCTION(BlueprintCallable, Category = "Fleet")
	void SubmitTargetPointById(const FString& DroneId, const FVector& TargetWorldPosition);

	// 外部系统返回路径后统一调用该接口
	UFUNCTION(BlueprintCallable, Category = "Fleet")
	void OnPathResponseReceivedById(const FString& DroneId, const TArray<FVector>& NewPathPoints);

	// 根据编号获取无人机 Actor
	UFUNCTION(BlueprintCallable, Category = "Fleet")
	ADroneActor* GetDroneActorById(const FString& DroneId) const;

	// 根据编号获取路径 Actor
	UFUNCTION(BlueprintCallable, Category = "Fleet")
	APathActor* GetPathActorById(const FString& DroneId) const;

	UFUNCTION(BlueprintCallable, Category = "Fleet")
	TArray<FString> GetRegisteredDroneIds() const;

private:
	void HandleMockPathResponse(FString DroneId);
	TArray<FVector> BuildMockPathPoints(const FVector& StartPoint, const FVector& EndPoint) const;

private:
	// 是否在 BeginPlay 时自动扫描场景
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fleet", meta = (AllowPrivateAccess = "true"))
	bool bAutoRegisterOnBeginPlay = true;

	// 是否启用本地 mock 路径响应
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mock Response", meta = (AllowPrivateAccess = "true"))
	bool bEnableMockPathResponse = true;

	// mock 路径响应延迟
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mock Response", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MockResponseDelay = 1.0f;

	// mock 路径巡航高度抬升
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mock Response", meta = (AllowPrivateAccess = "true"))
	float MockCruiseHeightOffset = 300.0f;

	// mock 路径中间点数量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mock Response", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "20"))
	int32 MockIntermediatePointCount = 3;

	// 调试显示
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	bool bDrawDebugTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	float DebugSphereRadius = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	float DebugDisplayTime = 5.0f;

private:
	// DroneId -> DroneActor
	UPROPERTY(Transient)
	TMap<FString, ADroneActor*> DroneActorMap;

	// DroneId -> PathActor
	UPROPERTY(Transient)
	TMap<FString, APathActor*> PathActorMap;

	// DroneId -> 待请求目标点
	TMap<FString, FVector> PendingTargetMap;

	// DroneId -> mock 定时器
	TMap<FString, FTimerHandle> MockResponseTimerMap;
};