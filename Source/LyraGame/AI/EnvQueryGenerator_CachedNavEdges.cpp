// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/EnvQueryGenerator_CachedNavEdges.h"
#include "AI/LyraCoverSubsystem.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "Engine/World.h"

UEnvQueryGenerator_CachedNavEdges::UEnvQueryGenerator_CachedNavEdges(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SearchCenter = UEnvQueryContext_Querier::StaticClass();
	SearchRadius.DefaultValue = 1500.0f;
	ItemType = UEnvQueryItemType_Point::StaticClass();
}

void UEnvQueryGenerator_CachedNavEdges::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!QueryOwner)
	{
		return;
	}

	SearchRadius.BindData(QueryOwner, QueryInstance.QueryID);
	const float CurrentRadius = SearchRadius.GetValue();

	// Resolve the search center context
	TArray<FVector> ContextLocations;
	if (!QueryInstance.PrepareContext(SearchCenter, ContextLocations) || ContextLocations.IsEmpty())
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(QueryOwner, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	ULyraCoverSubsystem* CoverSubsystem = World->GetSubsystem<ULyraCoverSubsystem>();
	if (!CoverSubsystem)
	{
		return;
	}

	// Use the first context location (the AI pawn) as the search center
	const FVector& Center = ContextLocations[0];

	TArray<FLyraCoverPoint> NearbyPoints;
	CoverSubsystem->GetCoverPointsInRadius(Center, CurrentRadius, NearbyPoints);

	// Sort by distance so that if scores are tied the nearest point wins the tiebreak.
	// (The Distance EQS test handles proper scoring, but this guards against equal-score situations.)
	NearbyPoints.Sort([&Center](const FLyraCoverPoint& A, const FLyraCoverPoint& B)
	{
		return FVector::DistSquared(Center, A.Location) < FVector::DistSquared(Center, B.Location);
	});

	// Feed each cached wall-boundary point into EQS as a candidate
	for (const FLyraCoverPoint& CoverPoint : NearbyPoints)
	{
		QueryInstance.AddItemData<UEnvQueryItemType_Point>(CoverPoint.Location);
	}
}

FText UEnvQueryGenerator_CachedNavEdges::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Cached Cover Points (Wall Edges)"));
}

FText UEnvQueryGenerator_CachedNavEdges::GetDescriptionDetails() const
{
	return FText::FromString(FString::Printf(TEXT("Queries ULyraCoverSubsystem within radius %.0f from context."),
	                                         SearchRadius.DefaultValue));
}
