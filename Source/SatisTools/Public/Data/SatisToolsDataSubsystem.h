#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SatisToolsMapTypes.h"
#include "SatisToolsDataSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSatisDataRefreshed);

UCLASS()
class SATISTOOLS_API USatisToolsDataSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static USatisToolsDataSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Scans the world and refreshes all cached data */
	UFUNCTION(BlueprintCallable, Category = "SatisTools|Data")
	void RefreshWorldData();

	/** Coordinate conversion: Unreal World XY (cm) to Normalized Map UV [0..1] */
	UFUNCTION(BlueprintPure, Category = "SatisTools|Coordinates")
	static FVector2D WorldToNormalizedUV(const FVector& WorldLocation);

	/** Coordinate conversion: Normalized Map UV [0..1] to Unreal World XY (cm) */
	UFUNCTION(BlueprintPure, Category = "SatisTools|Coordinates")
	static FVector NormalizedUVToWorld(const FVector2D& UV, float DefaultZ = 0.0f);

	/** Analyzes all buildings inside a given 2D polygon (in World coordinates) */
	UFUNCTION(BlueprintCallable, Category = "SatisTools|Analytics")
	FSatisZoneAnalytics AnalyzeZonePolygon(const TArray<FVector2D>& PolygonPointsWorld) const;

	/** Analyzes all buildings inside a given 2D bounding box (in World coordinates) */
	UFUNCTION(BlueprintCallable, Category = "SatisTools|Analytics")
	FSatisZoneAnalytics AnalyzeZoneBox(const FVector2D& BoxMinWorld, const FVector2D& BoxMaxWorld) const;

	// Zone Bookmarks Management
	UFUNCTION(BlueprintCallable, Category = "SatisTools|Zones")
	void SaveZone(const FSatisZoneData& NewZone);

	UFUNCTION(BlueprintCallable, Category = "SatisTools|Zones")
	void DeleteZone(const FGuid& ZoneId);

	UFUNCTION(BlueprintPure, Category = "SatisTools|Zones")
	TArray<FSatisZoneData> GetSavedZones() const;

	// Data Getters
	const TArray<FSatisResourceNodeData>& GetResourceNodes() const { return CachedResourceNodes; }
	const TArray<FSatisDropPodData>& GetDropPods() const { return CachedDropPods; }
	const TArray<FSatisBuildingData>& GetBuildings() const { return CachedBuildings; }
	const TArray<FSatisSplineData>& GetConveyors() const { return CachedConveyors; }
	const TArray<FSatisSplineData>& GetPipelines() const { return CachedPipelines; }

	UPROPERTY(BlueprintAssignable, Category = "SatisTools|Data")
	FOnSatisDataRefreshed OnDataRefreshed;

private:
	static bool IsPointInsidePolygon(const FVector2D& Point, const TArray<FVector2D>& Polygon);

	void CollectResourceNodes();
	void CollectDropPods();
	void CollectBuildables();

	TArray<FSatisResourceNodeData> CachedResourceNodes;
	TArray<FSatisDropPodData> CachedDropPods;
	TArray<FSatisBuildingData> CachedBuildings;
	TArray<FSatisSplineData> CachedConveyors;
	TArray<FSatisSplineData> CachedPipelines;
	TArray<FSatisZoneData> SavedZones;
};
