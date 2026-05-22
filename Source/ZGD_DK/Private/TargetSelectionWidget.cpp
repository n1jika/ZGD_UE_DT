#include "TargetSelectionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Widgets/SNullWidget.h"

TSharedRef<SWidget> UTargetSelectionWidget::RebuildWidget()
{
	BuildWidgetTreeIfNeeded();
	return Super::RebuildWidget();
}

void UTargetSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildWidgetTreeIfNeeded();

	if (StartSelectButton)
	{
		StartSelectButton->OnClicked.Clear();
		StartSelectButton->OnClicked.AddDynamic(this, &UTargetSelectionWidget::HandleStartButtonClicked);
	}

	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.Clear();
		ConfirmButton->OnClicked.AddDynamic(this, &UTargetSelectionWidget::HandleConfirmButtonClicked);
	}

	if (CancelButton)
	{
		CancelButton->OnClicked.Clear();
		CancelButton->OnClicked.AddDynamic(this, &UTargetSelectionWidget::HandleCancelButtonClicked);
	}

	if (HeightSlider)
	{
		HeightSlider->OnValueChanged.Clear();
		HeightSlider->OnValueChanged.AddDynamic(this, &UTargetSelectionWidget::HandleHeightSliderChanged);
	}

	if (DroneComboBox)
	{
		DroneComboBox->OnSelectionChanged.Clear();
		DroneComboBox->OnSelectionChanged.AddDynamic(this, &UTargetSelectionWidget::HandleDroneComboSelectionChanged);
	}

	RefreshVisualState();
}

void UTargetSelectionWidget::BuildWidgetTreeIfNeeded()
{
	if (WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	// RootCanvas 只作为布局容器，不拦截 UI 面板外的场景点击
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	RootBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.75f));

	RootCanvas->AddChild(RootBorder);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(RootBorder->Slot))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetPosition(FVector2D(20.0f, 20.0f));
	}

	RootVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootVBox"));
	RootBorder->SetContent(RootVBox);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("Target Selection")));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UVerticalBoxSlot* VBoxSlot = RootVBox->AddChildToVerticalBox(TitleText))
	{
		VBoxSlot->SetPadding(FMargin(12.0f, 10.0f, 12.0f, 6.0f));
	}

	HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
	HintText->SetText(FText::FromString(CachedHintMessage));
	HintText->SetAutoWrapText(true);
	HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f)));
	if (UVerticalBoxSlot* VBoxSlot = RootVBox->AddChildToVerticalBox(HintText))
	{
		VBoxSlot->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 8.0f));
	}

	UHorizontalBox* DroneRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DroneRow"));
	if (DroneRow)
	{
		if (UVerticalBoxSlot* VBoxSlot = RootVBox->AddChildToVerticalBox(DroneRow))
		{
			VBoxSlot->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 8.0f));
		}

		DroneLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DroneLabelText"));
		if (DroneLabelText)
		{
			DroneLabelText->SetText(FText::FromString(TEXT("Drone")));
			DroneLabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));

			if (UHorizontalBoxSlot* LabelSlot = DroneRow->AddChildToHorizontalBox(DroneLabelText))
			{
				LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
				LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			}
		}

		DroneComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("DroneComboBox"));
		if (DroneComboBox)
		{
			DroneComboBox->AddOption(TEXT("UAV_001"));
			DroneComboBox->SetSelectedOption(TEXT("UAV_001"));

			if (UHorizontalBoxSlot* ComboSlot = DroneRow->AddChildToHorizontalBox(DroneComboBox))
			{
				ComboSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}
	}

	StartSelectButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartSelectButton"));
	UTextBlock* StartButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartButtonText"));
	StartButtonText->SetText(FText::FromString(TEXT("Start")));
	StartButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	StartSelectButton->AddChild(StartButtonText);
	if (UVerticalBoxSlot* VBoxSlot = RootVBox->AddChildToVerticalBox(StartSelectButton))
	{
		VBoxSlot->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 8.0f));
	}

	HeightLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HeightLabelText"));
	HeightLabelText->SetText(FText::FromString(TEXT("Target Height")));
	HeightLabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UVerticalBoxSlot* VBoxSlot = RootVBox->AddChildToVerticalBox(HeightLabelText))
	{
		VBoxSlot->SetPadding(FMargin(12.0f, 4.0f, 12.0f, 2.0f));
	}

	HeightSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("HeightSlider"));
	HeightSlider->SetValue(0.1f);
	if (UVerticalBoxSlot* VBoxSlot = RootVBox->AddChildToVerticalBox(HeightSlider))
	{
		VBoxSlot->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 6.0f));
	}

	HeightValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HeightValueText"));
	HeightValueText->SetText(FText::FromString(TEXT("Target Height: 0 cm")));
	HeightValueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 1.0f, 0.75f, 1.0f)));
	if (UVerticalBoxSlot* VBoxSlot = RootVBox->AddChildToVerticalBox(HeightValueText))
	{
		VBoxSlot->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 8.0f));
	}

	ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
	UTextBlock* ConfirmButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmButtonText"));
	ConfirmButtonText->SetText(FText::FromString(TEXT("Confirm Target")));
	ConfirmButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ConfirmButton->AddChild(ConfirmButtonText);
	if (UVerticalBoxSlot* VBoxSlot = RootVBox->AddChildToVerticalBox(ConfirmButton))
	{
		VBoxSlot->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 6.0f));
	}

	CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelButton"));
	UTextBlock* CancelButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CancelButtonText"));
	CancelButtonText->SetText(FText::FromString(TEXT("Cancel")));
	CancelButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CancelButton->AddChild(CancelButtonText);
	if (UVerticalBoxSlot* VBoxSlot = RootVBox->AddChildToVerticalBox(CancelButton))
	{
		VBoxSlot->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 12.0f));
	}
}

