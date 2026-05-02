// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTService.h"
#include "GameplayTagContainer.h"

#include "BTService_AIStateObserver.generated.h"

class ULyraHealthComponent;

/**
 * BT Service: AI State Observer (MYST)
 *
 * Ticks at a configurable interval and writes the following Blackboard keys:
 *
 *  ┌──────────────────────────────┬──────────────────────────────────────────────────────────┐
 *  │ Blackboard Key (suggested)   │ What it tracks                                           │
 *  ├──────────────────────────────┼──────────────────────────────────────────────────────────┤
 *  │ OutOfAmmo          (Bool)    │ AI ASC has ReloadTag active (e.g. Event.Movement.Reload)  │
 *  │ HasTakenDamageRecently(Bool) │ Turns true on a qualifying hit; resets after              │
 *  │                              │ DamageCooldown seconds with no further significant hit.  │
 *  │ TargetIsReloading  (Bool)    │ Target ASC has ReloadTag active                          │
 *  │ TargetIsLowHealth  (Bool)    │ Target normalized health < LowHealthThreshold             │
 *  └──────────────────────────────┴──────────────────────────────────────────────────────────┘
 *
 * HasTakenDamageRecently is driven by a delegate bound to ULyraHealthComponent::OnHealthChanged
 * rather than tick-based health polling, so it fires exactly when damage lands and the cooldown
 * is measured in real world-time seconds.  bCreateNodeInstance = true gives each AI its own
 * UObject instance so per-AI state is stored as plain member variables.
 *
 * ── Blueprint subclassing ───────────────────────────────────────────────────
 * This service is Blueprintable.  Subclass it in Blueprint and implement:
 *   - OnAIReloadStateChanged(bIsNowReloading)   — fires each transition
 *   - OnDamageDetected(HealthLostFraction)       — fires immediately when a hit qualifies
 *
 * ── Typical interval ────────────────────────────────────────────────────────
 * Default Interval = 0.15s / RandomDeviation = 0.05s.
 */
UCLASS(Blueprintable, meta = (DisplayName = "AI State Observer (MYST)"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UBTService_AIStateObserver : public UBTService
{
	GENERATED_BODY()

public:

	UBTService_AIStateObserver(const FObjectInitializer& ObjectInitializer);

	virtual FString GetStaticDescription() const override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/**
	 * Safety net: unbind the health delegate if GC collects this node instance
	 * before OnCeaseRelevant fires (e.g. forceful actor destroy during PIE stop).
	 * Without this, ULyraHealthComponent::OnHealthChanged retains a dangling
	 * FScriptDelegate pointing to the destroyed UObject.
	 */
	virtual void BeginDestroy() override;

protected:

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// ── Blackboard Keys ────────────────────────────────────────────────────

	/** Bool key — true while the AI is actively reloading. Suggested name: OutOfAmmo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Keys|AI")
	FBlackboardKeySelector OutOfAmmoKey;

	/** Bool key — true for DamageCooldown seconds after a qualifying hit. Suggested name: HasTakenDamageRecently */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Keys|AI")
	FBlackboardKeySelector HasTakenDamageRecentlyKey;

	/** Object key — current enemy actor. Suggested name: TargetEnemy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Keys|Target")
	FBlackboardKeySelector TargetEnemyKey;

	/** Bool key — true while the target's ASC has an active reload tag. Suggested name: TargetIsReloading */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Keys|Target")
	FBlackboardKeySelector TargetIsReloadingKey;

	/** Bool key — true when target normalized health < LowHealthThreshold. Suggested name: TargetIsLowHealth */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Keys|Target")
	FBlackboardKeySelector TargetIsLowHealthKey;

	// ── Tuning ─────────────────────────────────────────────────────────────

	/** Gameplay tag checked against the AI's (and target's) ASC to detect an active reload. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Settings")
	FGameplayTag ReloadTag;

	/**
	 * Minimum fraction of MaxHealth lost in a single damage event to arm the flag.
	 * Prevents tiny DoT pulses from triggering.  Range: 0–1  (default 0.04 = 4%).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Settings",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageReactionThreshold = 0.04f;

	/**
	 * How many seconds HasTakenDamageRecently stays true after the last qualifying hit.
	 * Tune this directly on the BT node in the Behavior Tree editor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer",
		meta = (ClampMin = "0.1", UIMin = "1.0", UIMax = "10.0", ForceUnits = "s",
		        DisplayName = "Damage Cooldown (s)"))
	float DamageCooldown = 4.f;

	/** Normalized health value (0–1) below which the target is flagged as low health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer|Settings",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowHealthThreshold = 0.35f;

	// ── Blueprint Event Hooks ───────────────────────────────────────────────

	/** Fires on every reload-state transition (started / finished). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Observer|Events",
		meta = (DisplayName = "On AI Reload State Changed"))
	void OnAIReloadStateChanged(bool bIsNowReloading);

	/**
	 * Fires immediately when a hit above DamageReactionThreshold is received.
	 * HealthLostFraction is the normalized fraction lost in that single hit (0–1).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Observer|Events",
		meta = (DisplayName = "On AI Damage Detected"))
	void OnDamageDetected(float HealthLostFraction);

private:

	/**
	 * Delegate callback bound to ULyraHealthComponent::OnHealthChanged.
	 * Updates LastDamageTime when a qualifying hit lands.
	 * Must be a UFUNCTION so the dynamic multicast delegate can bind to it.
	 */
	UFUNCTION()
	void OnAIHealthChanged(ULyraHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator);

	/** Unbinds and clears the health delegate. Safe to call even if nothing is bound. */
	void UnbindHealthDelegate();

	// Per-AI instance state (valid because bCreateNodeInstance = true).

	/** World-time of the last qualifying damage hit. -1 means not currently armed. */
	float LastDamageTime = -1.f;

	/** Cached reload state for transition-edge detection in TickNode. */
	bool bWasReloading = false;


	/** Weak ref to the health component we're listening to, used for unbinding. */
	TWeakObjectPtr<ULyraHealthComponent> TrackedHealthComp;

	/** Weak ref to the BT component, used to write the BB from the health callback. */
	TWeakObjectPtr<UBehaviorTreeComponent> OwnerBTComp;
};

