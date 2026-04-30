// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "EnvironmentQuery/EnvQueryTest.h"
#include "MYSTEnvQueryTest_PeekLOS.generated.h"

/**
 * EQS Test: filters candidate locations to those with clear BIDIRECTIONAL
 * line-of-sight to a target actor read from the querier's Blackboard.
 *
 * Why bidirectional?
 * ------------------
 * UE line traces that START inside a convex mesh return no hit (the engine
 * considers the start already past the shape).  A reverse trace — starting
 * from the open-air target eye — correctly detects candidates that are
 * embedded in or behind cover geometry, preventing the AI from "peeking"
 * into a wall.
 *
 * Typical EQS asset setup
 * -----------------------
 *   Generator  : Donut centred on MYSTEnvQueryContext_CoverLocation
 *   Test 1     : This test  (Filter, keep items with clear LOS)
 *   Test 2     : Distance from MYSTEnvQueryContext_CoverLocation (Score, Minimize)
 */
UCLASS(meta = (DisplayName = "MYST: Peek LOS to Target"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMYSTEnvQueryTest_PeekLOS : public UEnvQueryTest
{
	GENERATED_BODY()

public:

	UMYSTEnvQueryTest_PeekLOS(const FObjectInitializer& ObjectInitializer);

	// UEnvQueryTest interface
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

	/**
	 * Name of the Blackboard Object key that holds the actor to trace toward.
	 * Must match the key name set on the Run EQS Query BT node's TargetEnemyKey.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "MYST")
	FName TargetActorKeyName = FName("TargetEnemy");

	/**
	 * Collision channel used for both the forward and reverse LOS traces.
	 * Must match the channel used by EQS_FindCover so the same geometry that
	 * provides cover also blocks peek sight lines.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "MYST")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/**
	 * Height above the candidate floor position from which the LOS traces fire.
	 * Set this to match the approximate world-space height of the weapon barrel
	 * socket on your AI character mesh, NOT the eye height.
	 *
	 * Why this matters: the eye clears a cover corner cleanly at ~160 cm, but
	 * the gun barrel at ~120 cm still clips that same corner.  If you trace from
	 * barrel height the EQS will only accept a position where the barrel itself
	 * has a clear shot, eliminating the "AI shoots its own cover" problem.
	 *
	 * Typical values:
	 *   120 cm  →  shoulder / barrel height for a standing Lyra character
	 *   160 cm  →  eye height (old default; produces edge-clipping shots)
	 *    80 cm  →  crouched barrel height
	 *
	 * Set to 0 to fall back to deriving the height live from GetActorEyesViewPoint().
	 */
	UPROPERTY(EditDefaultsOnly, Category = "MYST", meta = (ClampMin = "0.0", ForceUnits = "cm"))
	float TraceHeightCm = 120.f;
};