void UTargetSelectionWidget::SetAvailableDroneIds(const TArray<FString>& InDroneIds, const FString& InSelectedDroneId)
{
	BuildWidgetTreeIfNeeded();

	if (!DroneComboBox)
	{
		return;
	}

	DroneComboBox->ClearOptions();

	for (const FString& DroneId : InDroneIds)
	{
		if (!DroneId.IsEmpty())
		{
			DroneComboBox->AddOption(DroneId);
		}
	}

	FString TargetDroneId = InSelectedDroneId;

	if (TargetDroneId.IsEmpty() || DroneComboBox->FindOptionIndex(TargetDroneId) == INDEX_NONE)
	{
		if (DroneComboBox->GetOptionCount() > 0)
		{
			TargetDroneId = DroneComboBox->GetOptionAtIndex(0);
		}
		else
		{
			TargetDroneId = TEXT("UAV_001");
			DroneComboBox->AddOption(TargetDroneId);
		}
	}

	CachedSelectedDroneId = TargetDroneId;
	DroneComboBox->SetSelectedOption(CachedSelectedDroneId);
}

FString UTargetSelectionWidget::GetSelectedDroneId() const
{
	if (DroneComboBox)
	{
		const FString SelectedOption = DroneComboBox->GetSelectedOption();
		if (!SelectedOption.IsEmpty())
		{
			return SelectedOption;
		}
	}

	return CachedSelectedDroneId;
}

void UTargetSelectionWidget::SetSelectionMode(bool bInSelectionMode, bool bInHasSelectedXY)
{
	bSelectionMode = bInSelectionMode;
	bHasSelectedXY = bInHasSelectedXY;
	RefreshVisualState();
}

void UTargetSelectionWidget::SetHeightNormalized(float InNormalizedValue)
{
	if (HeightSlider)
	{
		HeightSlider->SetValue(InNormalizedValue);
	}
}

float UTargetSelectionWidget::GetHeightNormalized() const
{
	return HeightSlider ? HeightSlider->GetValue() : 0.0f;
}

void UTargetSelectionWidget::SetHeightValue(float InHeightCm)
{
	if (HeightValueText)
	{
		HeightValueText->SetText(FText::FromString(FString::Printf(TEXT("Target Height: %.0f cm"), InHeightCm)));
	}
}

void UTargetSelectionWidget::SetHintMessage(const FString& InMessage)
{
	CachedHintMessage = InMessage;

	if (HintText)
	{
		HintText->SetText(FText::FromString(CachedHintMessage));
	}
}

void UTargetSelectionWidget::HandleStartButtonClicked()
{
	OnStartTargetSelectionRequested.Broadcast();
}

void UTargetSelectionWidget::HandleConfirmButtonClicked()
{
	OnConfirmTargetSelectionRequested.Broadcast();
}

void UTargetSelectionWidget::HandleCancelButtonClicked()
{
	OnCancelTargetSelectionRequested.Broadcast();
}

void UTargetSelectionWidget::HandleHeightSliderChanged(float InValue)
{
	OnTargetHeightNormalizedChanged.Broadcast(InValue);
}

void UTargetSelectionWidget::HandleDroneComboSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectedItem.IsEmpty())
	{
		return;
	}

	CachedSelectedDroneId = SelectedItem;
	OnSelectedDroneChanged.Broadcast(CachedSelectedDroneId);
}

void UTargetSelectionWidget::RefreshVisualState()
{
	if (StartSelectButton)
	{
		StartSelectButton->SetVisibility(bSelectionMode ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	const ESlateVisibility SelectionVisibility = bSelectionMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

	if (HeightLabelText)
	{
		HeightLabelText->SetVisibility(SelectionVisibility);
	}

	if (HeightSlider)
	{
		HeightSlider->SetVisibility(SelectionVisibility);
	}

	if (HeightValueText)
	{
		HeightValueText->SetVisibility(SelectionVisibility);
	}

	if (ConfirmButton)
	{
		ConfirmButton->SetVisibility(SelectionVisibility);
		ConfirmButton->SetIsEnabled(bHasSelectedXY);
	}

	if (CancelButton)
	{
		CancelButton->SetVisibility(SelectionVisibility);
	}

	if (HintText)
	{
		HintText->SetText(FText::FromString(CachedHintMessage));
	}
}