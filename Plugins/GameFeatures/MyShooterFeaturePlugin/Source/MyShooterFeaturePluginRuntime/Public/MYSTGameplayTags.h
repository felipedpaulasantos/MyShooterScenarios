// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * Native Gameplay Tags for the MyShooterFeaturePlugin.
 *
 * All tags in this namespace are registered at module startup via UE_DEFINE_GAMEPLAY_TAG
 * and will appear automatically in the editor's tag picker.
 *
 * Naming convention: TAG_<DotSeparatedTagName>
 */
namespace MYSTGameplayTags
{
	// -------------------------------------------------------------------------
	// Item Categories — used by UInventoryFragment_ItemCategory and
	// UMYSTInventoryCapacityComponent to enforce per-slot-type limits.
	// -------------------------------------------------------------------------

	/** Permanent (fixed) weapons the player always carries. Maps to MaxWeaponSlots attribute. */
	MYSHOOTERFEATUREPLUGINRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Weapon);

	/** Consumable items (health potions, energy potions, etc.). Maps to MaxPotionSlots attribute. */
	MYSHOOTERFEATUREPLUGINRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Potion);

	/** Ammo items (bullets, shells, etc.). Maps to MaxAmmoCapacity attribute. */
	MYSHOOTERFEATUREPLUGINRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Ammo);

	/** Pickup / temporary weapons found in the world. Maps to MaxPickupWeapons attribute. */
	MYSHOOTERFEATUREPLUGINRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_PickupWeapon);

	/** Utility items (grenades, gadgets, etc.) that don't fit other categories. Unlimited by default. */
	MYSHOOTERFEATUREPLUGINRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Category_Utility);

	// -------------------------------------------------------------------------
	// Use-Item Cooldown Tags — set these on UInventoryFragment_UsableItem.CooldownTag
	// and on a matching GE_UseItem_Cooldown that grants the tag for a duration.
	// -------------------------------------------------------------------------

	/** Generic use-item cooldown. Used when all consumables share one cooldown timer. */
	MYSHOOTERFEATUREPLUGINRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_UseItem);

	/** Per-potion cooldown (health / energy potions). */
	MYSHOOTERFEATUREPLUGINRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_UseItem_Potion);

	// -------------------------------------------------------------------------
	// Weapon Bar Messages — broadcast via UGameplayMessageSubsystem
	// Subscribe in HUD widgets to refresh weapon bar display.
	// -------------------------------------------------------------------------

	/** Payload: FMYSTWeaponBarSlotsChangedMessage. Fired when fixed or pickup slot contents change. */
	MYSHOOTERFEATUREPLUGINRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(MYST_WeaponBar_Message_SlotsChanged);

	/** Payload: FMYSTWeaponBarActiveSlotChangedMessage. Fired when the active weapon slot changes. */
	MYSHOOTERFEATUREPLUGINRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(MYST_WeaponBar_Message_ActiveSlotChanged);
}

