// Copyright MyShooterScenarios. All Rights Reserved.

#include "Inventory/MYSTWeaponBarComponent.h"

#include "AbilitySystem/MYSTInventoryAttributeSet.h"
#include "Equipment/LyraEquipmentDefinition.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Equipment/LyraEquipmentInstance.h"
#include "Inventory/InventoryFragment_EquippableItem.h"
#include "MYSTGameplayTags.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MYSTWeaponBarComponent)

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / lifecycle
// ─────────────────────────────────────────────────────────────────────────────

UMYSTWeaponBarComponent::UMYSTWeaponBarComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UMYSTWeaponBarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, FixedSlots);
	DOREPLIFETIME(ThisClass, PickupSlot);
	DOREPLIFETIME(ThisClass, ActiveSlotIndex);
}

void UMYSTWeaponBarComponent::BeginPlay()
{
	Super::BeginPlay();

	// Only the server is authoritative over slot state.
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	// Determine how many fixed slots to create.
	// Prefer the MaxWeaponSlots attribute if the attribute set is already granted
	// (possible when the Experience completes before BeginPlay fires on a listen server).
	// Fall back to DefaultNumFixedSlots otherwise.
	int32 NumSlots = DefaultNumFixedSlots;

	if (const AController* MyController = Cast<AController>(GetOwner()))
	{
		if (const APlayerState* PS = MyController->GetPlayerState<APlayerState>())
		{
			if (const UAbilitySystemComponent* ASC =
				UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS))
			{
				if (const UMYSTInventoryAttributeSet* AttrSet =
					ASC->GetSet<UMYSTInventoryAttributeSet>())
				{
					const int32 AttributeSlots =
						FMath::TruncToInt(AttrSet->GetMaxWeaponSlots());
					if (AttributeSlots > 0)
					{
						NumSlots = AttributeSlots;
					}
				}
			}
		}
	}

	if (FixedSlots.Num() < NumSlots)
	{
		FixedSlots.AddDefaulted(NumSlots - FixedSlots.Num());
	}
}

void UMYSTWeaponBarComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Cleanly unequip on server before the component is removed.
	if (GetOwner()->HasAuthority() && EquippedItem)
	{
		UnequipActiveWeapon();
	}

	Super::EndPlay(EndPlayReason);
}

// ─────────────────────────────────────────────────────────────────────────────
// Fixed slot management
// ─────────────────────────────────────────────────────────────────────────────

void UMYSTWeaponBarComponent::AddWeaponToFixedSlot(int32 SlotIndex, ULyraInventoryItemInstance* Item)
{
	if (!FixedSlots.IsValidIndex(SlotIndex) || !IsValid(Item))
	{
		return;
	}

	const bool bWasActiveSlot = (ActiveSlotIndex == SlotIndex);

	// If replacing the active slot, unequip first.
	if (bWasActiveSlot)
	{
		UnequipActiveWeapon();
	}

	FixedSlots[SlotIndex] = Item;
	OnRep_FixedSlots();

	// Re-equip if this slot is (or remains) active.
	if (bWasActiveSlot)
	{
		EquipActiveWeapon();
		BroadcastActiveSlotChanged();
	}
}

