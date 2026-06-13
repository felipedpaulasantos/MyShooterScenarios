// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTTaskNode.h"

#include "BTTask_PeekFromCover.generated.h"

/**
 * BT Task: Peek From Cover (MYST)
 *
 * Strafes the AI laterally along the cover surface toward the target enemy
 * until the blueprint cover component grants the tag Status.Cover.CanLean, which
 * signals that the character has reached the cover edge and can lean out.
 *
 * ── Execution Flow ─────────────────────────────────────────────────────────
 *  1. Verifies the AI is in cover mode (LyraCharacterMovementComponent).
 *  2. Projects the direction to the target enemy onto the CoverSurfaceTangent
 *     and caches it as the strafe direction (computed once — no re-evaluation).
 *  3. Each tick: drives AddMovementInput along the cached strafe direction.
 *  4. As soon as the ASC carries the tag Status.Cover.CanLean the task stops
 *     movement and returns Success.  A timeout guards against getting stuck.
 *
 * ── Cleanup on Abort ───────────────────────────────────────────────────────
 *  - Stops issuing movement input (natural — no explicit stop needed).
 *
 * ── Setup ──────────────────────────────────────────────────────────────────
 *  - Wire MoveGoalKey to the Vector BB key holding the cover-edge position.
 *  - Tune StrafeSpeedScale and StrafeTimeout as needed.
 *  - ADS is handled separately — this task does NOT activate it.
 */
UCLASS(meta = (DisplayName = "Peek From Cover (MYST)"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UBTTask_PeekFromCover : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_PeekFromCover(const FObjectInitializer& ObjectInitializer);

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:

	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// ── Blackboard Keys ─────────────────────────────────────────────────────

	/**
	 * Vector key holding the world-space position of the cover edge to strafe toward.
	 * Replaces the former TargetEnemyKey — decouple move goal from the enemy actor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FBlackboardKeySelector MoveGoalKey;

	// ── Tuning Parameters ───────────────────────────────────────────────────

	/**
	 * Lateral movement speed scale (0–1) applied to AddMovementInput during strafe.
	 * Defaults to 1.0 for immediate, full-speed movement toward the cover edge.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Peek",
		meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float StrafeSpeedScale = 1.0f;

	/**
	 * Maximum time (s) to strafe before giving up.
	 * Guards against the AI never reaching the cover edge.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Peek",
		meta = (ClampMin = "0.5", UIMin = "1.0", UIMax = "15.0", ForceUnits = "s"))
	float StrafeTimeout = 5.0f;

private:

	enum class EPeekState : uint8
	{
		Strafing,   // Moving along cover toward the enemy
		Complete    // Status.Cover.CanLean received — finished
	};

	/** Per-task-instance state. */
	struct FBTPeekFromCoverMemory
	{
		EPeekState State = EPeekState::Strafing;

		/** Committed strafe direction (cached once in ExecuteTask, never recalculated). */
		FVector CachedStrafeDir = FVector::ZeroVector;

		/** World time when ExecuteTask was called (for timeout). */
		float StartTime = -1.0f;
	};
};

