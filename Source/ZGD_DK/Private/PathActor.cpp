#include "PathActor.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

APathActor::APathActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PathSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PathSpline"));
	PathSpline->SetupAttachment(SceneRoot);
	PathSpline->SetClosedLoop(false);

	ArrowInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ArrowInstances"));
	ArrowInstances->SetupAttachment(SceneRoot);
	ArrowInstances->SetMobility(EComponentMobility::Movable);
	ArrowInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArrowInstances->SetCastShadow(false);

	// 默认主路径 Mesh：Cube
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		PathMesh = CubeMeshFinder.Object;
		CenterLineMesh = CubeMeshFinder.Object;
	}

	// 默认箭头 Mesh：Plane（薄片箭头）
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMeshFinder.Succeeded())
	{
		ArrowMesh = PlaneMeshFinder.Object;
	}
}

void APathActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshPath();
}

void APathActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshPath();
}

void APathActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShowDirectionArrows && bAnimateDirectionArrows && PathSpline && PathSpline->GetNumberOfSplinePoints() >= 2)
	{
		const float SafeSpacing = FMath::Max(ArrowSpacing, 50.0f);
		ArrowMoveOffset = FMath::Fmod(ArrowMoveOffset + DeltaTime * ArrowMoveSpeed, SafeSpacing);
		RefreshDirectionArrows();
	}
}

void APathActor::UpdatePathPoints(const TArray<FVector>& InPathPoints)
{
	PathPoints = InPathPoints;
	RefreshPath();
}

void APathActor::ClearPath()
{
	PathPoints.Empty();
	ArrowMoveOffset = 0.0f;
	RefreshPath();
}

void APathActor::RefreshPath()
{
	RebuildSpline();
	RebuildPathMeshes();
	RebuildCenterLineMeshes();
	RefreshPathMaterials();
	RefreshDirectionArrows();
}

void APathActor::RebuildSpline()
{
	if (!PathSpline)
	{
		return;
	}

	PathSpline->ClearSplinePoints(false);

	if (PathPoints.Num() <= 0)
	{
		PathSpline->UpdateSpline();
		return;
	}

	// 按世界坐标设置路径点
	PathSpline->SetSplinePoints(PathPoints, ESplineCoordinateSpace::World, false);

	// 把每个点都设成 Linear，这样 spline 本身就是折线
	const int32 NumPoints = PathSpline->GetNumberOfSplinePoints();
	for (int32 Index = 0; Index < NumPoints; ++Index)
	{
		PathSpline->SetSplinePointType(Index, ESplinePointType::Linear, false);
	}

	PathSpline->UpdateSpline();
}

void APathActor::ClearSplineMeshArray(TArray<TObjectPtr<USplineMeshComponent>>& MeshArray)
{
	for (USplineMeshComponent* MeshComp : MeshArray)
	{
		if (MeshComp)
		{
			MeshComp->DestroyComponent();
		}
	}
	MeshArray.Empty();
}

USplineMeshComponent* APathActor::CreateSplineMeshComponent(const FName& BaseName)
{
	USplineMeshComponent* MeshComp = NewObject<USplineMeshComponent>(this, BaseName);
	if (!MeshComp)
	{
		return nullptr;
	}

	MeshComp->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	MeshComp->SetMobility(EComponentMobility::Movable);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetCastShadow(false);
	MeshComp->SetForwardAxis(ESplineMeshAxis::X);
	MeshComp->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepRelativeTransform);
	MeshComp->RegisterComponent();

	return MeshComp;
}

