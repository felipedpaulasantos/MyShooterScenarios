// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "EnvironmentQuery/EnvQueryContext.h"
#include "MYSTEnvQueryContext_CoverLocation.generated.h"

/**
 * EQS Context: provides the AI's current cover position as a single world point.
 *
 * Reads a named Vector key from the querier pawn's AIController Blackboard.
 * Use this as both the generator centre and the distance-scoring reference in
 * EQS_FindPeekLocation so the query always samples around the actual cover spot
 * and scores by closeness to it.
 *
 * Setup in EQS asset:
 *   Generator  : Donut  –  Center Context = MYSTEnvQueryContext_CoverLocation
 *                           Inner Radius  ~40 cm   (skip the cover itself)
 *                           Outer Radius  ~200 cm  (maximum peek distance)
 *                           Rings = 3,  Slices = 16
 *   Test 1     : MYST Peek LOS to Target  (Filter)
 *   Test 2     : Distance from MYSTEnvQueryContext_CoverLocation  (Score, Minimize)
 */
UCLASS(meta = (DisplayName = "MYST: Cover Location (Blackboard)"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMYSTEnvQueryContext_CoverLocation : public UEnvQueryContext
{
	GENERATED_BODY()

public:

	/**
	 * Name of the Vector Blackboard key that holds the cover position.
	 * Must match the key name set on BTTask_FindPeekLocation → CoverLocationKey.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "MYST")
	FName CoverLocationKeyName = FName("CoverLocation");

	// UEnvQueryContext interface
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance,
	                            FEnvQueryContextData& ContextData) const override;
};

