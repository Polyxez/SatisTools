#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Data/SatisToolsMapTypes.h"
#include "UI/SSatisToolsMapView.h"
#include "SatisToolsMapWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZoneAnalyticsReceived, const FSatisZoneAnalytics&, Analytics);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZoneSavedEvent, const FSatisZoneData&, SavedZone);

UCLASS()
class SATISTOOLS_API USatisToolsMapWidget : public UWidget
{
	GENERATED_BODY()

public:
	USatisToolsMapWidget();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SatisTools|Layers")
	bool bShowResources = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SatisTools|Layers")
	bool bShowDropPods = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SatisTools|Layers")
	bool bShowFactories = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SatisTools|Layers")
	bool bShowConveyors = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SatisTools|Layers")
	bool bShowPipelines = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SatisTools|Layers")
	bool bShowZones = true;

	UPROPERTY(BlueprintAssignable, Category = "SatisTools|Events")
	FOnZoneAnalyticsReceived OnZoneAnalyticsReceived;

	UPROPERTY(BlueprintAssignable, Category = "SatisTools|Events")
	FOnZoneSavedEvent OnZoneSavedEvent;

	UFUNCTION(BlueprintCallable, Category = "SatisTools|Control")
	void SetInteractionMode(int32 ModeIndex);

	UFUNCTION(BlueprintCallable, Category = "SatisTools|Control")
	void RefreshMap();

	UFUNCTION(BlueprintCallable, Category = "SatisTools|Control")
	void ResetCamera();

	UFUNCTION(BlueprintCallable, Category = "SatisTools|Control")
	void SetLayers(bool Resources, bool DropPods, bool Factories, bool Conveyors, bool Pipelines, bool Zones);

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void HandleZoneSelectionCompleted(const FSatisZoneAnalytics& Analytics);
	void HandleZoneCreated(const FSatisZoneData& Zone);

	TSharedPtr<SSatisToolsMapView> MapViewSlate;
};
