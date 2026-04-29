// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTService.h"
#include "GameplayTagContainer.h"

#include "BTService_AIStateObserver.generated.h"

/**
 * BT Service: AI State Observer (MYST)
 *
 * Ticks at a configurable interval and writes the following Blackboard keys:
 *
 *  ┌──────────────────────────────┬──────────────────────────────────────────────────────────┐
 *  │ Blackboard Key (suggested)   │ What it tracks                                           │
 *  ├──────────────────────────────┼──────────────────────────────────────────────────────────┤
 *  │ OutOfAmmo          (Bool)    │ AI ASC has ReloadTag active (e.g. Event.Movement.Reload)  │
 *  │ HasTakenDamageRecently(Bool) │ AI health fell by ≥ DamageReactionThreshold this tick;   │
 *  │                              │ stays true for DamageCooldown seconds, then resets.      │
 *  │ TargetIsReloading  (Bool)    │ Target ASC has ReloadTag active                          │
 *  │ TargetIsLowHealth  (Bool)    │ Target normalized health < LowHealthThreshold             │
 *  └──────────────────────────────┴──────────────────────────────────────────────────────────┘
 *
 * All health tracking is done via per-node instance memory — no delegates or
 * extra components are required.
 *
 * ── Blueprint subclassing ───────────────────────────────────────────────────
 * This service is Blueprintable.  Subclass it in Blueprint and implement:
 *   - OnAIReloadStateChanged(bIsNowReloading)   — fires each transition
 *   - OnDamageDetected(HealthLostFraction)       — fires when a hit is detected
 *
 * ── Typical interval ────────────────────────────────────────────────────────
 * Default Interval = 0.15s / RandomDeviation = 0.05s.
 * Lower to ≤ 0.1s for very responsive HasTakenDamageRecently behaviour.
 */
UCLASS(Blueprintable, meta = (DisplayName = "AI State Observer (MYST)"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UBTService_AIStateObserver : public UBTService
{
	GENERATED_BODY()

public:

	UBTService_AIStateObserver(const FObjectInitializer& ObjectInitializer);

	virtual FString GetStaticDescription() const override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;

protected:

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// ── Blackboard Keys ────────────────────────────────────────────────────

	/**
	 * Bool key that is true while the AI is actively reloading.
	 * Suggested key name: OutOfAmmo
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Keys|AI")
	FBlackboardKeySelector OutOfAmmoKey;

	/**
	 * Bool key that is true for DamageCooldown seconds after the AI takes a
	 * significant hit.
	 * Suggested key name: HasTakenDamageRecently
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Keys|AI")
	FBlackboardKeySelector HasTakenDamageRecentlyKey;

	/**
	 * Object key holding the current enemy actor (used to resolve target ASC
	 * and health).
	 * Suggested key name: TargetEnemy
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Keys|Target")
	FBlackboardKeySelector TargetEnemyKey;

	/**
	 * Bool key that is true while the target actor's ASC has an active
	 * reload tag.
	 * Suggested key name: TargetIsReloading
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Keys|Target")
	FBlackboardKeySelector TargetIsReloadingKey;

	/**
	 * Bool key that is true when the target's normalized health is below
	 * LowHealthThreshold.
	 * Suggested key name: TargetIsLowHealth
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Keys|Target")
	FBlackboardKeySelector TargetIsLowHealthKey;

	// ── Tuning ─────────────────────────────────────────────────────────────

	/**
	 * Gameplay tag checked against the AI's (and target's) ASC to detect an
	 * active reload.  Matches the ActivationOwnedTags on the reload GA.
	 * Default: Event.Movement.Reload
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Settings")
	FGameplayTag ReloadTag;

	/**
	 * Minimum fraction of MaxHealth lost in a single service tick to count as
	 * a "significant hit".  Prevents tiny DoT pulses from triggering the flag.
	 * Range: 0 – 1  (default 0.04 = 4 % of MaxHealth).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Settings",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageReactionThreshold = 0.04f;

	/**
	 * How many seconds HasTakenDamageRecently remains true after a qualifying
	 * hit.  Tracked entirely via instance memory.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Settings",
		meta = (ClampMin = "0.1", ForceUnits = "s"))
	float DamageCooldown = 4.f;

	/**
	 * Normalized health value (0 – 1) below which the target is flagged as
	 * "low health" in the TargetIsLowHealthKey.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Settings",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthThreshold = 0.35f;

	// ── Blueprint Event Hooks ───────────────────────────────────────────────

	/**
	 * Called every time the AI's reload state transitions.
	 * bIsNowReloading == true  → AI started reloading.
	 * bIsNowReloading == false → AI finished / interrupted reload.
	 * Override in Blueprint to trigger barks, animation reactions, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Observer|Events",
		meta = (DisplayName = "On AI Reload State Changed"))
	void OnAIReloadStateChanged(bool bIsNowReloading);

	/**
	 * Called on the tick that a hit above DamageReactionThreshold is detected.
	 * HealthLostFraction is the normalized fraction lost this tick (0–1).
	 * Override in Blueprint for flinch reactions, VO, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Observer|Events",
		meta = (DisplayName = "On AI Damage Detected"))
	void OnDamageDetected(float HealthLostFraction);

private:

	/** Per-node instance memory — avoids any heap allocations or delegates. */
	struct FAIStateObserverMemory
	{
		/** Normalized health sampled last tick (to compute delta). */
		float LastKnownHealthPct = 1.f;

		/** Seconds elapsed since the last qualifying hit. */
		float TimeSinceLastHit = 0.f;

		/** Cached reload state for transition detection. */
		bool bWasReloading = false;

		FAIStateObserverMemory() = default;
	};
};

