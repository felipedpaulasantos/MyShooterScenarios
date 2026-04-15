// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"

#include "GA_UseItem.generated.h"

class UInventoryFragment_UsableItem;
class ULyraInventoryItemInstance;
class ULyraInventoryManagerComponent;

/**
 * UGA_UseItem
 *
 * Generic "use consumable item" ability. Grant this once to the pawn via GAS_UsableItems
 * AbilitySet in the Experience Definition, NOT per-item (unlike equipment abilities).
 *
 * Activation flow:
 *   1. Finds the best usable item in inventory (first item with UInventoryFragment_UsableItem).
 *   2. Calls CommitAbility() — applies cost (removes StacksConsumedPerUse items) and cooldown.
 *   3. Applies the fragment's EffectToApply to the avatar's AbilitySystemComponent.
 *   4. Fires K2_OnItemEffectApplied for Blueprint subclasses to add VFX / sounds / screen effects.
 *   5. If UseMontage is set: plays it via UAbilityTask_PlayMontageAndWait, ends when done.
 *      If no montage: ends the ability immediately.
 *
 * Cooldown: GetCooldownTags() dynamically reads CooldownTag from the usable fragment so the
 * standard GAS CooldownGameplayEffectClass mechanism blocks re-activation for the right tag.
 * Set CooldownGameplayEffectClass in a Blueprint subclass (e.g. GE_UseItem_Cooldown) that
 * grants the same tag as the fragment's CooldownTag for the desired duration.
 *
 * Blueprint subclassing guidelines:
 *   - Override K2_OnItemEffectApplied to add VFX, sounds, and screen effects.
 *   - Do NOT override Event ActivateAbility in Blueprint; the C++ flow handles the full lifecycle.
 *   - Set CooldownGameplayEffectClass to a GE that grants the fragment's CooldownTag.
 */
UCLASS(Blueprintable, meta=(ShortTooltip="Activates and consumes the best usable item found in inventory."))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UGA_UseItem : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_UseItem(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UGameplayAbility interface
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	//~End of UGameplayAbility interface

	/**
	 * Searches the current pawn's inventory for the first item that carries
	 * a UInventoryFragment_UsableItem fragment. Returns null if none found.
	 * Can be called from Blueprint to preview which item would be consumed.
	 */
	UFUNCTION(BlueprintPure, Category="MYST|UseItem")
	ULyraInventoryItemInstance* GetBestUsableItem() const;

	/**
	 * Returns the UInventoryFragment_UsableItem fragment from the given item instance,
	 * or null if the item does not carry the fragment.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="MYST|UseItem")
	static const UInventoryFragment_UsableItem* GetUsableFragment(const ULyraInventoryItemInstance* ItemInstance);

protected:

	//~UGameplayAbility interface (protected, matching base class visibility)
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	//~End of UGameplayAbility interface

	/**
	 * Called immediately after EffectToApply is applied to the avatar's ASC,
	 * before the montage begins. Override in Blueprint to add VFX, sounds, or screen effects.
	 *
	 * @param UsedItem  The inventory item instance that was activated.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="MYST|UseItem", DisplayName="On Item Effect Applied")
	void K2_OnItemEffectApplied(ULyraInventoryItemInstance* UsedItem);

private:

	// ─── Montage task callbacks ───────────────────────────────────────────────

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	// ─── Internal helpers ─────────────────────────────────────────────────────

	/** Gets the InventoryManagerComponent from the Controller in the given ActorInfo. */
	static ULyraInventoryManagerComponent* GetInventoryFromActorInfo(const FGameplayAbilityActorInfo* ActorInfo);

	/**
	 * Searches the inventory for the first item with a UInventoryFragment_UsableItem fragment.
	 * Thread-safe const (only reads inventory state).
	 */
	ULyraInventoryItemInstance* FindBestUsableItemFromActorInfo(const FGameplayAbilityActorInfo* ActorInfo) const;

	// ─── Cooldown tag caching ─────────────────────────────────────────────────

	/**
	 * Mutable cache populated every call to GetCooldownTags().
	 * Merges CooldownGameplayEffectClass tags with the fragment's dynamic CooldownTag.
	 */
	mutable FGameplayTagContainer CooldownTagsCache;
};



