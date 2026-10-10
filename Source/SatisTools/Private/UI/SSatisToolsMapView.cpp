#include "UI/SSatisToolsMapView.h"
#include "Data/SatisToolsDataSubsystem.h"
#include "Rendering/DrawElements.h"

void SSatisToolsMapView::Construct(const FArguments& InArgs, USatisToolsDataSubsystem* InDataSubsystem)
{
	DataSubsystem = InDataSubsystem;
	bShowResources = InArgs._ShowResources;
	bShowDropPods = InArgs._ShowDropPods;
	bShowFactories = InArgs._ShowFactories;
	bShowConveyors = InArgs._ShowConveyors;
	bShowPipelines = InArgs._ShowPipelines;
	bShowZones = InArgs._ShowZones;

	OnZoneSelectionCompleted = InArgs._OnZoneSelectionCompleted;
	OnZoneCreated = InArgs._OnZoneCreated;

	ZoomLevel = 1.0f;
	ViewOffset = FVector2D::ZeroVector;
}

FVector2D SSatisToolsMapView::WorldToScreen(const FVector& WorldPos, const FGeometry& Geometry) const
{
	const FVector2D UV = USatisToolsDataSubsystem::WorldToNormalizedUV(WorldPos);
	const FVector2D LocalSize = Geometry.GetLocalSize();
	return (UV * LocalSize * ZoomLevel) + ViewOffset;
}

FVector SSatisToolsMapView::ScreenToWorld(const FVector2D& ScreenPos, const FGeometry& Geometry) const
{
	const FVector2D LocalSize = Geometry.GetLocalSize();
	if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f || ZoomLevel <= 0.0f)
	{
		return FVector::ZeroVector;
	}

	const FVector2D UV = (ScreenPos - ViewOffset) / (LocalSize * ZoomLevel);
	return USatisToolsDataSubsystem::NormalizedUVToWorld(UV);
}