ULyraInventoryItemInstance* UMYSTWeaponBarComponent::RemoveWeaponFromFixedSlot(int32 SlotIndex)
{
	if (!FixedSlots.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	ULyraInventoryItemInstance* Removed = FixedSlots[SlotIndex];

	if (ActiveSlotIndex == SlotIndex)
	{
		UnequipActiveWeapon();
		ActiveSlotIndex = INDEX_NONE;
		OnRep_ActiveSlotIndex();
		BroadcastActiveSlotChanged();
	}

	FixedSlots[SlotIndex] = nullptr;
	OnRep_FixedSlots();

	return Removed;
}

TArray<ULyraInventoryItemInstance*> UMYSTWeaponBarComponent::GetFixedSlots() const
{
	TArray<ULyraInventoryItemInstance*> Result;
	Result.Reserve(FixedSlots.Num());
	for (const TObjectPtr<ULyraInventoryItemInstance>& SlotPtr : FixedSlots)
	{
		Result.Add(SlotPtr.Get());
	}
	return Result;
}

void UMYSTWeaponBarComponent::ResizeFixedSlots(int32 NewCount)
{
	NewCount = FMath::Max(1, NewCount);

	if (NewCount == FixedSlots.Num())
	{
		return;
	}

	if (NewCount > FixedSlots.Num())
	{
		// Growing: append empty slots.
		FixedSlots.AddDefaulted(NewCount - FixedSlots.Num());
	}
	else
	{
		// Shrinking: unequip if active slot is being removed, then truncate.
		if (ActiveSlotIndex >= NewCount)
		{
			UnequipActiveWeapon();
			ActiveSlotIndex = INDEX_NONE;
			OnRep_ActiveSlotIndex();
			BroadcastActiveSlotChanged();
		}
		FixedSlots.SetNum(NewCount);
	}

	OnRep_FixedSlots();
}

int32 UMYSTWeaponBarComponent::GetNextFreeFixedSlot() const
{
	for (int32 i = 0; i < FixedSlots.Num(); ++i)
	{
		if (FixedSlots[i] == nullptr)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

// ─────────────────────────────────────────────────────────────────────────────
// Pickup slot management
// ─────────────────────────────────────────────────────────────────────────────

void UMYSTWeaponBarComponent::SetPickupWeapon(ULyraInventoryItemInstance* Item)
{
	if (!IsValid(Item))
	{
		ClearPickupWeapon();
		return;
	}

	const bool bPickupWasActive = (ActiveSlotIndex == PickupSlotActiveIndex);

	if (bPickupWasActive)
	{
		UnequipActiveWeapon();
	}

	PickupSlot = Item;
	OnRep_PickupSlot();

	if (bPickupWasActive)
	{
		EquipActiveWeapon();
		BroadcastActiveSlotChanged();
	}
}

void UMYSTWeaponBarComponent::ClearPickupWeapon()
{
	const bool bPickupWasActive = (ActiveSlotIndex == PickupSlotActiveIndex);

	PickupSlot = nullptr;
	OnRep_PickupSlot();

	if (bPickupWasActive)
	{
		UnequipActiveWeapon();

		// Fall back to the first occupied fixed slot; fully deactivate if none found.
		int32 FallbackIndex = INDEX_NONE;
		for (int32 i = 0; i < FixedSlots.Num(); ++i)
		{
			if (FixedSlots[i] != nullptr)
			{
				FallbackIndex = i;
				break;
			}
		}

		ActiveSlotIndex = FallbackIndex;
		EquipActiveWeapon();

		OnRep_ActiveSlotIndex();
		BroadcastActiveSlotChanged();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Active slot selection (Server RPCs)
// ─────────────────────────────────────────────────────────────────────────────

void UMYSTWeaponBarComponent::SetActiveFixedSlot_Implementation(int32 SlotIndex)
{
	if (!FixedSlots.IsValidIndex(SlotIndex) || FixedSlots[SlotIndex] == nullptr)
	{
		return;
	}

	ApplyActiveSlotChange(SlotIndex);
}

void UMYSTWeaponBarComponent::ActivatePickupSlot_Implementation()
{
	if (PickupSlot == nullptr)
	{
		return;
	}

	ApplyActiveSlotChange(PickupSlotActiveIndex);
}

void UMYSTWeaponBarComponent::ClearActiveSlot_Implementation()
{
	ApplyActiveSlotChange(INDEX_NONE);
}

// ─────────────────────────────────────────────────────────────────────────────
// Cycling
// ─────────────────────────────────────────────────────────────────────────────

void UMYSTWeaponBarComponent::CycleWeaponForward()
{
	const TArray<int32> AllIndices = GetAllNonEmptySlotIndices();
	if (AllIndices.Num() < 2)
	{
		return;
	}

	const int32 CurrentPos = AllIndices.Find(ActiveSlotIndex);
	const int32 NextPos = (CurrentPos == INDEX_NONE)
		? 0
		: (CurrentPos + 1) % AllIndices.Num();
	const int32 NextIdx = AllIndices[NextPos];

	if (NextIdx == PickupSlotActiveIndex)
	{
		ActivatePickupSlot();
	}
	else
	{
		SetActiveFixedSlot(NextIdx);
	}
}

void UMYSTWeaponBarComponent::CycleWeaponBackward()
{
	const TArray<int32> AllIndices = GetAllNonEmptySlotIndices();
	if (AllIndices.Num() < 2)
	{
		return;
	}

	const int32 CurrentPos = AllIndices.Find(ActiveSlotIndex);
	const int32 PrevPos = (CurrentPos <= 0)
		? AllIndices.Num() - 1
		: CurrentPos - 1;
	const int32 PrevIdx = AllIndices[PrevPos];

	if (PrevIdx == PickupSlotActiveIndex)
	{
		ActivatePickupSlot();
	}
	else
	{
		SetActiveFixedSlot(PrevIdx);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────────────────────

ULyraInventoryItemInstance* UMYSTWeaponBarComponent::GetActiveWeapon() const
{
	if (ActiveSlotIndex == PickupSlotActiveIndex)
	{
		return PickupSlot;
	}
	if (FixedSlots.IsValidIndex(ActiveSlotIndex))
	{
		return FixedSlots[ActiveSlotIndex];
	}
	return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// RepNotify handlers
// ─────────────────────────────────────────────────────────────────────────────

void UMYSTWeaponBarComponent::OnRep_FixedSlots()
{
	BroadcastSlotsChanged();
}

void UMYSTWeaponBarComponent::OnRep_PickupSlot()
{
	BroadcastSlotsChanged();
}

void UMYSTWeaponBarComponent::OnRep_ActiveSlotIndex()
{
	BroadcastActiveSlotChanged();
}

// ─────────────────────────────────────────────────────────────────────────────
// Equip / unequip helpers
// ─────────────────────────────────────────────────────────────────────────────

void UMYSTWeaponBarComponent::EquipActiveWeapon()
{
	// Guard: should never be called while something is already equipped.
	ensure(EquippedItem == nullptr);

	ULyraInventoryItemInstance* ActiveItem = GetActiveWeapon();
	if (!IsValid(ActiveItem))
	{
		return;
	}

	const UInventoryFragment_EquippableItem* EquipInfo =
		ActiveItem->FindFragmentByClass<UInventoryFragment_EquippableItem>();

	if (!EquipInfo || !EquipInfo->EquipmentDefinition)
	{
		// Item exists but has no equippable fragment — treat as successfully "equipped"
		// in the slot sense even if no actual equipment instance spawns.
		return;
	}

	ULyraEquipmentManagerComponent* EquipManager = FindEquipmentManager();
	if (!EquipManager)
	{
		return;
	}

	EquippedItem = EquipManager->EquipItem(EquipInfo->EquipmentDefinition);
	if (EquippedItem)
	{
		EquippedItem->SetInstigator(ActiveItem);
	}
}

void UMYSTWeaponBarComponent::UnequipActiveWeapon()
{
	if (!EquippedItem)
	{
		return;
	}

	ULyraEquipmentManagerComponent* EquipManager = FindEquipmentManager();
	if (EquipManager)
	{
		EquipManager->UnequipItem(EquippedItem);
	}
	EquippedItem = nullptr;
}

ULyraEquipmentManagerComponent* UMYSTWeaponBarComponent::FindEquipmentManager() const
{
	if (const AController* MyController = Cast<AController>(GetOwner()))
	{
		if (const APawn* Pawn = MyController->GetPawn())
		{
			return Pawn->FindComponentByClass<ULyraEquipmentManagerComponent>();
		}
	}
	return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

TArray<int32> UMYSTWeaponBarComponent::GetAllNonEmptySlotIndices() const
{
	TArray<int32> Indices;
	for (int32 i = 0; i < FixedSlots.Num(); ++i)
	{
		if (FixedSlots[i] != nullptr)
		{
			Indices.Add(i);
		}
	}
	if (PickupSlot != nullptr)
	{
		Indices.Add(PickupSlotActiveIndex);
	}
	return Indices;
}

void UMYSTWeaponBarComponent::ApplyActiveSlotChange(int32 NewSlotIndex)
{
	if (ActiveSlotIndex == NewSlotIndex)
	{
		return;
	}

	UnequipActiveWeapon();
	ActiveSlotIndex = NewSlotIndex;
	EquipActiveWeapon();

	// Notify both server-local listeners and clients via RepNotify.
	OnRep_ActiveSlotIndex();
	BroadcastActiveSlotChanged();
}

void UMYSTWeaponBarComponent::BroadcastSlotsChanged()
{
	FMYSTWeaponBarSlotsChangedMessage Message;
	Message.Owner      = GetOwner();
	Message.FixedSlots = FixedSlots;
	Message.PickupSlot = PickupSlot;

	UGameplayMessageSubsystem& MsgSystem = UGameplayMessageSubsystem::Get(this);
	MsgSystem.BroadcastMessage(MYSTGameplayTags::MYST_WeaponBar_Message_SlotsChanged, Message);

	// Also fire the direct implementable event so BP subclasses don't need to
	// subscribe to the GameplayMessage subsystem for basic HUD refresh work.
	K2_OnSlotsChanged(GetFixedSlots(), PickupSlot);
}

void UMYSTWeaponBarComponent::BroadcastActiveSlotChanged()
{
	ULyraInventoryItemInstance* Active = GetActiveWeapon();

	FMYSTWeaponBarActiveSlotChangedMessage Message;
	Message.Owner           = GetOwner();
	Message.ActiveSlotIndex = ActiveSlotIndex;
	Message.ActiveWeapon    = Active;

	UGameplayMessageSubsystem& MsgSystem = UGameplayMessageSubsystem::Get(this);
	MsgSystem.BroadcastMessage(MYSTGameplayTags::MYST_WeaponBar_Message_ActiveSlotChanged, Message);

	// Also fire the direct implementable event.
	K2_OnActiveSlotChanged(ActiveSlotIndex, Active);
}


