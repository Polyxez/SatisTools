#include "Data/SatisToolsDataSubsystem.h"
#include "EngineUtils.h"
#include "Resources/FGResourceNode.h"
#include "FGDropPod.h"
#include "FGBuildableSubsystem.h"
#include "Buildables/FGBuildableFactory.h"
#include "Buildables/FGBuildableManufacturer.h"
#include "Buildables/FGBuildableResourceExtractor.h"
#include "Buildables/FGBuildableConveyorBelt.h"
#include "Buildables/FGBuildablePipeline.h"
#include "FGRecipe.h"
#include "FGItemDescriptor.h"
#include "Components/SplineComponent.h"

namespace SatisCoords
{
	static const float WorldMinX = -324698.0f;
	static const float WorldMaxX = 425301.0f;
	static const float WorldMinY = -375000.0f;
	static const float WorldMaxY = 375000.0f;
}

USatisToolsDataSubsystem* USatisToolsDataSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetSubsystem<USatisToolsDataSubsystem>() : nullptr;
}

void USatisToolsDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USatisToolsDataSubsystem::Deinitialize()
{
	CachedResourceNodes.Empty();
	CachedDropPods.Empty();
	CachedBuildings.Empty();
	CachedConveyors.Empty();
	CachedPipelines.Empty();
	Super::Deinitialize();
}

FVector2D USatisToolsDataSubsystem::WorldToNormalizedUV(const FVector& WorldLocation)
{
	const float RangeX = SatisCoords::WorldMaxX - SatisCoords::WorldMinX;
	const float RangeY = SatisCoords::WorldMaxY - SatisCoords::WorldMinY;

	const float U = FMath::Clamp((WorldLocation.X - SatisCoords::WorldMinX) / RangeX, 0.0f, 1.0f);
	const float V = FMath::Clamp((WorldLocation.Y - SatisCoords::WorldMinY) / RangeY, 0.0f, 1.0f);

	return FVector2D(U, V);
}

FVector USatisToolsDataSubsystem::NormalizedUVToWorld(const FVector2D& UV, float DefaultZ)
{
	const float RangeX = SatisCoords::WorldMaxX - SatisCoords::WorldMinX;
	const float RangeY = SatisCoords::WorldMaxY - SatisCoords::WorldMinY;

	const float X = SatisCoords::WorldMinX + (UV.X * RangeX);
	const float Y = SatisCoords::WorldMinY + (UV.Y * RangeY);

	return FVector(X, Y, DefaultZ);
}

bool USatisToolsDataSubsystem::IsPointInsidePolygon(const FVector2D& Point, const TArray<FVector2D>& Polygon)
{
	const int32 NumVerts = Polygon.Num();
	if (NumVerts < 3)
	{
		return false;
	}

	bool bInside = false;
	for (int32 i = 0, j = NumVerts - 1; i < NumVerts; j = i++)
	{
		const FVector2D& Vi = Polygon[i];
		const FVector2D& Vj = Polygon[j];

		if (((Vi.Y > Point.Y) != (Vj.Y > Point.Y)) &&
			(Point.X < (Vj.X - Vi.X) * (Point.Y - Vi.Y) / (Vj.Y - Vi.Y) + Vi.X))
		{
			bInside = !bInside;
		}
	}
	return bInside;
}

void USatisToolsDataSubsystem::RefreshWorldData()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	CollectResourceNodes();
	CollectDropPods();
	CollectBuildables();

	OnDataRefreshed.Broadcast();
}

void USatisToolsDataSubsystem::CollectResourceNodes()
{
	CachedResourceNodes.Reset();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AFGResourceNode> It(World); It; ++It)
	{
		AFGResourceNode* Node = *It;
		if (!Node)
		{
			continue;
		}

		FSatisResourceNodeData NodeData;
		NodeData.Location = Node->GetActorLocation();

		TSubclassOf<UFGResourceDescriptor> ResClass = Node->GetResourceClass();
		if (ResClass)
		{
			NodeData.ResourceName = UFGItemDescriptor::GetItemName(ResClass).ToString();
		}

		switch (Node->GetResourcePurity())
		{
		case EResourcePurity::RP_Inpure:
			NodeData.Purity = ESatisNodePurity::Impure;
			break;
		case EResourcePurity::RP_Pure:
			NodeData.Purity = ESatisNodePurity::Pure;
			break;
		case EResourcePurity::RP_Normal:
		default:
			NodeData.Purity = ESatisNodePurity::Normal;
			break;
		}

		NodeData.bIsOccupied = !Node->CanPlaceResourceExtractor();
		CachedResourceNodes.Add(NodeData);
	}
}

