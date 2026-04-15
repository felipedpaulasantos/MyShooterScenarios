// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "Inventory/LyraInventoryItemDefinition.h"

#include "InventoryFragment_WeaponSlotType.generated.h"

/**
 * How a weapon occupies a slot in the UMYSTWeaponBarComponent (Phase 4).
 */
UENUM(BlueprintType)
enum class EWeaponSlotType : uint8
{
	/**
	 * Permanent weapon that the player always carries.
	 * Placed in a fixed slot (count driven by MaxWeaponSlots attribute).
	 * Persists across death/respawn and can be upgraded.
	 * Example: starting pistol, player's personal rifle.
	 */
	Fixed  UMETA(DisplayName="Fixed (Permanent)"),

	/**
	 * Temporary weapon picked up from the world.
	 * Occupies the single pickup weapon slot.
	 * Automatically discarded when its ammo reaches zero.
	 * Only one pickup weapon can be held at a time by default.
	 * Example: RPG found on the ground, enemy's dropped weapon.
	 */
	Pickup UMETA(DisplayName="Pickup (Temporary)"),
};

/**
 * UInventoryFragment_WeaponSlotType
 *
 * Attach this fragment to weapon ULyraInventoryItemDefinitions to control how
 * UMYSTWeaponBarComponent (Phase 4) assigns the weapon to a slot.
 *
 * Items without this fragment are treated as non-weapon inventory items and are
 * not placed in weapon slots by the weapon bar.
 */
UCLASS(Blueprintable, EditInlineNew, meta=(DisplayName="Weapon Slot Type"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UInventoryFragment_WeaponSlotType : public ULyraInventoryItemFragment
{
	GENERATED_BODY()

public:

	/** Determines whether this weapon occupies a permanent fixed slot or the temporary pickup slot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WeaponSlot")
	EWeaponSlotType SlotType = EWeaponSlotType::Fixed;
};