void APathActor::RebuildPathMeshes()
{
	ClearSplineMeshArray(PathMeshComponents);

	if (!bShowPath || !PathSpline || PathSpline->GetNumberOfSplinePoints() < 2 || !PathMesh)
	{
		return;
	}

	const int32 SegmentCount = PathSpline->GetNumberOfSplinePoints() - 1;

	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		USplineMeshComponent* MeshComp = CreateSplineMeshComponent(
			*FString::Printf(TEXT("PathMesh_%d"), Index));

		if (!MeshComp)
		{
			continue;
		}

		MeshComp->SetStaticMesh(PathMesh);

		FVector StartPos = PathSpline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::Local);
		FVector EndPos = PathSpline->GetLocationAtSplinePoint(Index + 1, ESplineCoordinateSpace::Local);

		StartPos.Z += PathHeightOffset;
		EndPos.Z += PathHeightOffset;

		// 直接按当前线段方向生成切线，不再用缩放后的 spline tangent
		const FVector SegmentVector = EndPos - StartPos;
		const FVector SegmentTangent = SegmentVector;

		MeshComp->SetStartAndEnd(StartPos, SegmentTangent, EndPos, SegmentTangent, true);

		const float WidthScale = PathWidth / 100.0f;
		const float ThicknessScale = PathThickness / 100.0f;
		MeshComp->SetStartScale(FVector2D(WidthScale, ThicknessScale), true);
		MeshComp->SetEndScale(FVector2D(WidthScale, ThicknessScale), true);

		PathMeshComponents.Add(MeshComp);
	}
}

void APathActor::RebuildCenterLineMeshes()
{
	ClearSplineMeshArray(CenterLineMeshComponents);

	if (!bShowCenterLine || !PathSpline || PathSpline->GetNumberOfSplinePoints() < 2 || !CenterLineMesh)
	{
		return;
	}

	const int32 SegmentCount = PathSpline->GetNumberOfSplinePoints() - 1;

	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		USplineMeshComponent* MeshComp = CreateSplineMeshComponent(
			*FString::Printf(TEXT("CenterLineMesh_%d"), Index));

		if (!MeshComp)
		{
			continue;
		}

		MeshComp->SetStaticMesh(CenterLineMesh);

		FVector StartPos = PathSpline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::Local);
		FVector EndPos = PathSpline->GetLocationAtSplinePoint(Index + 1, ESplineCoordinateSpace::Local);

		StartPos.Z += CenterLineHeightOffset;
		EndPos.Z += CenterLineHeightOffset;

		// 关键：中轴线也严格跟当前线段方向走
		const FVector SegmentVector = EndPos - StartPos;
		const FVector SegmentTangent = SegmentVector;

		MeshComp->SetStartAndEnd(StartPos, SegmentTangent, EndPos, SegmentTangent, true);

		const float WidthScale = CenterLineWidth / 100.0f;
		const float ThicknessScale = CenterLineThickness / 100.0f;
		MeshComp->SetStartScale(FVector2D(WidthScale, ThicknessScale), true);
		MeshComp->SetEndScale(FVector2D(WidthScale, ThicknessScale), true);

		CenterLineMeshComponents.Add(MeshComp);
	}
}

void APathActor::RefreshPathMaterials()
{
	for (USplineMeshComponent* MeshComp : PathMeshComponents)
	{
		if (MeshComp && PathMaterial)
		{
			const int32 MaterialSlotCount = FMath::Max(1, MeshComp->GetNumMaterials());
			for (int32 SlotIndex = 0; SlotIndex < MaterialSlotCount; ++SlotIndex)
			{
				MeshComp->SetMaterial(SlotIndex, PathMaterial);
			}
			MeshComp->MarkRenderStateDirty();
		}
	}

	for (USplineMeshComponent* MeshComp : CenterLineMeshComponents)
	{
		if (MeshComp)
		{
			UMaterialInterface* EffectiveCenterLineMaterial = CenterLineMaterial ? CenterLineMaterial : PathMaterial;
			if (EffectiveCenterLineMaterial)
			{
				const int32 MaterialSlotCount = FMath::Max(1, MeshComp->GetNumMaterials());
				for (int32 SlotIndex = 0; SlotIndex < MaterialSlotCount; ++SlotIndex)
				{
					MeshComp->SetMaterial(SlotIndex, EffectiveCenterLineMaterial);
				}
				MeshComp->MarkRenderStateDirty();
			}
		}
	}

	if (ArrowInstances)
	{
		if (ArrowMesh)
		{
			ArrowInstances->SetStaticMesh(ArrowMesh);
		}

		if (ArrowMaterial)
		{
			if (!ArrowMID || ArrowMID->Parent != ArrowMaterial)
			{
				ArrowMID = UMaterialInstanceDynamic::Create(ArrowMaterial, this);
			}

			const int32 MaterialSlotCount = FMath::Max(1, ArrowInstances->GetStaticMesh() ? ArrowInstances->GetStaticMesh()->GetStaticMaterials().Num() : 1);
			for (int32 SlotIndex = 0; SlotIndex < MaterialSlotCount; ++SlotIndex)
			{
				ArrowInstances->SetMaterial(SlotIndex, ArrowMID);
			}
		}

		ArrowInstances->MarkRenderStateDirty();
	}
}

