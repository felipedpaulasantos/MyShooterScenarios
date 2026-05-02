// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTService.h"

#include "BTService_PeekWillingness.generated.h"

/**
 * BT Service: Peek Willingness (MYST)
 *
 * Aggregates several Blackboard signals from UBTService_UpdateCombatState and
 * UBTService_AIStateObserver into a single normalized [0, 1] float written to
 * PeekWillingnessScoreKey.  A BT Blackboard Decorator on that float
 * (>= PeekThreshold, AbortSelf+LowerPriority) gates the Peek & Shoot branch.
 *
 * ── Score formula (all weights EditAnywhere) ────────────────────────────────
 *
 *  Offensive bonuses (player is vulnerable)
 *    + TargetIsReloadingBonus     (default +0.40)  ← target ASC has reload tag
 *    + TargetExposedBonus         (default +0.25)  ← target NOT in cover
 *    + TargetLowHealthBonus       (default +0.20)  ← target health < threshold
 *    + TargetDistractedBonus      (default +0.15)  ← target engaging an ally
 *    + TargetOverduePeekBonus     (default +0.10)  ← target in cover > N seconds
 *
 *  Defensive penalties (AI is unsafe)
 *    - OutOfAmmoPenalty           (default -0.60)  ← AI is reloading
 *    - RecentlyHitPenalty         (default -0.40)  ← AI took damage recently
 *    - LowSelfHealthPenalty       (default -0.30)  ← AI health < threshold
 *    - IsolatedPenalty            (default -0.20)  ← AI has no allies nearby
 *
 *  Result = Clamp(Sum, 0, 1)
 *
 * ── Mandatory input keys (set to matching BB key names in the BT editor) ────
 *  ┌──────────────────────────────────────┬─────────────────────────────────────────┐
 *  │ Key selector (this service)          │ Written by                              │
 *  ├──────────────────────────────────────┼─────────────────────────────────────────┤
 *  │ HealthPercentageKey                  │ BTService_UpdateCombatState             │
 *  │ IsIsolatedKey                        │ BTService_UpdateCombatState             │
 *  │ HasTargetInCoverKey                  │ BTService_UpdateCombatState             │
 *  │ TargetTimeInCoverKey                 │ BTService_UpdateCombatState             │
 *  │ IsTargetEngagingOtherKey             │ BTService_UpdateCombatState             │
 *  │ OutOfAmmoKey                         │ BTService_AIStateObserver               │
 *  │ HasTakenDamageRecentlyKey            │ BTService_AIStateObserver               │
 *  │ TargetIsReloadingKey                 │ BTService_AIStateObserver               │
 *  │ TargetIsLowHealthKey                 │ BTService_AIStateObserver               │
 *  └──────────────────────────────────────┴─────────────────────────────────────────┘
 *
 * ── Output key ───────────────────────────────────────────────────────────────
 *  PeekWillingnessScoreKey (Float)  — set this to a Float BB key in the editor.
 *
 * ── Blueprint subclassing ────────────────────────────────────────────────────
 * This service is Blueprintable.  Subclass it to implement:
 *   - OnPeekReadinessChanged(bIsNowReady)  — fires when score crosses PeekThreshold
 *
 * ── Attach all three services to the same Selector root ─────────────────────
 * Selector [Combat Root]
 *   [Service] BTService_UpdateCombatState
 *   [Service] BTService_AIStateObserver
 *   [Service] BTService_PeekWillingness       ← this one; tick last
 */
