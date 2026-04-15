// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "Inventory/LyraInventoryItemDefinition.h"

#include "InventoryFragment_UsableItem.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * UInventoryFragment_UsableItem
 *
 * Attach this fragment to any ULyraInventoryItemDefinition that can be actively "used"
 * by the player (health potions, energy drinks, grenades, etc.).
 *
 * The companion ability GA_UseItem (Phase 3) reads this fragment to:
 *   1. Play UseMontage on the avatar.
 *   2. Apply EffectToApply to the avatar's AbilitySystemComponent.
 *   3. Consume StacksConsumedPerUse instances from the inventory via ULyraAbilityCost_InventoryItem.
 *   4. Enforce a cooldown via CooldownTag.
 *
 * Blueprint subclasses of GA_UseItem can add VFX, sounds, and screen effects on top.
 */
UCLASS(Blueprintable, EditInlineNew, meta=(DisplayName="Usable Item"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UInventoryFragment_UsableItem : public ULyraInventoryItemFragment
{
	GENERATED_BODY()

public:

	/**
	 * The Gameplay Effect applied to the avatar's ASC when this item is used.
	 * Typically an instant effect that modifies Health, Energy, or another attribute.
	 * Example: GE_HealPotion (instant, +50 to Health attribute).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Usage")
	TSubclassOf<UGameplayEffect> EffectToApply;

	/**
	 * Animation Montage played on the avatar during item use.
	 * The ability waits for the montage to finish (or reach the notify section)
	 * before committing the cost and applying the effect.
	 * Can be left null for an instant use with no animation.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Usage")
	TObjectPtr<UAnimMontage> UseMontage;

	/**
	 * Cooldown Gameplay Tag applied after the item is used.
	 * GA_UseItem will use this tag to block re-activation until the cooldown expires.
	 * Should match the tag set on the cooldown GameplayEffect in the ability.
	 * Example: Ability.Cooldown.UseItem.Potion
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Usage",
	          meta=(Categories="Ability.Cooldown"))
	FGameplayTag CooldownTag;

	/**
	 * How many inventory item instances are consumed per single use activation.
	 * Usually 1. Keyed on ability level (ScalableFloat) for future extensibility.
	 * The cost is applied via ULyraAbilityCost_InventoryItem (re-enabled in Phase 1).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Usage")
	FScalableFloat StacksConsumedPerUse;

	/**
	 * Returns the number of stacks this item consumes at the given ability level.
	 * Convenience wrapper for HUD/Blueprint use; internally calls FScalableFloat::GetValueAtLevel.
	 * Use AbilityLevel = 1 for standard items that don't scale with level.
	 */
	UFUNCTION(BlueprintPure, Category="Usage", meta=(AdvancedDisplay="AbilityLevel"))
	int32 GetStacksConsumedAtLevel(int32 AbilityLevel = 1) const
	{
		return FMath::Max(1, FMath::TruncToInt(StacksConsumedPerUse.GetValueAtLevel(AbilityLevel)));
	}

	UInventoryFragment_UsableItem()
	{
		StacksConsumedPerUse.SetValue(1.0f);
	}
};

