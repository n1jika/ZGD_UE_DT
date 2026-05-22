#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DroneActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class ZGD_DK_API ADroneActor : public AActor
{
    GENERATED_BODY()

public:
    ADroneActor();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone")
    USceneComponent* Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone")
    UStaticMeshComponent* DroneMesh;

    // 无人机唯一编号，用于多无人机场景下区分不同实体
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone")
    FString DroneId = TEXT("UAV_001");

    UFUNCTION(BlueprintCallable, Category = "Drone")
    FString GetDroneId() const { return DroneId; }

    // 实时位置输入接口
    UFUNCTION(BlueprintCallable, Category = "Drone")
    void UpdateDronePosition(const FVector& NewPosition);
};