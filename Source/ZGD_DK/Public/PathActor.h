#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PathActor.generated.h"

class USceneComponent;
class USplineComponent;
class USplineMeshComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

UCLASS()
class ZGD_DK_API APathActor : public AActor
{
	GENERATED_BODY()

public:
	APathActor();

	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// 外部调用：更新整条路径点
	UFUNCTION(BlueprintCallable, Category = "Path")
	void UpdatePathPoints(const TArray<FVector>& InPathPoints);

	// 清空路径
	UFUNCTION(BlueprintCallable, Category = "Path")
	void ClearPath();

	// 手动刷新
	UFUNCTION(BlueprintCallable, Category = "Path")
	void RefreshPath();

protected:
	virtual void BeginPlay() override;

private:
	void RebuildSpline();
	void RebuildPathMeshes();
	void RebuildCenterLineMeshes();
	void RefreshPathMaterials();
	void RefreshDirectionArrows();

	void ClearSplineMeshArray(TArray<TObjectPtr<USplineMeshComponent>>& MeshArray);
	USplineMeshComponent* CreateSplineMeshComponent(const FName& BaseName);

private:
	// =========================
	// 基础组件
	// =========================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> PathSpline;

	// 动态箭头实例组件（薄片箭头）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> ArrowInstances;

	// =========================
	// 路径点
	// =========================
public:
	// 注意：这里按“世界坐标”理解
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Path")
	TArray<FVector> PathPoints;

	// =========================
	// 主路径可视化
	// =========================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Path")
	bool bShowPath = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Path")
	TObjectPtr<UStaticMesh> PathMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Path")
	TObjectPtr<UMaterialInterface> PathMaterial;

	// 路径宽度（横向）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Path", meta = (ClampMin = "1.0"))
	float PathWidth = 12.0f;

	// 路径厚度（竖向）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Path", meta = (ClampMin = "1.0"))
	float PathThickness = 12.0f;

	// 路径抬高高度
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Path")
	float PathHeightOffset = 12.0f;

	// 切线缩放，控制弯曲外观
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Path", meta = (ClampMin = "0.01"))
	float TangentScale = 0.2f;

	// =========================
	// 中轴线可视化
	// =========================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|CenterLine")
	bool bShowCenterLine = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|CenterLine")
	TObjectPtr<UStaticMesh> CenterLineMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|CenterLine")
	TObjectPtr<UMaterialInterface> CenterLineMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|CenterLine", meta = (ClampMin = "1.0"))
	float CenterLineWidth = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|CenterLine", meta = (ClampMin = "1.0"))
	float CenterLineThickness = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|CenterLine")
	float CenterLineHeightOffset = 15.0f;

	// =========================
	// 薄片箭头
	// =========================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arrows")
	bool bShowDirectionArrows = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arrows")
	bool bAnimateDirectionArrows = true;

	// 默认会自动给你用 Plane
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arrows")
	TObjectPtr<UStaticMesh> ArrowMesh;

	// 建议你给一个“箭头样式”的材质
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arrows")
	TObjectPtr<UMaterialInterface> ArrowMaterial;

	// 箭头间距
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arrows", meta = (ClampMin = "50.0"))
	float ArrowSpacing = 250.0f;

	// 箭头沿中轴线的前后偏移
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arrows")
	float ArrowForwardOffset = 0.0f;

	// 箭头横向偏移
	// 设为 0 就严格在中轴线上
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arrows")
	float ArrowLateralOffset = 0.0f;

	// 箭头额外抬高
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arrows")
	float ArrowHeightOffset = 3.0f;

	// 薄片箭头缩放
	// Plane 默认是一个平面，所以这里更像“长 × 宽 × 厚度相关”
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arrows")
	FVector ArrowScale = FVector(0.18f, 0.08f, 1.0f);

	// 箭头朝向补偿
	// 如果方向不对，你可以在 Details 里改
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arrows")
	FRotator ArrowRotationOffset = FRotator::ZeroRotator;

	// 箭头流动速度
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arrows", meta = (ClampMin = "0.0"))
	float ArrowMoveSpeed = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arrows")
	bool bAutoCenterArrowByMeshBounds = true;

private:
	// 运行时创建出来的主路径网格段
	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> PathMeshComponents;

	// 运行时创建出来的中轴线网格段
	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> CenterLineMeshComponents;

	// 箭头动画偏移
	UPROPERTY(Transient)
	float ArrowMoveOffset = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ArrowMID;
};