#include "UI/SatisToolsMapWidget.h"
#include "Data/SatisToolsDataSubsystem.h"

USatisToolsMapWidget::USatisToolsMapWidget()
{
	bShowResources = true;
	bShowDropPods = true;
	bShowFactories = true;
	bShowConveyors = true;
	bShowPipelines = true;
	bShowZones = true;
}

TSharedRef<SWidget> USatisToolsMapWidget::RebuildWidget()
{
	USatisToolsDataSubsystem* Subsystem = USatisToolsDataSubsystem::Get(this);

	MapViewSlate = SNew(SSatisToolsMapView, Subsystem)
		.ShowResources(bShowResources)
		.ShowDropPods(bShowDropPods)
		.ShowFactories(bShowFactories)
		.ShowConveyors(bShowConveyors)
		.ShowPipelines(bShowPipelines)
		.ShowZones(bShowZones)
		.OnZoneSelectionCompleted(FOnZoneSelectionCompleted::CreateUObject(this, &USatisToolsMapWidget::HandleZoneSelectionCompleted))
		.OnZoneCreated(FOnZoneCreated::CreateUObject(this, &USatisToolsMapWidget::HandleZoneCreated));

	return MapViewSlate.ToSharedRef();
}

void USatisToolsMapWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MapViewSlate.Reset();
}

void USatisToolsMapWidget::SetInteractionMode(int32 ModeIndex)
{
	if (!MapViewSlate.IsValid())
	{
		return;
	}

	ESatisMapInteractionMode Mode = ESatisMapInteractionMode::Pan;
	if (ModeIndex == 1)
	{
		Mode = ESatisMapInteractionMode::BoxSelect;
	}
	else if (ModeIndex == 2)
	{
		Mode = ESatisMapInteractionMode::LassoSelect;
	}

	MapViewSlate->SetInteractionMode(Mode);
}

void USatisToolsMapWidget::RefreshMap()
{
	if (USatisToolsDataSubsystem* Subsystem = USatisToolsDataSubsystem::Get(this))
	{
		Subsystem->RefreshWorldData();
	}
}

void USatisToolsMapWidget::ResetCamera()
{
	if (MapViewSlate.IsValid())
	{
		MapViewSlate->ResetView();
	}
}

void USatisToolsMapWidget::SetLayers(bool Resources, bool DropPods, bool Factories, bool Conveyors, bool Pipelines, bool Zones)
{
	bShowResources = Resources;
	bShowDropPods = DropPods;
	bShowFactories = Factories;
	bShowConveyors = Conveyors;
	bShowPipelines = Pipelines;
	bShowZones = Zones;

	if (MapViewSlate.IsValid())
	{
		MapViewSlate->SetLayerVisibility(Resources, DropPods, Factories, Conveyors, Pipelines, Zones);
	}
}

void USatisToolsMapWidget::HandleZoneSelectionCompleted(const FSatisZoneAnalytics& Analytics)
{
	OnZoneAnalyticsReceived.Broadcast(Analytics);
}

void USatisToolsMapWidget::HandleZoneCreated(const FSatisZoneData& Zone)
{
	OnZoneSavedEvent.Broadcast(Zone);
}

#if WITH_EDITOR
const FText USatisToolsMapWidget::GetPaletteCategory()
{
	return FText::FromString(TEXT("SatisTools"));
}
#endif
