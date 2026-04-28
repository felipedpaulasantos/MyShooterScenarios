// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "DataProviders/AIDataProvider.h"
#include "EnvQueryGenerator_CachedNavEdges.generated.h"

/**
 * EQS Generator that pulls pre-computed cover points from ULyraCoverSubsystem.
 * All candidate locations are placed at NavMesh wall boundaries, so the AI
 * always moves to a position that is flush against cover geometry.
 *
 * Usage: replace the Grid/Cone generator in EQS_FindCover with this generator.
 * Call ULyraCoverSubsystem::StartCachingArea() for any level region that
 * contains cover before running this query, or enable auto-caching via the subsystem.
 */
UCLASS(meta = (DisplayName = "Cached Cover Points (Wall Edges)"))
class LYRAGAME_API UEnvQueryGenerator_CachedNavEdges : public UEnvQueryGenerator
{
	GENERATED_BODY()

public:
	UEnvQueryGenerator_CachedNavEdges(const FObjectInitializer& ObjectInitializer);

	/** Context used as the center of the search radius (usually the Querier / AI pawn). */
	UPROPERTY(EditDefaultsOnly, Category = "Generator")
	TSubclassOf<class UEnvQueryContext> SearchCenter;

	/** Radius around SearchCenter to gather cached cover points from the subsystem. */
	UPROPERTY(EditDefaultsOnly, Category = "Generator")
	FAIDataProviderFloatValue SearchRadius;

	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};