void APathActor::RefreshDirectionArrows()
{
	if (!ArrowInstances)
	{
		return;
	}

	ArrowInstances->ClearInstances();

	if (!bShowDirectionArrows)
	{
		return;
	}

	if (!ArrowMesh)
	{
		return;
	}

	if (!PathSpline || PathSpline->GetNumberOfSplinePoints() < 2)
	{
		return;
	}

	ArrowInstances->SetStaticMesh(ArrowMesh);

	if (ArrowMaterial)
	{
		const int32 MaterialSlotCount =
			FMath::Max(1, ArrowInstances->GetStaticMesh() ? ArrowInstances->GetStaticMesh()->GetStaticMaterials().Num() : 1);

		for (int32 SlotIndex = 0; SlotIndex < MaterialSlotCount; ++SlotIndex)
		{
			ArrowInstances->SetMaterial(SlotIndex, ArrowMaterial);
		}
	}

	const float SplineLength = PathSpline->GetSplineLength();
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		ArrowInstances->MarkRenderStateDirty();
		return;
	}

	const float SafeSpacing = FMath::Max(ArrowSpacing, 50.0f);

	// 关键：箭头基准高度直接跟中轴线走
	const float FinalHeightOffset = CenterLineHeightOffset + ArrowHeightOffset;

	// 读取箭头网格本地 bounds，用于自动把几何中心压回中轴线
	const FBoxSphereBounds MeshBounds = ArrowMesh->GetBounds();
	const FVector LocalBoundsOrigin = MeshBounds.Origin;

	for (float Distance = ArrowMoveOffset; Distance < SplineLength; Distance += SafeSpacing)
	{
		const FVector WorldLocation =
			PathSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);

		const FVector Tangent =
			PathSpline->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World).GetSafeNormal();

		if (Tangent.IsNearlyZero())
		{
			continue;
		}

		const FRotator BaseRotation = Tangent.Rotation();
		const FRotator FinalRotation = BaseRotation + ArrowRotationOffset;

		const FVector RightVector = FRotationMatrix(BaseRotation).GetUnitAxis(EAxis::Y);

		FVector FinalLocation = WorldLocation;

		// 先按中轴线前后/左右做微调
		FinalLocation += Tangent * ArrowForwardOffset;
		FinalLocation += RightVector * ArrowLateralOffset;

		// 再放到中轴线高度
		FinalLocation.Z += FinalHeightOffset;

		// 自动把“网格几何中心”对齐到中轴线
		if (bAutoCenterArrowByMeshBounds)
		{
			const FVector ScaledLocalBoundsOrigin(
				LocalBoundsOrigin.X * ArrowScale.X,
				LocalBoundsOrigin.Y * ArrowScale.Y,
				LocalBoundsOrigin.Z * ArrowScale.Z);

			const FVector BoundsOffsetWorld = FinalRotation.RotateVector(ScaledLocalBoundsOrigin);

			// 注意这里是减去，因为要把几何中心拉回到中轴线上
			FinalLocation -= BoundsOffsetWorld;
		}

		const FTransform InstanceTransform(FinalRotation, FinalLocation, ArrowScale);
		ArrowInstances->AddInstanceWorldSpace(InstanceTransform);
	}

	ArrowInstances->MarkRenderStateDirty();
}