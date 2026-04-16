// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
// EWeaponSlotType enum lives here — included so it can be used as a return type below.
#include "Inventory/InventoryFragment_WeaponSlotType.h"

#include "MYSTInventoryFunctionLibrary.generated.h"

class AController;
class APawn;
class ULyraInventoryItemDefinition;
class ULyraInventoryItemInstance;
class ULyraInventoryManagerComponent;
class UMYSTInventoryAttributeSet;
class UMYSTInventoryCapacityComponent;
class UMYSTWeaponBarComponent;

/**
 * UMYSTInventoryFunctionLibrary
 *
 * Static Blueprint utility nodes for the MYST inventory / weapon-bar system.
 *
 * Organised into four groups:
 *
 *   COMPONENT ACCESSORS
 *     Retrieve the MYST components and attribute set from a Controller or Pawn.
 *     All return null when the component is not present (e.g. before the
 *     Experience is fully loaded, or on a pawn without an inventory).
 *
 *   ITEM FRAGMENT QUERIES
 *     Read MYST-specific fragment data from a ULyraInventoryItemInstance without
 *     manually casting fragments.  All return safe defaults when the fragment is absent.
 *
 *   INVENTORY QUERIES
 *     Search the inventory manager for items matching a category or definition class.
 *
 *   HIGH-LEVEL OPERATIONS
 *     One-stop helpers that combine multiple lower-level calls.  All are
 *     Authority-only because they mutate authoritative inventory / weapon-bar state.
 */