UCLASS(Blueprintable, meta = (DisplayName = "Peek Willingness (MYST)"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UBTService_PeekWillingness : public UBTService
{
	GENERATED_BODY()

public:

	UBTService_PeekWillingness(const FObjectInitializer& ObjectInitializer);

	virtual FString GetStaticDescription() const override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;

	/**
	 * Placement-new constructor for FPeekWillingnessMemory.
	 * NOTE: if FPeekWillingnessMemory ever gains non-trivial members (e.g.
	 * FTimerHandle, TArray, TWeakObjectPtr), add a matching CleanupMemory
	 * override that calls Memory->~FPeekWillingnessMemory() to avoid leaks.
	 * (CleanupMemory is intentionally absent while the struct is all-POD.)
	 */
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;

protected:

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// ── Input keys (read-only — written by the other two services) ───────────

	/** Set to the same BB key as in BTService_UpdateCombatState. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Keys|Input")
	FBlackboardKeySelector HealthPercentageKey;

	/** Set to the same BB key as in BTService_UpdateCombatState. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Keys|Input")
	FBlackboardKeySelector IsIsolatedKey;

	/** Set to the same BB key as in BTService_UpdateCombatState. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Keys|Input")
	FBlackboardKeySelector HasTargetInCoverKey;

	/** Set to the same BB key as in BTService_UpdateCombatState. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Keys|Input")
	FBlackboardKeySelector TargetTimeInCoverKey;

	/** Set to the same BB key as in BTService_UpdateCombatState. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Keys|Input")
	FBlackboardKeySelector IsTargetEngagingOtherKey;

	/** Set to the same BB key as in BTService_AIStateObserver. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Keys|Input")
	FBlackboardKeySelector OutOfAmmoKey;

	/** Set to the same BB key as in BTService_AIStateObserver. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Keys|Input")
	FBlackboardKeySelector HasTakenDamageRecentlyKey;

	/** Set to the same BB key as in BTService_AIStateObserver. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Keys|Input")
	FBlackboardKeySelector TargetIsReloadingKey;

	/** Set to the same BB key as in BTService_AIStateObserver. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Keys|Input")
	FBlackboardKeySelector TargetIsLowHealthKey;

	// ── Output key ────────────────────────────────────────────────────────────

	/**
	 * Float BB key to write the computed score into [0, 1].
	 * Point a BB Decorator (>= PeekThreshold, AbortSelf+LowerPriority) at this
	 * key to gate the Peek & Shoot sequence.
	 * Suggested key name: PeekWillingnessScore
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Keys|Output")
	FBlackboardKeySelector PeekWillingnessScoreKey;

	// ── Offensive bonus weights ────────────────────────────────────────────────

	/** Added when the target's ASC has the reload tag active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Weights|Offensive",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TargetIsReloadingBonus = 0.40f;

	/** Added when the target is NOT in cover. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Weights|Offensive",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TargetExposedBonus = 0.25f;

	/** Added when the target's health is below the low-health threshold (set in BTService_AIStateObserver). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Weights|Offensive",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TargetLowHealthBonus = 0.20f;

	/** Added when the target is engaging a different AI (not looking at us). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Weights|Offensive",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TargetDistractedBonus = 0.15f;

	/**
	 * Added when the target has been in cover for longer than
	 * TargetOverduePeekTime (punishes permanent camping).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Weights|Offensive",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TargetOverduePeekBonus = 0.10f;

	/**
	 * Minimum seconds the target must spend in cover before
	 * TargetOverduePeekBonus is applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Weights|Offensive",
		meta = (ClampMin = "0.0", ForceUnits = "s"))
	float TargetOverduePeekTime = 5.0f;

	// ── Defensive penalty weights ─────────────────────────────────────────────

	/** Subtracted when the AI is currently reloading / out of ammo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Weights|Defensive",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OutOfAmmoPenalty = 0.60f;

	/** Subtracted when the AI has taken damage recently. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Weights|Defensive",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RecentlyHitPenalty = 0.40f;

	/** Subtracted when the AI's own health is below LowSelfHealthThreshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Weights|Defensive",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowSelfHealthPenalty = 0.30f;

	/** Subtracted when the AI has no allies within isolation radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Weights|Defensive",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IsolatedPenalty = 0.20f;

	/**
	 * Self health fraction below which LowSelfHealthPenalty is applied.
	 * Should match your definition of "low health" for this AI archetype.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Weights|Defensive",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowSelfHealthThreshold = 0.30f;

	// ── Threshold ─────────────────────────────────────────────────────────────

	/**
	 * Score above which the AI considers itself ready to peek.
	 * Wire a BB Float Decorator (>= this value) on the Peek Sequence and set it
	 * to AbortSelf+LowerPriority so the BT reacts instantly.
	 * Also used to fire the OnPeekReadinessChanged Blueprint event.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Threshold",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PeekThreshold = 0.35f;

	/**
	 * Minimum score change required to actually write PeekWillingnessScoreKey to
	 * the Blackboard.  Without this dead-band, micro-changes in input floats
	 * (HealthPct, TargetCoverTime) produce a different accumulated score every tick,
	 * firing BB change notifications that trigger Observer-Abort decorators and
	 * restart active tasks (e.g. EQS_FindCover) before they can complete.
	 *
	 * Set to 0.0 to disable (writes every tick — matches legacy behavior).
	 * Default: 0.02 (2% change required).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Willingness|Threshold",
		meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float ScoreWriteDeadBand = 0.02f;

	// ── Blueprint hook ────────────────────────────────────────────────────────

	/**
	 * Called every time the score crosses PeekThreshold (in either direction).
	 * bIsNowReady == true  → score just rose above threshold (consider peeking).
	 * bIsNowReady == false → score just fell below threshold (return to cover).
	 *
	 * Note: the BT Decorator already handles the branch switching reactively.
	 * Use this event for secondary reactions (e.g., audio, animations).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Willingness|Events",
		meta = (DisplayName = "On Peek Readiness Changed"))
	void OnPeekReadinessChanged(bool bIsNowReady);

private:

	/** Per-node memory to detect threshold transitions for the BP hook. */
	struct FPeekWillingnessMemory
	{
		bool bWasReady = false;

		/** Last score value actually committed to the Blackboard (for dead-band check). */
		float LastWrittenScore = -1.f;

		FPeekWillingnessMemory() = default;
	};
};

