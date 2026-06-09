// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTService.h"

#include "BTService_CoverModeSync.generated.h"

/**
 * BT Service: Cover Mode Sync (MYST)
 *
 * Polls ULyraCharacterMovementComponent::IsInCoverMode() on every tick
 * and writes the result to a configurable Blackboard Bool key.
 *
 * This is the authoritative source for the IsInCoverMode BB key.
 * BTTask_EnterCover and BTTask_ExitCover no longer set it directly,
 * which eliminates the race-condition / feedback-loop that required
 * BTTask_EnterCover's CoverStabilizationDelay workaround.
 *
 * ── Usage ────────────────────────────────────────────────────────────────────
 *  1. Add this service to the same Selector (or root node) as your
 *     ENTER COVER and PEEK FROM COVER sequences.
 *  2. Wire IsInCoverModeKey to your Bool BB key (e.g., "IsInCoverMode").
 *  3. Remove any wiring of IsInCoverModeKey on BTTask_EnterCover / ExitCover
 *     (those properties have been removed).
 *
 * ── Tick Interval ────────────────────────────────────────────────────────────
 *  Default: 0.1 s interval, 0.05 s random deviation.
 *  Covers the transition within one or two frames; fast enough for decorator
 *  reactivity without per-frame overhead.
 *  Reduce to 0.0 if you need frame-perfect sync (e.g., during cinematic tests).
 */
UCLASS(meta = (DisplayName = "Cover Mode Sync (MYST)"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UBTService_CoverModeSync : public UBTService
{
	GENERATED_BODY()

public:

	UBTService_CoverModeSync(const FObjectInitializer& ObjectInitializer);

	virtual FString GetStaticDescription() const override;

protected:

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// ── Blackboard Key ───────────────────────────────────────────────────────

	/**
	 * Bool key to keep in sync with ULyraCharacterMovementComponent::IsInCoverMode().
	 * Suggested key name: IsInCoverMode
	 * Gate your PEEK and ENTER COVER BT decorator conditions on this key.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FBlackboardKeySelector IsInCoverModeKey;
};

