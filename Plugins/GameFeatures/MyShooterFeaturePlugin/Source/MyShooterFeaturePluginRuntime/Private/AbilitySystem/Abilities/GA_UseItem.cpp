// Copyright MyShooterScenarios. All Rights Reserved.

#include "AbilitySystem/Abilities/GA_UseItem.h"

#include "Inventory/InventoryFragment_UsableItem.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryManagerComponent.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Math/UnrealMathUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GA_UseItem)

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

UGA_UseItem::UGA_UseItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Must be instanced per actor so CurrentActorInfo / CurrentSpecHandle are
	// reliably populated for the cooldown tag look-up and montage callbacks.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

// ─────────────────────────────────────────────────────────────────────────────
// UGameplayAbility overrides
// ─────────────────────────────────────────────────────────────────────────────

bool UGA_UseItem::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// Respect any additional costs configured on the CDO (base class checks AdditionalCosts).
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	// We additionally require at least one usable item in inventory.
	return FindBestUsableItemFromActorInfo(ActorInfo) != nullptr;
}

void UGA_UseItem::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// Apply any static additional costs (base class).
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

	// Item consumption is authority-only.
	if (!ActorInfo->IsNetAuthority())
	{
		return;
	}

	ULyraInventoryItemInstance* Item = FindBestUsableItemFromActorInfo(ActorInfo);
	if (!Item)
	{
		return;
	}

	const UInventoryFragment_UsableItem* Fragment = GetUsableFragment(Item);
	if (!Fragment)
	{
		return;
	}

	ULyraInventoryManagerComponent* Inventory = GetInventoryFromActorInfo(ActorInfo);
	if (!Inventory)
	{
		return;
	}

	const int32 AbilityLevel   = GetAbilityLevel(Handle, ActorInfo);
	const int32 StacksToRemove = FMath::Max(1, FMath::TruncToInt(Fragment->StacksConsumedPerUse.GetValueAtLevel(AbilityLevel)));

	Inventory->ConsumeItemsByDefinition(Item->GetItemDef(), StacksToRemove);
}

const FGameplayTagContainer* UGA_UseItem::GetCooldownTags() const
{
	// Rebuild the cache every call (cheap — tag container copy + one optional tag add).
	FGameplayTagContainer* MutableCache = const_cast<FGameplayTagContainer*>(&CooldownTagsCache);
	MutableCache->Reset();

	// Include any tags from the configured CooldownGameplayEffectClass (base class).
	if (const FGameplayTagContainer* ParentTags = Super::GetCooldownTags())
	{
		MutableCache->AppendTags(*ParentTags);
	}

	// Dynamically append the fragment's CooldownTag for the best item in inventory.
	// GetCurrentActorInfo() is valid for InstancedPerActor abilities after OnGiveAbility.
	if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
	{
		if (const ULyraInventoryItemInstance* Item = FindBestUsableItemFromActorInfo(ActorInfo))
		{
			if (const UInventoryFragment_UsableItem* Fragment = GetUsableFragment(Item))
			{
				if (Fragment->CooldownTag.IsValid())
				{
					MutableCache->AddTag(Fragment->CooldownTag);
				}
			}
		}
	}

	return MutableCache->IsEmpty() ? nullptr : MutableCache;
}

// ─────────────────────────────────────────────────────────────────────────────
// Activate / deactivate
// ─────────────────────────────────────────────────────────────────────────────

void UGA_UseItem::ActivateAbility(
	const FGameplayAbilitySpecHandle   Handle,
	const FGameplayAbilityActorInfo*   ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData*          TriggerEventData)
{
	// ── 1. Find item and fragment ─────────────────────────────────────────────
	ULyraInventoryItemInstance* Item = FindBestUsableItemFromActorInfo(ActorInfo);
	if (!Item)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	const UInventoryFragment_UsableItem* Fragment = GetUsableFragment(Item);
	if (!Fragment)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ── 2. Commit cost + cooldown ─────────────────────────────────────────────
	// This calls our overridden CheckCost / ApplyCost / ApplyCooldown.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ── 3. Apply the gameplay effect to the avatar's ASC ──────────────────────
	if (Fragment->EffectToApply && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const FGameplayEffectSpecHandle SpecHandle =
			MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, Fragment->EffectToApply);

		if (SpecHandle.IsValid())
		{
			// Return value (FActiveGameplayEffectHandle) intentionally discarded;
			// we don't need to track this transient effect.
			(void)ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}

	// ── 4. Notify Blueprint subclasses (add VFX / sounds / screen effects) ────
	K2_OnItemEffectApplied(Item);

	// ── 5. Play cosmetic montage (optional) ───────────────────────────────────
	if (Fragment->UseMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this,
				NAME_None,            // TaskInstanceName
				Fragment->UseMontage,
				/*Rate=*/1.0f,
				NAME_None,            // StartSection
				/*bStopWhenAbilityEnds=*/true);

		// All four delegates call EndAbility; multiple calls are guarded by IsEndAbilityValid.
		MontageTask->OnCompleted.AddDynamic(this, &UGA_UseItem::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_UseItem::OnMontageBlendOut);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_UseItem::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_UseItem::OnMontageCancelled);

		MontageTask->ReadyForActivation();
		// Ability stays alive until a montage callback fires.
	}
	else
	{
		// No montage — effect was applied instantly, we're done.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Montage task callbacks
// ─────────────────────────────────────────────────────────────────────────────

void UGA_UseItem::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}

void UGA_UseItem::OnMontageBlendOut()
{
	// Blend-out is part of natural completion; end cleanly.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_UseItem::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, /*bWasCancelled=*/true);
}

void UGA_UseItem::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

// ─────────────────────────────────────────────────────────────────────────────
// Blueprint-callable helpers
// ─────────────────────────────────────────────────────────────────────────────

ULyraInventoryItemInstance* UGA_UseItem::GetBestUsableItem() const
{
	return FindBestUsableItemFromActorInfo(GetCurrentActorInfo());
}

/*static*/ const UInventoryFragment_UsableItem* UGA_UseItem::GetUsableFragment(const ULyraInventoryItemInstance* ItemInstance)
{
	if (!ItemInstance)
	{
		return nullptr;
	}
	return ItemInstance->FindFragmentByClass<UInventoryFragment_UsableItem>();
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ ULyraInventoryManagerComponent* UGA_UseItem::GetInventoryFromActorInfo(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!ActorInfo)
	{
		return nullptr;
	}

	// Primary: player controller has the inventory component in Lyra.
	if (AController* PC = ActorInfo->PlayerController.Get())
	{
		if (ULyraInventoryManagerComponent* Inventory = PC->GetComponentByClass<ULyraInventoryManagerComponent>())
		{
			return Inventory;
		}
	}

	// Fallback: try to resolve controller from the avatar pawn.
	if (APawn* Pawn = Cast<APawn>(ActorInfo->AvatarActor.Get()))
	{
		if (AController* PC = Pawn->GetController())
		{
			return PC->GetComponentByClass<ULyraInventoryManagerComponent>();
		}
	}

	return nullptr;
}

ULyraInventoryItemInstance* UGA_UseItem::FindBestUsableItemFromActorInfo(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const ULyraInventoryManagerComponent* Inventory = GetInventoryFromActorInfo(ActorInfo);
	if (!Inventory)
	{
		return nullptr;
	}

	for (ULyraInventoryItemInstance* Instance : Inventory->GetAllItems())
	{
		if (!IsValid(Instance))
		{
			continue;
		}

		if (Instance->FindFragmentByClass<UInventoryFragment_UsableItem>() != nullptr)
		{
			return Instance;
		}
	}

	return nullptr;
}




