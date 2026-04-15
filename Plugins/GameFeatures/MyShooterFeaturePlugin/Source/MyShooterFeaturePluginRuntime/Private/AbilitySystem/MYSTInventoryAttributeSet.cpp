// Copyright MyShooterScenarios. All Rights Reserved.

#include "AbilitySystem/MYSTInventoryAttributeSet.h"
#include "MYSTGameplayTags.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MYSTInventoryAttributeSet)

class FLifetimeProperty;

// ─────────────────────────────────────────────────────────────────────────────
// UMYSTInventoryAttributeSet
// ─────────────────────────────────────────────────────────────────────────────

UMYSTInventoryAttributeSet::UMYSTInventoryAttributeSet()
	: MaxWeaponSlots(2.0f)
	, MaxPotionSlots(3.0f)
	, MaxAmmoCapacity(200.0f)
	, MaxPickupWeapons(1.0f)
{
}

void UMYSTInventoryAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMYSTInventoryAttributeSet, MaxWeaponSlots,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMYSTInventoryAttributeSet, MaxPotionSlots,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMYSTInventoryAttributeSet, MaxAmmoCapacity,  COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMYSTInventoryAttributeSet, MaxPickupWeapons, COND_OwnerOnly, REPNOTIFY_Always);
}

void UMYSTInventoryAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	// All capacity attributes must be non-negative
	NewValue = FMath::Max(NewValue, 0.0f);
}

float UMYSTInventoryAttributeSet::GetCapacityForCategory(FGameplayTag CategoryTag) const
{
	if (CategoryTag == MYSTGameplayTags::Item_Category_Weapon)
	{
		return GetMaxWeaponSlots();
	}
	if (CategoryTag == MYSTGameplayTags::Item_Category_Potion)
	{
		return GetMaxPotionSlots();
	}
	if (CategoryTag == MYSTGameplayTags::Item_Category_Ammo)
	{
		return GetMaxAmmoCapacity();
	}
	if (CategoryTag == MYSTGameplayTags::Item_Category_PickupWeapon)
	{
		return GetMaxPickupWeapons();
	}

	// Unknown / unmapped category (e.g. Item.Category.Utility) → unlimited
	return -1.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
// RepNotify handlers
// ─────────────────────────────────────────────────────────────────────────────

void UMYSTInventoryAttributeSet::OnRep_MaxWeaponSlots(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMYSTInventoryAttributeSet, MaxWeaponSlots, OldValue);
}

void UMYSTInventoryAttributeSet::OnRep_MaxPotionSlots(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMYSTInventoryAttributeSet, MaxPotionSlots, OldValue);
}

void UMYSTInventoryAttributeSet::OnRep_MaxAmmoCapacity(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMYSTInventoryAttributeSet, MaxAmmoCapacity, OldValue);
}

void UMYSTInventoryAttributeSet::OnRep_MaxPickupWeapons(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMYSTInventoryAttributeSet, MaxPickupWeapons, OldValue);
}