int32 SSatisToolsMapView::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FSlateBrush WhiteBrush;

	// 1. Draw Map Background
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(LocalSize, FSlateLayoutTransform()),
		&WhiteBrush,
		ESlateDrawEffect::None,
		FLinearColor(0.08f, 0.09f, 0.11f, 1.0f)
	);
	LayerId++;

	if (!DataSubsystem.IsValid())
	{
		return LayerId;
	}

	const FSlateRect ViewRect(0.0f, 0.0f, LocalSize.X, LocalSize.Y);

	// 2. Draw Conveyors (Batched Lines)
	if (bShowConveyors)
	{
		const TArray<FSatisSplineData>& Conveyors = DataSubsystem->GetConveyors();
		for (const FSatisSplineData& Spline : Conveyors)
		{
			if (Spline.WorldPoints.Num() < 2)
			{
				continue;
			}

			TArray<FVector2D> ScreenLine;
			ScreenLine.Reserve(Spline.WorldPoints.Num());

			for (const FVector& Pt : Spline.WorldPoints)
			{
				ScreenLine.Add(WorldToScreen(Pt, AllottedGeometry));
			}

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				ScreenLine,
				ESlateDrawEffect::None,
				Spline.Color,
				false,
				1.5f
			);
		}
		LayerId++;
	}

	// 3. Draw Pipelines
	if (bShowPipelines)
	{
		const TArray<FSatisSplineData>& Pipelines = DataSubsystem->GetPipelines();
		for (const FSatisSplineData& Pipe : Pipelines)
		{
			if (Pipe.WorldPoints.Num() < 2)
			{
				continue;
			}

			TArray<FVector2D> ScreenLine;
			ScreenLine.Reserve(Pipe.WorldPoints.Num());

			for (const FVector& Pt : Pipe.WorldPoints)
			{
				ScreenLine.Add(WorldToScreen(Pt, AllottedGeometry));
			}

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				ScreenLine,
				ESlateDrawEffect::None,
				Pipe.Color,
				false,
				2.0f
			);
		}
		LayerId++;
	}

	// 4. Draw Factories / Buildings
	if (bShowFactories)
	{
		const TArray<FSatisBuildingData>& Buildings = DataSubsystem->GetBuildings();
		const float BoxSize = FMath::Clamp(4.0f * ZoomLevel, 3.0f, 12.0f);

		for (const FSatisBuildingData& Building : Buildings)
		{
			const FVector2D ScreenPos = WorldToScreen(Building.Location, AllottedGeometry);
			if (!ViewRect.ContainsPoint(ScreenPos))
			{
				continue;
			}

			FLinearColor StatusColor = FLinearColor::Green;
			if (Building.Status == ESatisBuildingStatus::Starved)
			{
				StatusColor = FLinearColor(1.0f, 0.8f, 0.1f, 1.0f); // Yellow
			}
			else if (Building.Status == ESatisBuildingStatus::Full || Building.Status == ESatisBuildingStatus::NoPower)
			{
				StatusColor = FLinearColor::Red;
			}

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2D(BoxSize, BoxSize), FSlateLayoutTransform(ScreenPos - FVector2D(BoxSize * 0.5f, BoxSize * 0.5f))),
				&WhiteBrush,
				ESlateDrawEffect::None,
				StatusColor
			);
		}
		LayerId++;
	}

	// 5. Draw Resource Nodes
	if (bShowResources)
	{
		const TArray<FSatisResourceNodeData>& Nodes = DataSubsystem->GetResourceNodes();
		const float NodeSize = FMath::Clamp(6.0f * ZoomLevel, 4.0f, 16.0f);

		for (const FSatisResourceNodeData& Node : Nodes)
		{
			const FVector2D ScreenPos = WorldToScreen(Node.Location, AllottedGeometry);
			if (!ViewRect.ContainsPoint(ScreenPos))
			{
				continue;
			}

			FLinearColor NodeColor = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);
			if (Node.Purity == ESatisNodePurity::Pure)
			{
				NodeColor = FLinearColor(1.0f, 0.3f, 0.9f, 1.0f); // Pure: Magenta
			}
			else if (Node.Purity == ESatisNodePurity::Impure)
			{
				NodeColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.8f); // Impure: Dim Grey
			}

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2D(NodeSize, NodeSize), FSlateLayoutTransform(ScreenPos - FVector2D(NodeSize * 0.5f, NodeSize * 0.5f))),
				&WhiteBrush,
				ESlateDrawEffect::None,
				NodeColor
			);
		}
		LayerId++;
	}

	// 6. Draw Drop Pods
	if (bShowDropPods)
	{
		const TArray<FSatisDropPodData>& Pods = DataSubsystem->GetDropPods();
		const float PodSize = 8.0f;

		for (const FSatisDropPodData& Pod : Pods)
		{
			const FVector2D ScreenPos = WorldToScreen(Pod.Location, AllottedGeometry);
			if (!ViewRect.ContainsPoint(ScreenPos))
			{
				continue;
			}

			const FLinearColor PodColor = Pod.bHasBeenOpened ? FLinearColor(0.3f, 0.3f, 0.3f, 0.5f) : FLinearColor(1.0f, 0.6f, 0.0f, 1.0f);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2D(PodSize, PodSize), FSlateLayoutTransform(ScreenPos - FVector2D(PodSize * 0.5f, PodSize * 0.5f))),
				&WhiteBrush,
				ESlateDrawEffect::None,
				PodColor
			);
		}
		LayerId++;
	}

	// 7. Draw Saved Zones
	if (bShowZones)
	{
		for (const FSatisZoneData& Zone : DataSubsystem->GetSavedZones())
		{
			if (Zone.PolygonPointsWorld.Num() < 3)
			{
				continue;
			}

			TArray<FVector2D> ZoneScreen;
			for (const FVector2D& Pt : Zone.PolygonPointsWorld)
			{
				ZoneScreen.Add(WorldToScreen(FVector(Pt.X, Pt.Y, 0.0f), AllottedGeometry));
			}
			ZoneScreen.Add(ZoneScreen[0]); // Close loop

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				ZoneScreen,
				ESlateDrawEffect::None,
				Zone.BorderColor,
				false,
				2.5f
			);
		}
		LayerId++;
	}

	// 8. Draw Active Selection (Box or Lasso)
	if (bIsSelecting)
	{
		if (CurrentInteractionMode == ESatisMapInteractionMode::BoxSelect)
		{
			const FVector2D MinPt(FMath::Min(SelectionStartScreen.X, SelectionCurrentScreen.X), FMath::Min(SelectionStartScreen.Y, SelectionCurrentScreen.Y));
			const FVector2D MaxPt(FMath::Max(SelectionStartScreen.X, SelectionCurrentScreen.X), FMath::Max(SelectionStartScreen.Y, SelectionCurrentScreen.Y));
			const FVector2D BoxDimensions = MaxPt - MinPt;

			// Semi-transparent fill
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(BoxDimensions, FSlateLayoutTransform(MinPt)),
				&WhiteBrush,
				ESlateDrawEffect::None,
				FLinearColor(0.0f, 0.7f, 1.0f, 0.15f)
			);

			// Outline
			TArray<FVector2D> BoxOutline;
			BoxOutline.Add(MinPt);
			BoxOutline.Add(FVector2D(MaxPt.X, MinPt.Y));
			BoxOutline.Add(MaxPt);
			BoxOutline.Add(FVector2D(MinPt.X, MaxPt.Y));
			BoxOutline.Add(MinPt);

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(),
				BoxOutline,
				ESlateDrawEffect::None,
				FLinearColor(0.0f, 0.8f, 1.0f, 0.9f),
				false,
				2.0f
			);
			LayerId += 2;
		}
		else if (CurrentInteractionMode == ESatisMapInteractionMode::LassoSelect && LassoPointsScreen.Num() > 1)
		{
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				LassoPointsScreen,
				ESlateDrawEffect::None,
				FLinearColor(0.2f, 1.0f, 0.4f, 0.9f),
				false,
				2.0f
			);
			LayerId++;
		}
	}

	return LayerId;
}

