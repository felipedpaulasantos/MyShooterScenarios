// Copyright MyShooterScenarios. All Rights Reserved.

#include "Inventory/MYSTInventoryFunctionLibrary.h"

#include "AbilitySystem/MYSTInventoryAttributeSet.h"
#include "Inventory/InventoryFragment_ItemCategory.h"
#include "Inventory/InventoryFragment_UsableItem.h"
#include "Inventory/InventoryFragment_WeaponSlotType.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryManagerComponent.h"
#include "Inventory/MYSTInventoryCapacityComponent.h"
#include "Inventory/MYSTWeaponBarComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MYSTInventoryFunctionLibrary)

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace
{
	/** Safely resolves a Pawn to its Controller. Returns null for non-player pawns. */
	static AController* ControllerFromPawn(APawn* Pawn)
	{
		return IsValid(Pawn) ? Pawn->GetController() : nullptr;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Component Accessors
// ─────────────────────────────────────────────────────────────────────────────

UMYSTWeaponBarComponent* UMYSTInventoryFunctionLibrary::GetWeaponBarComponent(AController* Controller)
{
	return IsValid(Controller)
		? Controller->FindComponentByClass<UMYSTWeaponBarComponent>()
		: nullptr;
}

UMYSTWeaponBarComponent* UMYSTInventoryFunctionLibrary::GetWeaponBarFromPawn(APawn* Pawn)
{
	return GetWeaponBarComponent(ControllerFromPawn(Pawn));
}

UMYSTInventoryCapacityComponent* UMYSTInventoryFunctionLibrary::GetInventoryCapacityComponent(
	AController* Controller)
{
	return IsValid(Controller)
		? Controller->FindComponentByClass<UMYSTInventoryCapacityComponent>()
		: nullptr;
}

ULyraInventoryManagerComponent* UMYSTInventoryFunctionLibrary::GetInventoryManager(
	AController* Controller)
{
	return IsValid(Controller)
		? Controller->FindComponentByClass<ULyraInventoryManagerComponent>()
		: nullptr;
}

ULyraInventoryManagerComponent* UMYSTInventoryFunctionLibrary::GetInventoryManagerFromPawn(
	APawn* Pawn)
{
	return GetInventoryManager(ControllerFromPawn(Pawn));
}

const UMYSTInventoryAttributeSet* UMYSTInventoryFunctionLibrary::GetInventoryAttributeSet(
	AController* Controller)
{
	if (!IsValid(Controller))
	{
		return nullptr;
	}

	const APlayerState* PS = Controller->GetPlayerState<APlayerState>();
	if (!PS)
	{
		return nullptr;
	}

	const UAbilitySystemComponent* ASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS);

	return ASC ? ASC->GetSet<UMYSTInventoryAttributeSet>() : nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Item Fragment Queries
// ─────────────────────────────────────────────────────────────────────────────

FGameplayTag UMYSTInventoryFunctionLibrary::GetItemCategory(
	const ULyraInventoryItemInstance* Item)
{
	if (!IsValid(Item))
	{
		return FGameplayTag::EmptyTag;
	}
	const UInventoryFragment_ItemCategory* Frag =
		Item->FindFragmentByClass<UInventoryFragment_ItemCategory>();
	return Frag ? Frag->ItemCategory : FGameplayTag::EmptyTag;
}

EWeaponSlotType UMYSTInventoryFunctionLibrary::GetWeaponSlotType(
	const ULyraInventoryItemInstance* Item)
{
	if (!IsValid(Item))
	{
		return EWeaponSlotType::Fixed;
	}
	const UInventoryFragment_WeaponSlotType* Frag =
		Item->FindFragmentByClass<UInventoryFragment_WeaponSlotType>();
	return Frag ? Frag->SlotType : EWeaponSlotType::Fixed;
}

bool UMYSTInventoryFunctionLibrary::HasWeaponSlotFragment(
	const ULyraInventoryItemInstance* Item)
{
	return IsValid(Item) &&
		Item->FindFragmentByClass<UInventoryFragment_WeaponSlotType>() != nullptr;
}

bool UMYSTInventoryFunctionLibrary::IsFixedWeapon(const ULyraInventoryItemInstance* Item)
{
	if (!IsValid(Item))
	{
		return false;
	}
	const UInventoryFragment_WeaponSlotType* Frag =
		Item->FindFragmentByClass<UInventoryFragment_WeaponSlotType>();
	return Frag && Frag->SlotType == EWeaponSlotType::Fixed;
}

bool UMYSTInventoryFunctionLibrary::IsPickupWeapon(const ULyraInventoryItemInstance* Item)
{
	if (!IsValid(Item))
	{
		return false;
	}
	const UInventoryFragment_WeaponSlotType* Frag =
		Item->FindFragmentByClass<UInventoryFragment_WeaponSlotType>();
	return Frag && Frag->SlotType == EWeaponSlotType::Pickup;
}

bool UMYSTInventoryFunctionLibrary::IsUsableItem(const ULyraInventoryItemInstance* Item)
{
	return IsValid(Item) &&
		Item->FindFragmentByClass<UInventoryFragment_UsableItem>() != nullptr;
}

FText UMYSTInventoryFunctionLibrary::GetItemDisplayName(const ULyraInventoryItemInstance* Item)
{
	if (!IsValid(Item) || !Item->GetItemDef())
	{
		return FText::GetEmpty();
	}
	const ULyraInventoryItemDefinition* CDO =
		GetDefault<ULyraInventoryItemDefinition>(Item->GetItemDef());
	return CDO ? CDO->DisplayName : FText::GetEmpty();
}

// ─────────────────────────────────────────────────────────────────────────────
// Inventory Queries
// ─────────────────────────────────────────────────────────────────────────────

TArray<ULyraInventoryItemInstance*> UMYSTInventoryFunctionLibrary::FindItemsByCategory(
	AController* Controller, FGameplayTag Category)
{
	TArray<ULyraInventoryItemInstance*> Result;

	const ULyraInventoryManagerComponent* Inventory = GetInventoryManager(Controller);
	if (!Inventory || !Category.IsValid())
	{
		return Result;
	}

	for (ULyraInventoryItemInstance* Instance : Inventory->GetAllItems())
	{
		if (!IsValid(Instance))
		{
			continue;
		}
		const UInventoryFragment_ItemCategory* Frag =
			Instance->FindFragmentByClass<UInventoryFragment_ItemCategory>();
		if (Frag && Frag->ItemCategory == Category)
		{
			Result.Add(Instance);
		}
	}
	return Result;
}

TArray<ULyraInventoryItemInstance*> UMYSTInventoryFunctionLibrary::FindItemsByDefinition(
	AController* Controller, TSubclassOf<ULyraInventoryItemDefinition> ItemDef)
{
	TArray<ULyraInventoryItemInstance*> Result;

	const ULyraInventoryManagerComponent* Inventory = GetInventoryManager(Controller);
	if (!Inventory || !ItemDef)
	{
		return Result;
	}

	for (ULyraInventoryItemInstance* Instance : Inventory->GetAllItems())
	{
		if (IsValid(Instance) && Instance->GetItemDef() == ItemDef)
		{
			Result.Add(Instance);
		}
	}
	return Result;
}

bool UMYSTInventoryFunctionLibrary::CanGiveItemToPlayer(
	AController* Controller,
	TSubclassOf<ULyraInventoryItemDefinition> ItemDef,
	int32 StackCount)
{
	if (!IsValid(Controller) || !ItemDef || StackCount <= 0)
	{
		return false;
	}
	ULyraInventoryManagerComponent* Inventory = GetInventoryManager(Controller);
	return Inventory ? Inventory->CanAddItemDefinition(ItemDef, StackCount) : false;
}

bool UMYSTInventoryFunctionLibrary::FindWeaponBarSlotForItem(
	AController*               Controller,
	ULyraInventoryItemInstance* Item,
	bool&                      bOutIsPickupSlot,
	int32&                     OutFixedSlotIndex)
{
	bOutIsPickupSlot   = false;
	OutFixedSlotIndex  = INDEX_NONE;

	if (!IsValid(Controller) || !IsValid(Item))
	{
		return false;
	}

	UMYSTWeaponBarComponent* WeaponBar = GetWeaponBarComponent(Controller);
	if (!WeaponBar)
	{
		return false;
	}

	// Search fixed slots first.
	TArray<ULyraInventoryItemInstance*> Fixed = WeaponBar->GetFixedSlots();
	for (int32 i = 0; i < Fixed.Num(); ++i)
	{
		if (Fixed[i] == Item)
		{
			OutFixedSlotIndex = i;
			return true;
		}
	}

	// Check pickup slot.
	if (WeaponBar->GetPickupWeapon() == Item)
	{
		bOutIsPickupSlot = true;
		return true;
	}

	return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// High-level Operations
// ─────────────────────────────────────────────────────────────────────────────

ULyraInventoryItemInstance* UMYSTInventoryFunctionLibrary::GiveItemToPlayer(
	AController*                              Controller,
	TSubclassOf<ULyraInventoryItemDefinition> ItemDef,
	int32                                     StackCount)
{
	if (!IsValid(Controller) || !ItemDef || StackCount <= 0)
	{
		return nullptr;
	}

	ULyraInventoryManagerComponent* Inventory = GetInventoryManager(Controller);
	if (!Inventory)
	{
		return nullptr;
	}

	// Capacity check (includes MYST category limits via OnCanAddItem delegate).
	if (!Inventory->CanAddItemDefinition(ItemDef, StackCount))
	{
		return nullptr;
	}

	// Add to inventory.
	ULyraInventoryItemInstance* Item = Inventory->AddItemDefinition(ItemDef, StackCount);
	if (!IsValid(Item))
	{
		return nullptr;
	}

	// Auto-assign to weapon bar if the item has a weapon slot fragment.
	const UInventoryFragment_WeaponSlotType* SlotFrag =
		Item->FindFragmentByClass<UInventoryFragment_WeaponSlotType>();

	if (SlotFrag)
	{
		UMYSTWeaponBarComponent* WeaponBar = GetWeaponBarComponent(Controller);
		if (WeaponBar)
		{
			if (SlotFrag->SlotType == EWeaponSlotType::Fixed)
			{
				const int32 FreeSlot = WeaponBar->GetNextFreeFixedSlot();
				if (FreeSlot != INDEX_NONE)
				{
					WeaponBar->AddWeaponToFixedSlot(FreeSlot, Item);
				}
				// If no free slot: item is in inventory but not in the bar.
				// The caller can use AssignWeaponToFixedSlot later to place it.
			}
			else // EWeaponSlotType::Pickup
			{
				WeaponBar->SetPickupWeapon(Item);
			}
		}
	}

	return Item;
}

bool UMYSTInventoryFunctionLibrary::RemoveWeaponFromPlayer(
	AController*               Controller,
	ULyraInventoryItemInstance* WeaponItem,
	bool                       bAlsoRemoveFromInventory)
{
	if (!IsValid(Controller) || !IsValid(WeaponItem))
	{
		return false;
	}

	bool bRemovedFromBar = false;

	UMYSTWeaponBarComponent* WeaponBar = GetWeaponBarComponent(Controller);
	if (WeaponBar)
	{
		// Search fixed slots.
		TArray<ULyraInventoryItemInstance*> Fixed = WeaponBar->GetFixedSlots();
		for (int32 i = 0; i < Fixed.Num(); ++i)
		{
			if (Fixed[i] == WeaponItem)
			{
				WeaponBar->RemoveWeaponFromFixedSlot(i);
				bRemovedFromBar = true;
				break;
			}
		}

		// Check pickup slot.
		if (!bRemovedFromBar && WeaponBar->GetPickupWeapon() == WeaponItem)
		{
			WeaponBar->ClearPickupWeapon();
			bRemovedFromBar = true;
		}
	}

	if (bAlsoRemoveFromInventory)
	{
		ULyraInventoryManagerComponent* Inventory = GetInventoryManager(Controller);
		if (Inventory)
		{
			Inventory->RemoveItemInstance(WeaponItem);
		}
	}

	return bRemovedFromBar;
}

bool UMYSTInventoryFunctionLibrary::AssignWeaponToFixedSlot(
	AController*               Controller,
	ULyraInventoryItemInstance* WeaponItem,
	int32                      SlotIndex)
{
	if (!IsValid(Controller) || !IsValid(WeaponItem) || SlotIndex < 0)
	{
		return false;
	}

	// Verify the item is actually in this controller's inventory.
	ULyraInventoryManagerComponent* Inventory = GetInventoryManager(Controller);
	if (!Inventory)
	{
		return false;
	}

	bool bFoundInInventory = false;
	for (ULyraInventoryItemInstance* Instance : Inventory->GetAllItems())
	{
		if (Instance == WeaponItem)
		{
			bFoundInInventory = true;
			break;
		}
	}
	if (!bFoundInInventory)
	{
		return false;
	}

	UMYSTWeaponBarComponent* WeaponBar = GetWeaponBarComponent(Controller);
	if (!WeaponBar || SlotIndex >= WeaponBar->GetNumFixedSlots())
	{
		return false;
	}

	WeaponBar->AddWeaponToFixedSlot(SlotIndex, WeaponItem);
	return true;
}

