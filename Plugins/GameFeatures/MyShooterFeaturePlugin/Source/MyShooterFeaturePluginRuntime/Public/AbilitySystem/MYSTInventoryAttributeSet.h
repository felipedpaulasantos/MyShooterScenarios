// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraAttributeSet.h"

#include "MYSTInventoryAttributeSet.generated.h"

class UObject;
struct FFrame;
struct FGameplayTag;

/**
 * UMYSTInventoryAttributeSet
 *
 * GAS Attribute Set that stores the player's inventory capacity limits.
 * Grant this to the PlayerState's AbilitySystemComponent via a ULyraAbilitySet
 * in the Experience Definition.
 *
 * All attributes are replicated to the owning client only (COND_OwnerOnly) and
 * are clamped to [0, ∞) via PreAttributeBaseChange.
 *
 * Capacity upgrades are applied through standard Gameplay Effects:
 *   GE_BackpackUpgrade_Potions  → +1 to MaxPotionSlots
 *   GE_BackpackUpgrade_Weapons  → +1 to MaxWeaponSlots
 *   etc.
 *
 * UMYSTInventoryCapacityComponent queries GetCapacityForCategory() to resolve
 * which attribute corresponds to a given Item.Category.* tag.
 */
UCLASS(BlueprintType)
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMYSTInventoryAttributeSet : public ULyraAttributeSet
{
	GENERATED_BODY()

public:

	UMYSTInventoryAttributeSet();

	// -------------------------------------------------------------------------
	// Attribute accessors (generated: GetX, SetX, InitX, GetXAttribute)
	// -------------------------------------------------------------------------

	/** Maximum number of permanent (Fixed) weapon slots. Default: 2. */
	ATTRIBUTE_ACCESSORS(UMYSTInventoryAttributeSet, MaxWeaponSlots);

	/** Maximum number of consumable (Potion) items the player can carry. Default: 3. */
	ATTRIBUTE_ACCESSORS(UMYSTInventoryAttributeSet, MaxPotionSlots);

	/** Maximum total ammo item count across all ammo types. Default: 200. */
	ATTRIBUTE_ACCESSORS(UMYSTInventoryAttributeSet, MaxAmmoCapacity);

	/** Maximum number of simultaneous pickup (Temporary) weapons. Default: 1. */
	ATTRIBUTE_ACCESSORS(UMYSTInventoryAttributeSet, MaxPickupWeapons);

	// -------------------------------------------------------------------------
	// Category-to-attribute helper
	// -------------------------------------------------------------------------

	/**
	 * Returns the capacity value for the given item category tag.
	 *
	 * Returns -1.0f when the tag is not mapped to any attribute, which
	 * UMYSTInventoryCapacityComponent interprets as "unlimited."
	 *
	 * Tag → Attribute mapping:
	 *   Item.Category.Weapon       → MaxWeaponSlots
	 *   Item.Category.Potion       → MaxPotionSlots
	 *   Item.Category.Ammo         → MaxAmmoCapacity
	 *   Item.Category.PickupWeapon → MaxPickupWeapons
	 *   (all others)               → -1.0f  (unlimited)
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Capacity")
	float GetCapacityForCategory(FGameplayTag CategoryTag) const;

	// ─── Individual named capacity getters (Blueprint convenience) ────────────
	// These wrap the C++ ATTRIBUTE_ACCESSORS getters as BlueprintPure UFUNCTIONs
	// so BP graphs can read them by name without needing to know the category tag.

	/** Maximum number of permanent weapon slots the player can carry. Default: 2. */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Capacity", DisplayName="Get Max Weapon Slots")
	float BP_GetMaxWeaponSlots() const { return GetMaxWeaponSlots(); }

	/** Maximum number of consumable (potion) items the player can carry. Default: 3. */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Capacity", DisplayName="Get Max Potion Slots")
	float BP_GetMaxPotionSlots() const { return GetMaxPotionSlots(); }

	/** Maximum total ammo item count across all ammo types. Default: 200. */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Capacity", DisplayName="Get Max Ammo Capacity")
	float BP_GetMaxAmmoCapacity() const { return GetMaxAmmoCapacity(); }

	/** Maximum number of simultaneous pickup (temporary) weapons. Default: 1. */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Capacity", DisplayName="Get Max Pickup Weapons")
	float BP_GetMaxPickupWeapons() const { return GetMaxPickupWeapons(); }

	// ─── UAttributeSet overrides (public to match base class visibility) ──────

	/** Clamps all capacity attributes to >= 0. */
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

protected:

	// ─── RepNotify handlers ───────────────────────────────────────────────────

	UFUNCTION()
	void OnRep_MaxWeaponSlots(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxPotionSlots(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxAmmoCapacity(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxPickupWeapons(const FGameplayAttributeData& OldValue);


private:

	/** Maximum number of permanent weapon slots the player can have. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxWeaponSlots,
	          Category="MYST|Inventory", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData MaxWeaponSlots;

	/** Maximum number of consumable (potion) items the player can carry. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxPotionSlots,
	          Category="MYST|Inventory", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData MaxPotionSlots;

	/** Maximum total ammo item count across all ammo types. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxAmmoCapacity,
	          Category="MYST|Inventory", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData MaxAmmoCapacity;

	/** Maximum number of simultaneous pickup weapons the player can hold. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxPickupWeapons,
	          Category="MYST|Inventory", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData MaxPickupWeapons;
};

