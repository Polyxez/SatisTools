#pragma once

#include "CoreMinimal.h"
#include "SatisToolsMapTypes.generated.h"

UENUM(BlueprintType)
enum class ESatisNodePurity : uint8
{
	Impure UMETA(DisplayName = "Impure"),
	Normal UMETA(DisplayName = "Normal"),
	Pure   UMETA(DisplayName = "Pure")
};

UENUM(BlueprintType)
enum class ESatisBuildingStatus : uint8
{
	Producing UMETA(DisplayName = "Producing"),
	Starved   UMETA(DisplayName = "Starved (No Input)"),
	Full      UMETA(DisplayName = "Full (Output Blocked)"),
	Idle      UMETA(DisplayName = "Idle / Standby"),
	NoPower   UMETA(DisplayName = "No Power")
};

USTRUCT(BlueprintType)
struct FSatisItemRate
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	FString ItemName;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	float ItemsPerMinute = 0.0f;
};

USTRUCT(BlueprintType)
struct FSatisResourceNodeData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	FString ResourceName;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	ESatisNodePurity Purity = ESatisNodePurity::Normal;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	bool bIsOccupied = false;
};

USTRUCT(BlueprintType)
struct FSatisDropPodData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	bool bHasBeenOpened = false;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	FString RequiredItem;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	float RequiredPower = 0.0f;
};

USTRUCT(BlueprintType)
struct FSatisBuildingData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	FString BuildingName;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	FString RecipeName;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	float Potential = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	float PowerConsumptionMW = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	ESatisBuildingStatus Status = ESatisBuildingStatus::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	TArray<FSatisItemRate> InputRates;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	TArray<FSatisItemRate> OutputRates;
};

USTRUCT(BlueprintType)
struct FSatisSplineData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	TArray<FVector> WorldPoints;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	uint8 Tier = 1;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	FLinearColor Color = FLinearColor::White;
};

USTRUCT(BlueprintType)
struct FSatisZoneData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "SatisTools")
	FGuid ZoneId;

	UPROPERTY(BlueprintReadWrite, Category = "SatisTools")
	FString ZoneName;

	UPROPERTY(BlueprintReadWrite, Category = "SatisTools")
	FLinearColor BorderColor = FLinearColor(0.1f, 0.6f, 1.0f, 0.8f);

	UPROPERTY(BlueprintReadWrite, Category = "SatisTools")
	TArray<FVector2D> PolygonPointsWorld;

	UPROPERTY(BlueprintReadWrite, Category = "SatisTools")
	bool bIsPinned = false;
};

USTRUCT(BlueprintType)
struct FSatisZoneAnalytics
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	int32 TotalBuildings = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	float TotalPowerConsumptionMW = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	float AverageEfficiencyPercent = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	TMap<FString, float> ProductionRates;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	TMap<FString, float> ConsumptionRates;

	UPROPERTY(BlueprintReadOnly, Category = "SatisTools")
	TMap<FString, float> NetRates;
};
