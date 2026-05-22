#pragma once

#include "Components/ComboBoxString.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TargetSelectionWidget.generated.h"

class UButton;
class USlider;
class UTextBlock;
class UBorder;
class UVerticalBox;
class UComboBoxString;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStartTargetSelectionRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConfirmTargetSelectionRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCancelTargetSelectionRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetHeightNormalizedChanged, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectedDroneChanged, FString, DroneId);

UCLASS()
class ZGD_DK_API UTargetSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStartTargetSelectionRequested OnStartTargetSelectionRequested;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnConfirmTargetSelectionRequested OnConfirmTargetSelectionRequested;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCancelTargetSelectionRequested OnCancelTargetSelectionRequested;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTargetHeightNormalizedChanged OnTargetHeightNormalizedChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSelectedDroneChanged OnSelectedDroneChanged;

	void SetAvailableDroneIds(const TArray<FString>& InDroneIds, const FString& InSelectedDroneId);

	FString GetSelectedDroneId() const;
	void SetSelectionMode(bool bInSelectionMode, bool bInHasSelectedXY);
	void SetHeightNormalized(float InNormalizedValue);
	float GetHeightNormalized() const;
	void SetHeightValue(float InHeightCm);
	void SetHintMessage(const FString& InMessage);

private:
	UFUNCTION()
	void HandleStartButtonClicked();

	UFUNCTION()
	void HandleConfirmButtonClicked();

	UFUNCTION()
	void HandleCancelButtonClicked();

	UFUNCTION()
	void HandleHeightSliderChanged(float InValue);

	UFUNCTION()
	void HandleDroneComboSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	void BuildWidgetTreeIfNeeded();
	void RefreshVisualState();

private:
	UPROPERTY()
	UButton* StartSelectButton = nullptr;

	UPROPERTY()
	UButton* ConfirmButton = nullptr;

	UPROPERTY()
	UButton* CancelButton = nullptr;

	UPROPERTY()
	USlider* HeightSlider = nullptr;

	UPROPERTY()
	UTextBlock* TitleText = nullptr;

	UPROPERTY()
	UTextBlock* HintText = nullptr;

	UPROPERTY()
	UTextBlock* HeightValueText = nullptr;

	UPROPERTY()
	UTextBlock* HeightLabelText = nullptr;

	UPROPERTY()
	UBorder* RootBorder = nullptr;

	UPROPERTY()
	UVerticalBox* RootVBox = nullptr;

	UPROPERTY()
	UTextBlock* DroneLabelText = nullptr;

	UPROPERTY()
	UComboBoxString* DroneComboBox = nullptr;

	bool bSelectionMode = false;
	bool bHasSelectedXY = false;
	FString CachedHintMessage = TEXT("Click Start to begin target selection.");
	FString CachedSelectedDroneId = TEXT("UAV_001");
};