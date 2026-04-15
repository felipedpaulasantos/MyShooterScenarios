// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Inventory/LyraInventoryItemDefinition.h"

#include "InventoryFragment_ItemCategory.generated.h"

/**
 * UInventoryFragment_ItemCategory
 *
 * Attach this fragment to a ULyraInventoryItemDefinition to assign it a category tag
 * (e.g. Item.Category.Weapon, Item.Category.Potion, Item.Category.Ammo).
 *
 * UMYSTInventoryCapacityComponent reads this fragment on every item in the inventory
 * to enforce per-category slot limits that are driven by UMYSTInventoryAttributeSet.
 *
 * Items without this fragment are exempt from capacity checks (treated as unlimited).
 */
UCLASS(Blueprintable, EditInlineNew, meta=(DisplayName="Item Category"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UInventoryFragment_ItemCategory : public ULyraInventoryItemFragment
{
	GENERATED_BODY()

public:

	/**
	 * The category this item belongs to.
	 * Must match one of the Item.Category.* tags registered in MYSTGameplayTags
	 * for the capacity system to enforce limits on it.
	 *
	 * Examples:
	 *   Item.Category.Weapon        → capacity driven by MaxWeaponSlots attribute
	 *   Item.Category.Potion        → capacity driven by MaxPotionSlots attribute
	 *   Item.Category.Ammo          → capacity driven by MaxAmmoCapacity attribute
	 *   Item.Category.PickupWeapon  → capacity driven by MaxPickupWeapons attribute
	 *   Item.Category.Utility       → no attribute mapping, treated as unlimited
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Category",
	          meta=(Categories="Item.Category"))
	FGameplayTag ItemCategory;
};

