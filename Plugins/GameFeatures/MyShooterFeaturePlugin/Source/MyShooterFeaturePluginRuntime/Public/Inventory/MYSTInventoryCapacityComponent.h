// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"
#include "GameplayTagContainer.h"

#include "MYSTInventoryCapacityComponent.generated.h"

class ULyraInventoryItemDefinition;
class ULyraInventoryManagerComponent;
class UMYSTInventoryAttributeSet;

/**
 * UMYSTInventoryCapacityComponent
 *
 * A ControllerComponent that enforces per-category inventory slot limits.
 * Add to the PlayerController via UGameFeatureAction_AddComponents in your Experience.
 *
 * HOW IT WORKS
 * ─────────────
 * On BeginPlay it binds to ULyraInventoryManagerComponent::OnCanAddItem.
 * Whenever something tries to add an item, HandleCanAddItem fires:
 *   1. Reads UInventoryFragment_ItemCategory from the item definition.
 *   2. Counts how many items with that same category tag are already in inventory.
 *   3. Queries UMYSTInventoryAttributeSet on the controller's ASC for the max capacity.
 *   4. If current + incoming > max, sets bCanAdd = false to block the addition.
 *
 * Items without a UInventoryFragment_ItemCategory, or with a category that has no
 * corresponding attribute (returns -1), are always allowed through.
 *
 * UPGRADING CAPACITY
 * ───────────────────
 * Apply a Gameplay Effect that adds to the relevant attribute:
 *   GE_BackpackUpgrade_Potions → +1 MaxPotionSlots
 *   GE_BackpackUpgrade_Weapons → +1 MaxWeaponSlots
 *   etc.
 *
 * BLUEPRINT QUERIES
 * ──────────────────
 * Use GetCurrentCountForCategory / GetMaxCapacityForCategory / GetRemainingCapacityForCategory
 * to populate inventory HUD widgets.
 */
UCLASS(Blueprintable, ClassGroup=Inventory, meta=(BlueprintSpawnableComponent,
       DisplayName="MYST Inventory Capacity Component"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMYSTInventoryCapacityComponent : public UControllerComponent
{
	GENERATED_BODY()

public:

	UMYSTInventoryCapacityComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	// -------------------------------------------------------------------------
	// Blueprint-callable queries  (useful for HUD widgets)
	// -------------------------------------------------------------------------

	/**
	 * Returns the number of items currently in the inventory that belong to the
	 * given category (matched via UInventoryFragment_ItemCategory).
	 */
	UFUNCTION(BlueprintPure, Category="Inventory|Capacity")
	int32 GetCurrentCountForCategory(FGameplayTag Category) const;

	/**
	 * Returns the maximum capacity for the given category, as specified by
	 * UMYSTInventoryAttributeSet on the controller's ASC.
	 * Returns -1.0f if the category is not mapped to any attribute (= unlimited).
	 */
	UFUNCTION(BlueprintPure, Category="Inventory|Capacity")
	float GetMaxCapacityForCategory(FGameplayTag Category) const;

	/**
	 * Returns (MaxCapacity - CurrentCount) for the given category.
	 * Returns -1.0f if the category is unlimited.
	 * A value of 0 means the inventory is full for this category.
	 */
	UFUNCTION(BlueprintPure, Category="Inventory|Capacity")
	float GetRemainingCapacityForCategory(FGameplayTag Category) const;

	/**
	 * Returns true if at least one more item of the given category can be added.
	 * Returns true for unlimited categories (no attribute mapping).
	 */
	UFUNCTION(BlueprintPure, Category="Inventory|Capacity")
	bool CanAddItemOfCategory(FGameplayTag Category) const;

private:

	/**
	 * Bound to ULyraInventoryManagerComponent::OnCanAddItem.
	 * Sets bCanAdd = false when adding ItemDef would exceed the category capacity.
	 */
	UFUNCTION()
	void HandleCanAddItem(TSubclassOf<ULyraInventoryItemDefinition> ItemDef, int32 StackCount, bool& bCanAdd);

	// -------------------------------------------------------------------------
	// Internal helpers
	// -------------------------------------------------------------------------

	/** Returns the InventoryManagerComponent on the owning Controller, or null. */
	ULyraInventoryManagerComponent* GetInventoryManager() const;

	/** Returns the MYSTInventoryAttributeSet from the controller's PlayerState ASC, or null. */
	const UMYSTInventoryAttributeSet* GetAttributeSet() const;
};

