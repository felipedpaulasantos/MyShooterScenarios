// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTTaskNode.h"

#include "BTTask_EnterCover.generated.h"

/**
 * BT Task: Enter Cover (MYST)
 *
 * Atomic cover-entry task that claims a cover spot, moves the AI to it, traces
 * the wall surface, and calls ULyraCharacterMovementComponent::EnterCoverMode.
 *
 * ── Execution Flow ─────────────────────────────────────────────────────────
 *  1. Reads CoverLocation from blackboard (typically written by an EQS_FindCover query)
 *  2. Claims the spot via UMYSTCoverClaimSubsystem (prevents other AIs from using it)
 *  3. Moves the AI to the location using SimpleMoveToLocation
 *  4. On arrival, fires a line trace toward the wall to get a valid FHitResult
 *  5. Calls CMC->EnterCoverMode(WallHit, CoverMaxSpeed)
 *  6. Returns Success
 *
 * NOTE: The IsInCoverMode Blackboard key is now managed by BTService_CoverModeSync,
 * which polls CMC::IsInCoverMode() directly. Do NOT wire that key on this task.
 *
 * ── Abort / Failure Cleanup ────────────────────────────────────────────────
 *  - If aborted or the wall trace fails, releases the cover claim and exits cover mode
 *  - Safe to abort mid-move or after entering cover
 *
 * ── Setup ──────────────────────────────────────────────────────────────────
 *  - Place after an EQS query that writes CoverLocation
 *  - Wire CoverLocationKey to the Vector BB key holding the cover position
 *  - Tune CoverMaxSpeed to match desired lateral movement speed
 *  - Add BTService_CoverModeSync to your BT root/selector to sync IsInCoverMode automatically
 */
UCLASS(meta = (DisplayName = "Enter Cover (MYST)"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UBTTask_EnterCover : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_EnterCover(const FObjectInitializer& ObjectInitializer);

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:

	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// ── Blackboard Keys ─────────────────────────────────────────────────────

	/**
	 * Vector key holding the cover location to move to.
	 * Typically written by an EQS_FindCover query.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FBlackboardKeySelector CoverLocationKey;


	// ── Tuning Parameters ───────────────────────────────────────────────────

	/**
	 * Maximum lateral speed (cm/s) while moving along cover.
	 * Passed to ULyraCharacterMovementComponent::EnterCoverMode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover",
		meta = (ClampMin = "50.0", UIMin = "100.0", UIMax = "600.0"))
	float CoverMaxSpeed = 250.0f;

	/**
	 * Acceptance radius (cm) for the SimpleMoveToLocation call.
	 * AI is considered "arrived" when within this distance of the cover location.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover",
		meta = (ClampMin = "10.0", UIMin = "30.0", UIMax = "200.0"))
	float AcceptanceRadius = 50.0f;

	/**
	 * Claim radius (cm) passed to UMYSTCoverClaimSubsystem::ClaimSpot.
	 * Other AIs within this radius of the cover location will see it as occupied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover",
		meta = (ClampMin = "50.0", UIMin = "100.0", UIMax = "300.0"))
	float ClaimRadius = 150.0f;

	/**
	 * Length of the wall-detection trace (cm).
	 * Increased values handle wider cover-entry distances.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover",
		meta = (ClampMin = "50.0", UIMin = "100.0", UIMax = "500.0"))
	float WallTraceDistance = 200.0f;

	/** Collision channel for the wall-detection trace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;


	/**
	 * Tag name that must be present on the hit actor or component to be considered valid cover.
	 * Prevents AIs from entering "cover mode" on the ground, non-cover walls, etc.
	 * Set this tag on your cover meshes in the editor (Actor Tags or Component Tags).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	FName RequiredCoverTag = FName("Cover");

private:

	/** Per-task-instance state: has the spot been claimed? */
	struct FBTEnterCoverMemory
	{
		bool bSpotClaimed = false;
		FVector ClaimedLocation = FVector::ZeroVector;
		bool bCoverModeEntered = false;
	};
};