void USatisToolsDataSubsystem::CollectDropPods()
{
	CachedDropPods.Reset();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AFGDropPod> It(World); It; ++It)
	{
		AFGDropPod* Pod = *It;
		if (!Pod)
		{
			continue;
		}

		FSatisDropPodData PodData;
		PodData.Location = Pod->GetActorLocation();
		PodData.bHasBeenOpened = Pod->HasBeenOpened();
		PodData.RequiredPower = Pod->GetPowerConsumption();

		TSubclassOf<UFGItemDescriptor> ReqItem = Pod->GetRepairItemRule();
		if (ReqItem)
		{
			PodData.RequiredItem = UFGItemDescriptor::GetItemName(ReqItem).ToString();
		}

		CachedDropPods.Add(PodData);
	}
}

void USatisToolsDataSubsystem::CollectBuildables()
{
	CachedBuildings.Reset();
	CachedConveyors.Reset();
	CachedPipelines.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(World);
	if (!BuildableSubsystem)
	{
		return;
	}

	const TArray<AFGBuildable*>& AllBuildables = BuildableSubsystem->GetAllBuildablesRef();
	for (AFGBuildable* Buildable : AllBuildables)
	{
		if (!IsValid(Buildable))
		{
			continue;
		}

		// Conveyor Belts
		if (AFGBuildableConveyorBelt* Belt = Cast<AFGBuildableConveyorBelt>(Buildable))
		{
			FSatisSplineData ConveyorData;
			USplineComponent* SplineComp = Belt->GetSplineComponent();
			if (SplineComp)
			{
				const int32 NumPoints = SplineComp->GetNumberOfSplinePoints();
				for (int32 i = 0; i < NumPoints; ++i)
				{
					ConveyorData.WorldPoints.Add(SplineComp->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
				}
			}

			ConveyorData.Tier = 1;
			ConveyorData.Color = FLinearColor(0.2f, 0.7f, 1.0f, 1.0f);
			CachedConveyors.Add(ConveyorData);
			continue;
		}

		// Pipelines
		if (AFGBuildablePipeline* Pipe = Cast<AFGBuildablePipeline>(Buildable))
		{
			FSatisSplineData PipeData;
			USplineComponent* SplineComp = Pipe->GetSplineComponent();
			if (SplineComp)
			{
				const int32 NumPoints = SplineComp->GetNumberOfSplinePoints();
				for (int32 i = 0; i < NumPoints; ++i)
				{
					PipeData.WorldPoints.Add(SplineComp->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
				}
			}

			PipeData.Tier = 1;
			PipeData.Color = FLinearColor(1.0f, 0.5f, 0.1f, 1.0f);
			CachedPipelines.Add(PipeData);
			continue;
		}

		// Resource Extractors (Miners, Oil, Water)
		if (AFGBuildableResourceExtractor* Extractor = Cast<AFGBuildableResourceExtractor>(Buildable))
		{
			FSatisBuildingData BuildingData;
			BuildingData.Location = Extractor->GetActorLocation();
			BuildingData.BuildingName = Extractor->mDisplayName.ToString();
			BuildingData.PowerConsumptionMW = Extractor->GetPowerConsumption();
			BuildingData.Potential = Extractor->GetCurrentPotential();

			TSubclassOf<UFGResourceDescriptor> ResClass = Extractor->GetResourceClass();
			if (ResClass)
			{
				FSatisItemRate OutRate;
				OutRate.ItemName = UFGItemDescriptor::GetItemName(ResClass).ToString();
				OutRate.ItemsPerMinute = Extractor->GetExtractionPerMinute();
				BuildingData.OutputRates.Add(OutRate);
			}

			BuildingData.Status = Extractor->IsProducing() ? ESatisBuildingStatus::Producing : ESatisBuildingStatus::Idle;
			CachedBuildings.Add(BuildingData);
			continue;
		}

		// Manufacturers / Factories (Constructors, Assemblers, Refineries, etc.)
		if (AFGBuildableManufacturer* Manufacturer = Cast<AFGBuildableManufacturer>(Buildable))
		{
			FSatisBuildingData BuildingData;
			BuildingData.Location = Manufacturer->GetActorLocation();
			BuildingData.BuildingName = Manufacturer->mDisplayName.ToString();
			BuildingData.PowerConsumptionMW = Manufacturer->GetPowerConsumption();
			BuildingData.Potential = Manufacturer->GetCurrentPotential();

			TSubclassOf<UFGRecipe> CurrentRecipe = Manufacturer->GetCurrentRecipe();
			if (CurrentRecipe)
			{
				BuildingData.RecipeName = UFGRecipe::GetRecipeName(CurrentRecipe).ToString();
				const float CycleDuration = Manufacturer->GetProductionCycleTime();

				if (CycleDuration > 0.0f)
				{
					const float CyclesPerMin = 60.0f / CycleDuration;

					for (const FItemAmount& InItem : UFGRecipe::GetIngredients(World, CurrentRecipe))
					{
						if (InItem.ItemClass)
						{
							FSatisItemRate InRate;
							InRate.ItemName = UFGItemDescriptor::GetItemName(InItem.ItemClass).ToString();
							InRate.ItemsPerMinute = InItem.Amount * CyclesPerMin;
							BuildingData.InputRates.Add(InRate);
						}
					}

					for (const FItemAmount& OutItem : UFGRecipe::GetProducts(CurrentRecipe))
					{
						if (OutItem.ItemClass)
						{
							FSatisItemRate OutRate;
							OutRate.ItemName = UFGItemDescriptor::GetItemName(OutItem.ItemClass).ToString();
							OutRate.ItemsPerMinute = OutItem.Amount * CyclesPerMin;
							BuildingData.OutputRates.Add(OutRate);
						}
					}
				}
			}

			if (Manufacturer->IsProducing())
			{
				BuildingData.Status = ESatisBuildingStatus::Producing;
			}
			else if (Manufacturer->HasPower())
			{
				BuildingData.Status = ESatisBuildingStatus::Starved;
			}
			else
			{
				BuildingData.Status = ESatisBuildingStatus::NoPower;
			}

			CachedBuildings.Add(BuildingData);
			continue;
		}
	}
}

FSatisZoneAnalytics USatisToolsDataSubsystem::AnalyzeZonePolygon(const TArray<FVector2D>& PolygonPointsWorld) const
{
	FSatisZoneAnalytics Analytics;

	if (PolygonPointsWorld.Num() < 3)
	{
		return Analytics;
	}

	float TotalEfficiencySum = 0.0f;

	for (const FSatisBuildingData& Building : CachedBuildings)
	{
		const FVector2D BuildingPos2D(Building.Location.X, Building.Location.Y);
		if (!IsPointInsidePolygon(BuildingPos2D, PolygonPointsWorld))
		{
			continue;
		}

		Analytics.TotalBuildings++;
		Analytics.TotalPowerConsumptionMW += Building.PowerConsumptionMW;

		TotalEfficiencySum += (Building.Status == ESatisBuildingStatus::Producing) ? 100.0f : 0.0f;

		for (const FSatisItemRate& InRate : Building.InputRates)
		{
			float& Current = Analytics.ConsumptionRates.FindOrAdd(InRate.ItemName);
			Current += InRate.ItemsPerMinute;
		}

		for (const FSatisItemRate& OutRate : Building.OutputRates)
		{
			float& Current = Analytics.ProductionRates.FindOrAdd(OutRate.ItemName);
			Current += OutRate.ItemsPerMinute;
		}
	}

	if (Analytics.TotalBuildings > 0)
	{
		Analytics.AverageEfficiencyPercent = TotalEfficiencySum / static_cast<float>(Analytics.TotalBuildings);
	}

	// Calculate Net Rates (Production - Consumption)
	TSet<FString> AllItems;
	for (const auto& Pair : Analytics.ProductionRates)
	{
		AllItems.Add(Pair.Key);
	}
	for (const auto& Pair : Analytics.ConsumptionRates)
	{
		AllItems.Add(Pair.Key);
	}

	for (const FString& Item : AllItems)
	{
		const float Prod = Analytics.ProductionRates.FindRef(Item);
		const float Cons = Analytics.ConsumptionRates.FindRef(Item);
		Analytics.NetRates.Add(Item, Prod - Cons);
	}

	return Analytics;
}

FSatisZoneAnalytics USatisToolsDataSubsystem::AnalyzeZoneBox(const FVector2D& BoxMinWorld, const FVector2D& BoxMaxWorld) const
{
	TArray<FVector2D> BoxPolygon;
	BoxPolygon.Add(FVector2D(BoxMinWorld.X, BoxMinWorld.Y));
	BoxPolygon.Add(FVector2D(BoxMaxWorld.X, BoxMinWorld.Y));
	BoxPolygon.Add(FVector2D(BoxMaxWorld.X, BoxMaxWorld.Y));
	BoxPolygon.Add(FVector2D(BoxMinWorld.X, BoxMaxWorld.Y));

	return AnalyzeZonePolygon(BoxPolygon);
}

void USatisToolsDataSubsystem::SaveZone(const FSatisZoneData& NewZone)
{
	int32 ExistingIndex = SavedZones.IndexOfByPredicate([&](const FSatisZoneData& Z) {
		return Z.ZoneId == NewZone.ZoneId;
	});

	if (ExistingIndex != INDEX_NONE)
	{
		SavedZones[ExistingIndex] = NewZone;
	}
	else
	{
		SavedZones.Add(NewZone);
	}
}

void USatisToolsDataSubsystem::DeleteZone(const FGuid& ZoneId)
{
	SavedZones.RemoveAll([&](const FSatisZoneData& Z) {
		return Z.ZoneId == ZoneId;
	});
}

TArray<FSatisZoneData> USatisToolsDataSubsystem::GetSavedZones() const
{
	return SavedZones;
}