FReply SSatisToolsMapView::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D MousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton || MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		bIsPanning = true;
		LastMousePosition = MousePos;
		return FReply::Handled().CaptureMouse(AsShared());
	}

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (CurrentInteractionMode == ESatisMapInteractionMode::Pan)
		{
			bIsPanning = true;
			LastMousePosition = MousePos;
			return FReply::Handled().CaptureMouse(AsShared());
		}
		else if (CurrentInteractionMode == ESatisMapInteractionMode::BoxSelect)
		{
			bIsSelecting = true;
			SelectionStartScreen = MousePos;
			SelectionCurrentScreen = MousePos;
			return FReply::Handled().CaptureMouse(AsShared());
		}
		else if (CurrentInteractionMode == ESatisMapInteractionMode::LassoSelect)
		{
			bIsSelecting = true;
			LassoPointsScreen.Reset();
			LassoPointsScreen.Add(MousePos);
			return FReply::Handled().CaptureMouse(AsShared());
		}
	}

	return FReply::Unhandled();
}

FReply SSatisToolsMapView::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D MousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	if (bIsPanning)
	{
		const FVector2D Delta = MousePos - LastMousePosition;
		ViewOffset += Delta;
		LastMousePosition = MousePos;
		return FReply::Handled();
	}

	if (bIsSelecting)
	{
		if (CurrentInteractionMode == ESatisMapInteractionMode::BoxSelect)
		{
			SelectionCurrentScreen = MousePos;
			return FReply::Handled();
		}
		else if (CurrentInteractionMode == ESatisMapInteractionMode::LassoSelect)
		{
			if (LassoPointsScreen.Num() == 0 || FVector2D::Distance(LassoPointsScreen.Last(), MousePos) > 5.0f)
			{
				LassoPointsScreen.Add(MousePos);
			}
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SSatisToolsMapView::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsPanning && (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton ||
		MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton ||
		MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton))
	{
		bIsPanning = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	if (bIsSelecting && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsSelecting = false;

		if (DataSubsystem.IsValid())
		{
			if (CurrentInteractionMode == ESatisMapInteractionMode::BoxSelect)
			{
				const FVector WorldPt1 = ScreenToWorld(SelectionStartScreen, MyGeometry);
				const FVector WorldPt2 = ScreenToWorld(SelectionCurrentScreen, MyGeometry);

				const FVector2D BoxMin(FMath::Min(WorldPt1.X, WorldPt2.X), FMath::Min(WorldPt1.Y, WorldPt2.Y));
				const FVector2D BoxMax(FMath::Max(WorldPt1.X, WorldPt2.X), FMath::Max(WorldPt1.Y, WorldPt2.Y));

				FSatisZoneAnalytics Analytics = DataSubsystem->AnalyzeZoneBox(BoxMin, BoxMax);
				OnZoneSelectionCompleted.ExecuteIfBound(Analytics);
			}
			else if (CurrentInteractionMode == ESatisMapInteractionMode::LassoSelect && LassoPointsScreen.Num() >= 3)
			{
				TArray<FVector2D> WorldPolygon;
				WorldPolygon.Reserve(LassoPointsScreen.Num());

				for (const FVector2D& ScreenPt : LassoPointsScreen)
				{
					const FVector WorldPos = ScreenToWorld(ScreenPt, MyGeometry);
					WorldPolygon.Add(FVector2D(WorldPos.X, WorldPos.Y));
				}

				FSatisZoneAnalytics Analytics = DataSubsystem->AnalyzeZonePolygon(WorldPolygon);
				OnZoneSelectionCompleted.ExecuteIfBound(Analytics);
			}
		}

		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}

FReply SSatisToolsMapView::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const float WheelDelta = MouseEvent.GetWheelDelta();
	const float OldZoom = ZoomLevel;
	ZoomLevel = FMath::Clamp(ZoomLevel * (1.0f + WheelDelta * 0.15f), 0.2f, 35.0f);

	const FVector2D MouseLocal = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	ViewOffset = MouseLocal - (MouseLocal - ViewOffset) * (ZoomLevel / OldZoom);

	return FReply::Handled();
}

void SSatisToolsMapView::SetInteractionMode(ESatisMapInteractionMode NewMode)
{
	CurrentInteractionMode = NewMode;
	bIsSelecting = false;
	LassoPointsScreen.Empty();
}

void SSatisToolsMapView::SetLayerVisibility(bool InResources, bool InDropPods, bool InFactories, bool InConveyors, bool InPipelines, bool InZones)
{
	bShowResources = InResources;
	bShowDropPods = InDropPods;
	bShowFactories = InFactories;
	bShowConveyors = InConveyors;
	bShowPipelines = InPipelines;
	bShowZones = InZones;
}

void SSatisToolsMapView::ResetView()
{
	ZoomLevel = 1.0f;
	ViewOffset = FVector2D::ZeroVector;
}
