// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTTaskNode.h"

#include "BTTask_ExitCover.generated.h"

/**
 * BT Task: Exit Cover (MYST)
 *
 * Exits cover mode and moves the AI away from the wall by a specified retreat distance.
 *
 * ── Execution Flow ─────────────────────────────────────────────────────────
 *  1. Reads CoverSurfaceNormal from the AI's ULyraCharacterMovementComponent
 *  2. Calls CMC->ExitCoverMode() to restore MOVE_Walking
 *  3. Computes a retreat point: CurrentLocation + Normal * RetreatDistance
 *  4. Uses SimpleMoveToLocation to back away from the wall
 *  5. Releases the cover claim via UMYSTCoverClaimSubsystem
 *  6. Returns Success when the AI reaches the retreat point
 *
 * NOTE: The IsInCoverMode Blackboard key is now managed by BTService_CoverModeSync.
 * This task no longer clears it directly — the service detects the CMC state change
 * automatically on its next tick.
 *
 * ── Abort Cleanup ──────────────────────────────────────────────────────────
 *  - Safe to abort mid-retreat; cover mode is already exited and claim released
 *
 * ── Setup ──────────────────────────────────────────────────────────────────
 *  - Place in a BT branch triggered by exit conditions (e.g., low health, no targets)
 *  - Ensure BTService_CoverModeSync is attached to your BT root/selector
 */
UCLASS(meta = (DisplayName = "Exit Cover (MYST)"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UBTTask_ExitCover : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_ExitCover(const FObjectInitializer& ObjectInitializer);

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// ── Tuning Parameters ───────────────────────────────────────────────────

	/**
	 * Distance (cm) to move away from the wall when exiting cover.
	 * Direction is perpendicular to the wall surface (along CoverSurfaceNormal).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover",
		meta = (ClampMin = "50.0", UIMin = "100.0", UIMax = "500.0"))
	float RetreatDistance = 150.0f;

	/**
	 * Acceptance radius (cm) for the retreat move.
	 * AI is considered "clear of cover" when within this distance of the retreat point.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover",
		meta = (ClampMin = "10.0", UIMin = "30.0", UIMax = "200.0"))
	float AcceptanceRadius = 50.0f;

private:

	/** Per-task-instance state: retreat target location. */
	struct FBTExitCoverMemory
	{
		FVector RetreatLocation = FVector::ZeroVector;
		bool bRetreatStarted = false;
	};
};