UCLASS()
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMYSTInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// =========================================================================
	// COMPONENT ACCESSORS
	// =========================================================================

	/**
	 * Returns the UMYSTWeaponBarComponent attached to the given Controller.
	 * Null if the component was not added (e.g. Experience not yet loaded).
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Accessors",
	          DisplayName="Get Weapon Bar (Controller)")
	static UMYSTWeaponBarComponent* GetWeaponBarComponent(AController* Controller);

	/**
	 * Returns the UMYSTWeaponBarComponent from the Controller of the given Pawn.
	 * Convenience wrapper — equivalent to GetWeaponBarComponent(Pawn->GetController()).
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Accessors",
	          DisplayName="Get Weapon Bar (Pawn)")
	static UMYSTWeaponBarComponent* GetWeaponBarFromPawn(APawn* Pawn);

	/**
	 * Returns the UMYSTInventoryCapacityComponent attached to the given Controller.
	 * Null if the component was not added.
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Accessors",
	          DisplayName="Get Inventory Capacity Component (Controller)")
	static UMYSTInventoryCapacityComponent* GetInventoryCapacityComponent(AController* Controller);

	/**
	 * Returns the ULyraInventoryManagerComponent attached to the given Controller.
	 * Null if none is present.
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Accessors",
	          DisplayName="Get Inventory Manager (Controller)")
	static ULyraInventoryManagerComponent* GetInventoryManager(AController* Controller);

	/**
	 * Returns the ULyraInventoryManagerComponent from the Controller of the given Pawn.
	 * Convenience wrapper.
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Accessors",
	          DisplayName="Get Inventory Manager (Pawn)")
	static ULyraInventoryManagerComponent* GetInventoryManagerFromPawn(APawn* Pawn);

	/**
	 * Returns the UMYSTInventoryAttributeSet from the Controller's PlayerState ASC.
	 * Null if the attribute set has not yet been granted (common before Experience load).
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Accessors",
	          DisplayName="Get Inventory Attribute Set")
	static const UMYSTInventoryAttributeSet* GetInventoryAttributeSet(AController* Controller);

	// =========================================================================
	// ITEM FRAGMENT QUERIES
	// =========================================================================

	/**
	 * Returns the item's category tag (from UInventoryFragment_ItemCategory).
	 * Returns an invalid tag when the fragment is absent (item has no category).
	 *
	 * Example categories: Item.Category.Weapon, Item.Category.Potion, Item.Category.Ammo
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Item",
	          DisplayName="Get Item Category")
	static FGameplayTag GetItemCategory(const ULyraInventoryItemInstance* Item);

	/**
	 * Returns the weapon slot type (from UInventoryFragment_WeaponSlotType).
	 * Returns EWeaponSlotType::Fixed as a safe default when the fragment is absent.
	 * Use HasWeaponSlotFragment() first if you need to distinguish "no fragment" from Fixed.
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Item",
	          DisplayName="Get Weapon Slot Type")
	static EWeaponSlotType GetWeaponSlotType(const ULyraInventoryItemInstance* Item);

	/**
	 * Returns true when the item has a UInventoryFragment_WeaponSlotType fragment.
	 * Use this to determine whether an item is a weapon at all before reading its slot type.
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Item",
	          DisplayName="Has Weapon Slot Fragment")
	static bool HasWeaponSlotFragment(const ULyraInventoryItemInstance* Item);

	/**
	 * Returns true when the item is a fixed (permanent) weapon.
	 * Equivalent to: HasWeaponSlotFragment && GetWeaponSlotType == Fixed.
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Item",
	          DisplayName="Is Fixed Weapon")
	static bool IsFixedWeapon(const ULyraInventoryItemInstance* Item);

	/**
	 * Returns true when the item is a pickup (temporary) weapon.
	 * Equivalent to: HasWeaponSlotFragment && GetWeaponSlotType == Pickup.
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Item",
	          DisplayName="Is Pickup Weapon")
	static bool IsPickupWeapon(const ULyraInventoryItemInstance* Item);

	/**
	 * Returns true when the item can be actively "used" (has UInventoryFragment_UsableItem).
	 * Usable items are consumed by the GA_UseItem ability (potions, grenades, etc.).
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Item",
	          DisplayName="Is Usable Item")
	static bool IsUsableItem(const ULyraInventoryItemInstance* Item);

	/**
	 * Returns the display name defined on the item's ULyraInventoryItemDefinition CDO.
	 * Returns an empty text when the item or its definition is null.
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Item",
	          DisplayName="Get Item Display Name")
	static FText GetItemDisplayName(const ULyraInventoryItemInstance* Item);

	// =========================================================================
	// INVENTORY QUERIES
	// =========================================================================

	/**
	 * Returns all items in the Controller's inventory that belong to the given category.
	 * Filters by UInventoryFragment_ItemCategory.ItemCategory == Category.
	 * Returns an empty array when no matches are found or the inventory is unavailable.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="MYST|Inventory|Queries",
	          DisplayName="Find Items By Category")
	static TArray<ULyraInventoryItemInstance*> FindItemsByCategory(
		AController*    Controller,
		FGameplayTag    Category);

	/**
	 * Returns all items in the Controller's inventory that match the given item definition class.
	 * Useful for counting stacks of a specific consumable.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="MYST|Inventory|Queries",
	          DisplayName="Find Items By Definition")
	static TArray<ULyraInventoryItemInstance*> FindItemsByDefinition(
		AController*                                  Controller,
		TSubclassOf<ULyraInventoryItemDefinition>     ItemDef);

	/**
	 * Returns true when the Controller's inventory can accept the given item definition
	 * (respects capacity limits from UMYSTInventoryCapacityComponent).
	 * Does NOT add the item.  Call GiveItemToPlayer to actually add it.
	 */
	UFUNCTION(BlueprintPure, Category="MYST|Inventory|Queries",
	          DisplayName="Can Give Item To Player")
	static bool CanGiveItemToPlayer(
		AController*                              Controller,
		TSubclassOf<ULyraInventoryItemDefinition> ItemDef,
		int32                                     StackCount = 1);

	/**
	 * Searches the Controller's weapon bar for the slot that holds the given item.
	 *
	 * @param OutIsPickupSlot    Set to true when the item is in the pickup slot.
	 * @param OutFixedSlotIndex  Set to the 0-based fixed slot index when not a pickup, or INDEX_NONE.
	 * @return                   true when the item was found in the weapon bar.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="MYST|Inventory|Queries",
	          DisplayName="Find Weapon Bar Slot For Item")
	static bool FindWeaponBarSlotForItem(
		AController*              Controller,
		ULyraInventoryItemInstance* Item,
		bool&                     bOutIsPickupSlot,
		int32&                    OutFixedSlotIndex);

	// =========================================================================
	// HIGH-LEVEL OPERATIONS  (Authority-only)
	// =========================================================================

	/**
	 * Adds an item to the player's inventory and, when it's a weapon, automatically
	 * assigns it to the weapon bar:
	 *   Fixed weapon  → placed in the first free fixed slot (if available).
	 *   Pickup weapon → replaces the pickup slot.
	 *
	 * Returns the created ULyraInventoryItemInstance, or null when:
	 *   • the inventory rejects the item (capacity full, null def, etc.)
	 *   • the inventory manager or controller is unavailable
	 *
	 * If a fixed weapon is added but all weapon bar slots are occupied the item
	 * is still in the inventory — it just isn't in any weapon bar slot yet.
	 * The player can manually assign it later.
	 *
	 * Authority-only.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MYST|Inventory|Operations",
	          DisplayName="Give Item To Player")
	static ULyraInventoryItemInstance* GiveItemToPlayer(
		AController*                              Controller,
		TSubclassOf<ULyraInventoryItemDefinition> ItemDef,
		int32                                     StackCount = 1);

	/**
	 * Removes a weapon from the weapon bar and optionally removes it from the inventory.
	 *
	 * Use bAlsoRemoveFromInventory = true  when the weapon is being dropped or discarded.
	 * Use bAlsoRemoveFromInventory = false when the weapon should stay in the inventory
	 *   (e.g. swapping slots) while only clearing the bar slot.
	 *
	 * Returns true when the item was found in (and removed from) the weapon bar.
	 * Authority-only.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MYST|Inventory|Operations",
	          DisplayName="Remove Weapon From Player")
	static bool RemoveWeaponFromPlayer(
		AController*               Controller,
		ULyraInventoryItemInstance* WeaponItem,
		bool                       bAlsoRemoveFromInventory = true);

	/**
	 * Assigns an item that is already in the inventory to a specific fixed weapon bar slot.
	 *
	 * Use this to manually re-arrange fixed weapons between slots, or to explicitly
	 * assign a weapon that GiveItemToPlayer couldn't auto-place (all slots were full).
	 *
	 * Returns true on success.  Fails silently when:
	 *   • the item is not in the inventory
	 *   • SlotIndex is out-of-range
	 *   • the weapon bar component is unavailable
	 *
	 * Authority-only.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MYST|Inventory|Operations",
	          DisplayName="Assign Weapon To Fixed Slot")
	static bool AssignWeaponToFixedSlot(
		AController*               Controller,
		ULyraInventoryItemInstance* WeaponItem,
		int32                      SlotIndex);
};

