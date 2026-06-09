// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTTaskNode.h"

#include "BTTask_FindPeekLocation.generated.h"

/**
 * BT Task: Find Peek Location (MYST)
 *
 * Samples strafe positions from the AI's current cover location and performs line-of-sight
 * traces to the target actor. Writes the first position with clear LOS to the blackboard
 * as the peek location.
 *
 * ── Algorithm ──────────────────────────────────────────────────────────────
 *  1. StrafeAxis = CrossProduct(normalize2D(CoverLocation → TargetLocation), UpVector)
 *  2. For Step in [1 … MaxSteps], for Sign in [+1, −1]:
 *       Candidate = CoverLocation + StrafeAxis * (StepSize * Step * Sign)
 *       [Optional] Project Candidate onto NavMesh (bRequireNavMesh)
 *       LineTrace (eye height) from Candidate to Target
 *       if ClearLOS → write PeekLocation, return Success
 *  3. return Failure (AI stays in cover this tick; BT retries next frame)
 *
 * ── Setup ──────────────────────────────────────────────────────────────────
 *  - Place in a Peek Sequence after the AI is already in cover
 *  - Wire CoverLocationKey to the BB key holding the cover location
 *  - Wire TargetActorKey to the BB key holding the enemy/player
 *  - Wire PeekLocationKey to the output Vector BB key
 *  - Tune StepSize (default 80 cm) and MaxSteps (default 3) for cover geometry width
 */
UCLASS(meta = (DisplayName = "Find Peek Location (MYST)"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UBTTask_FindPeekLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_FindPeekLocation(const FObjectInitializer& ObjectInitializer);

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:

	// ── Blackboard Keys ─────────────────────────────────────────────────────

	/** Vector key holding the cover location the AI is currently at. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FBlackboardKeySelector CoverLocationKey;

	/** Object key holding the target actor to peek at (player/enemy). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** Vector key to write the computed peek location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FBlackboardKeySelector PeekLocationKey;

	// ── Tuning Parameters ───────────────────────────────────────────────────

	/**
	 * Distance (cm) between each strafe probe along the cover wall.
	 * Tune to cover geometry width — smaller steps = more precise, more traces.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Peek Search",
		meta = (ClampMin = "10.0", UIMin = "50.0", UIMax = "200.0"))
	float StepSize = 80.0f;

	/**
	 * Maximum number of steps to probe in each direction (left/right).
	 * Total candidates = 2 × MaxSteps.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Peek Search",
		meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxSteps = 3;

	/**
	 * Z-offset from the candidate location for the LOS trace start point.
	 * Should match character eye height.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Peek Search",
		meta = (ClampMin = "50.0", UIMin = "100.0", UIMax = "250.0"))
	float EyeHeightOffset = 160.0f;

	/** Collision channel for the LOS trace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Peek Search")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/**
	 * When true, candidate positions are projected onto the NavMesh before
	 * the LOS trace. Positions that fail projection are skipped.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Peek Search")
	bool bRequireNavMesh = true;

	/**
	 * Vertical search range for NavMesh projection (cm).
	 * Increase if cover surfaces are on steps/slopes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Peek Search",
		meta = (ClampMin = "10.0", UIMin = "50.0", UIMax = "500.0", EditCondition = "bRequireNavMesh"))
	float NavProjectionExtentZ = 100.0f;
};

