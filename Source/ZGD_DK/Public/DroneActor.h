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

    // 实时位置输入接口
    UFUNCTION(BlueprintCallable, Category = "Drone")
    void UpdateDronePosition(const FVector& NewPosition);
};