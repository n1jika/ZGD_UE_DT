#include "DroneActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"

ADroneActor::ADroneActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    DroneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneMesh"));
    DroneMesh->SetupAttachment(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    if (CubeMesh.Succeeded())
    {
        DroneMesh->SetStaticMesh(CubeMesh.Object);
        DroneMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.2f));
    }
}

void ADroneActor::BeginPlay()
{
    Super::BeginPlay();
}

void ADroneActor::UpdateDronePosition(const FVector& NewPosition)
{
    SetActorLocation(NewPosition);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            1.5f,
            FColor::Green,
            FString::Printf(TEXT("Drone Position: %s"), *NewPosition.ToString())
        );
    }
}