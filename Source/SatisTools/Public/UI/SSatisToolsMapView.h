#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Data/SatisToolsMapTypes.h"

class USatisToolsDataSubsystem;

DECLARE_DELEGATE_OneParam(FOnZoneSelectionCompleted, const FSatisZoneAnalytics&);
DECLARE_DELEGATE_OneParam(FOnZoneCreated, const FSatisZoneData&);

enum class ESatisMapInteractionMode : uint8
{
	Pan,
	BoxSelect,
	LassoSelect
};

class SATISTOOLS_API SSatisToolsMapView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSatisToolsMapView)
		: _ShowResources(true)
		, _ShowDropPods(true)
		, _ShowFactories(true)
		, _ShowConveyors(true)
		, _ShowPipelines(true)
		, _ShowZones(true)
	{}
		SLATE_ARGUMENT(bool, ShowResources)
		SLATE_ARGUMENT(bool, ShowDropPods)
		SLATE_ARGUMENT(bool, ShowFactories)
		SLATE_ARGUMENT(bool, ShowConveyors)
		SLATE_ARGUMENT(bool, ShowPipelines)
		SLATE_ARGUMENT(bool, ShowZones)
		SLATE_EVENT(FOnZoneSelectionCompleted, OnZoneSelectionCompleted)
		SLATE_EVENT(FOnZoneCreated, OnZoneCreated)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, USatisToolsDataSubsystem* InDataSubsystem);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	void SetInteractionMode(ESatisMapInteractionMode NewMode);
	void SetLayerVisibility(bool InResources, bool InDropPods, bool InFactories, bool InConveyors, bool InPipelines, bool InZones);
	void ResetView();

private:
	FVector2D WorldToScreen(const FVector& WorldPos, const FGeometry& Geometry) const;
	FVector ScreenToWorld(const FVector2D& ScreenPos, const FGeometry& Geometry) const;

	TWeakObjectPtr<USatisToolsDataSubsystem> DataSubsystem;

	ESatisMapInteractionMode CurrentInteractionMode = ESatisMapInteractionMode::Pan;
	bool bIsPanning = false;
	bool bIsSelecting = false;
	FVector2D LastMousePosition = FVector2D::ZeroVector;
	FVector2D ViewOffset = FVector2D::ZeroVector;
	float ZoomLevel = 1.0f;

	FVector2D SelectionStartScreen = FVector2D::ZeroVector;
	FVector2D SelectionCurrentScreen = FVector2D::ZeroVector;
	TArray<FVector2D> LassoPointsScreen;

	// Layer flags
	bool bShowResources = true;
	bool bShowDropPods = true;
	bool bShowFactories = true;
	bool bShowConveyors = true;
	bool bShowPipelines = true;
	bool bShowZones = true;

	FOnZoneSelectionCompleted OnZoneSelectionCompleted;
	FOnZoneCreated OnZoneCreated;
};
