// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateCombatState.generated.h"

/**
 * Service that updates combat state, including tracking health %, 
 * if target is tracked in cover, and if this AI is isolated from its team.
 */
UCLASS()
class LYRAGAME_API UBTService_UpdateCombatState : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateCombatState();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;

protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector HealthPercentageKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector IsIsolatedKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector HasTargetInCoverKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetEnemyKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetTimeInCoverKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector IsTargetEngagingOtherKey;

	UPROPERTY(EditAnywhere, Category = "Combat Settings")
	float IsolationRadius = 1500.f;

	/**
	 * Dead-band for HealthPercentageKey writes.
	 * The BB key is only updated when the normalized health changes by at least
	 * this fraction, avoiding micro-changes (regen ticks, float precision) from
	 * firing BB notifications and triggering Observer-Abort decorators every tick.
	 * Default: 0.01 (1% of MaxHealth).
	 */
	UPROPERTY(EditAnywhere, Category = "Combat Settings",
		meta = (ClampMin = "0.0", ClampMax = "0.1"))
	float HealthWriteDeadBand = 0.01f;

	/**
	 * Hysteresis band (cm) applied to IsolationRadius for the IsIsolated check.
	 * An ally must enter within (IsolationRadius - IsolationHysteresis) to clear
	 * the flag, but must leave beyond IsolationRadius to set it.  Prevents
	 * constant true/false flipping when an ally stands right at the boundary.
	 * Default: 100 cm.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat Settings",
		meta = (ClampMin = "0.0", ForceUnits = "cm"))
	float IsolationHysteresis = 100.f;

private:
	struct FCombatStateMemory
	{
		float TargetCoverTime = 0.f;

		/** Last health fraction actually written to the BB (for dead-band check). */
		float LastWrittenHealthPct = -1.f;

		/**
		 * Hysteresis state for IsTargetEngagingOther.
		 * Only written to BB on an actual flip, preventing dot-product noise from
		 * triggering Observer-Abort decorators at frame rate.
		 */
		bool bCachedEngagingOther = false;

		/**
		 * Hysteresis state for IsIsolated.
		 * Prevents boundary oscillation when an ally sits at the IsolationRadius edge.
		 */
		bool bCachedIsIsolated = true;

		/**
		 * False on the very first tick after memory is (re)initialized.
		 * Guards TargetCoverTime from being reset by a spurious BT abort
		 * mid-fight (mirrors the same pattern in BTService_AIStateObserver).
		 */
		bool bCoverTimeInitialized = false;

		FCombatStateMemory() = default;
	};
};
