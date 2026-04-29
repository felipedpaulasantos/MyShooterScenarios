// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindPeekLocation.generated.h"

/**
 * BT Task: Finds a "peek" position beside the AI's claimed cover spot that has
 * a clear line of sight to a target actor, so the AI can step out and shoot.
 *
 * ── Algorithm ───────────────────────────────────────────────────────────────
 *  1. Computes a strafe axis perpendicular to the cover→target direction.
 *  2. Samples candidate positions along that axis at ±StepSize, ±2*StepSize …
 *     (alternating left/right, closest first).
 *  3. Optionally projects each candidate onto the NavMesh to ensure the AI
 *     can actually reach it (bRequireNavMesh).
 *  4. Line-traces from candidateEyePos to targetEyePos on TraceChannel.
 *  5. Writes the first unblocked candidate to PeekLocationKey.
 *
 * ── Returns ─────────────────────────────────────────────────────────────────
 *  Success  – PeekLocationKey is populated with a reachable, visible spot.
 *  Failure  – No suitable location found; AI should stay in cover.
 *
 * ── Typical BT peeking sub-tree ─────────────────────────────────────────────
 *
 *  Sequence [Peek & Shoot]
 *  ├─ Decorator: Blackboard (TargetActor is set)                   [Abort Self]
 *  ├─ Decorator: Blackboard (bNeedsReload == false)                [Abort Self]
 *  ├─ Decorator: Blackboard (bRecentlyHit == false)                [Abort Self]
 *  ├─ BTTask_FindPeekLocation
 *  ├─ MoveTo (PeekLocation)
 *  └─ [fire / aim task]
 *
 *  ── When bNeedsReload or bRecentlyHit flips true the decorators abort the
 *     sequence, and the parent BT node falls through to a "MoveTo CoverLocation"
 *     branch — no extra C++ needed for the return-to-cover path.
 */
UCLASS(meta = (DisplayName = "Find Peek Location (MYST)"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UBTTask_FindPeekLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_FindPeekLocation(const FObjectInitializer& ObjectInitializer);

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

	// ── Blackboard keys ─────────────────────────────────────────────────────

	/** Cover spot already claimed by this AI (FVector). */
	UPROPERTY(EditAnywhere, Category = "Peek|Keys")
	FBlackboardKeySelector CoverLocationKey;

	/** Actor this AI wants to shoot (used for LOS target position). */
	UPROPERTY(EditAnywhere, Category = "Peek|Keys")
	FBlackboardKeySelector TargetActorKey;

	/** Output: the chosen peek position (FVector). */
	UPROPERTY(EditAnywhere, Category = "Peek|Keys")
	FBlackboardKeySelector PeekLocationKey;

	// ── Sampling parameters ─────────────────────────────────────────────────

	/**
	 * Distance (cm) between sampled positions along the strafe axis.
	 * Tune to roughly the width of cover geometry.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Peek|Sampling",
		meta = (ClampMin = "10.0", ForceUnits = "cm"))
	float StepSize = 80.f;

	/**
	 * Maximum number of steps to try on each side (left + right).
	 * Total candidates checked = 2 * MaxSteps.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Peek|Sampling",
		meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxSteps = 3;

	/**
	 * Z offset above the cover/peek location used for eye-level LOS traces.
	 * Set this to your character's approximate eye height.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Peek|Sampling",
		meta = (ClampMin = "0.0", ForceUnits = "cm"))
	float EyeHeightOffset = 160.f;

	/** Trace channel for LOS checks. ECC_Visibility is usually correct. */
	UPROPERTY(EditDefaultsOnly, Category = "Peek|Sampling")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// ── NavMesh projection ───────────────────────────────────────────────────

	/**
	 * When true, each candidate is projected onto the NavMesh before the LOS
	 * check.  Candidates that fall off-nav are skipped so the AI won't get
	 * stuck trying to move somewhere unreachable.
	 * Requires NavigationSystem module (already linked in via AIModule).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Peek|Navigation")
	bool bRequireNavMesh = true;

	/**
	 * How far (cm) to search up/down from the candidate when projecting to
	 * the NavMesh.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Peek|Navigation",
		meta = (ClampMin = "1.0", ForceUnits = "cm", EditCondition = "bRequireNavMesh"))
	float NavProjectionExtentZ = 100.f;
};

