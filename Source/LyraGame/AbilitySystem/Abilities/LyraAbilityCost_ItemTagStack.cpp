// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraAbilityCost_ItemTagStack.h"

#include "AbilitySystemComponent.h"
#include "Equipment/LyraEquipmentInstance.h"
#include "Equipment/LyraGameplayAbility_FromEquipment.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "NativeGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraAbilityCost_ItemTagStack)

UE_DEFINE_GAMEPLAY_TAG(TAG_ABILITY_FAIL_COST, "Ability.ActivateFail.Cost");

ULyraAbilityCost_ItemTagStack::ULyraAbilityCost_ItemTagStack()
{
	Quantity.SetValue(1.0f);
	FailureTag = TAG_ABILITY_FAIL_COST;
}

bool ULyraAbilityCost_ItemTagStack::CheckCost(const ULyraGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// Cost checks happen in pre-activation paths; don't assume the ability is instantiated.
	if (!Ability || !ActorInfo)
	{
		return false;
	}

	ULyraInventoryItemInstance* ItemInstance = nullptr;

	if (const ULyraGameplayAbility_FromEquipment* EquipmentAbility = Cast<const ULyraGameplayAbility_FromEquipment>(Ability))
	{
		// Prefer resolving from the spec instead of using GetCurrentAbilitySpec(), which is invalid on the CDO.
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle))
			{
				if (ULyraEquipmentInstance* Equipment = Cast<ULyraEquipmentInstance>(Spec->SourceObject.Get()))
				{
					ItemInstance = Cast<ULyraInventoryItemInstance>(Equipment->GetInstigator());
				}
			}
		}

		// Fallback: if the ability is instantiated, the helper can work.
		if (!ItemInstance)
		{
			ItemInstance = EquipmentAbility->GetAssociatedItem();
		}
	}

	if (ItemInstance)
	{
		const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

		const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
		const int32 NumStacks = FMath::TruncToInt(NumStacksReal);
		const bool bCanApplyCost = ItemInstance->GetStatTagStackCount(Tag) >= NumStacks;

		// Inform other abilities why this cost cannot be applied
		if (!bCanApplyCost && OptionalRelevantTags && FailureTag.IsValid())
		{
			OptionalRelevantTags->AddTag(FailureTag);
		}
		return bCanApplyCost;
	}

	return false;
}

void ULyraAbilityCost_ItemTagStack::ApplyCost(const ULyraGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (ActorInfo->IsNetAuthority())
	{
		if (const ULyraGameplayAbility_FromEquipment* EquipmentAbility = Cast<const ULyraGameplayAbility_FromEquipment>(Ability))
		{
			if (ULyraInventoryItemInstance* ItemInstance = EquipmentAbility->GetAssociatedItem())
			{
				const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

				const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
				const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

				ItemInstance->RemoveStatTagStack(Tag, NumStacks);
			}
		}
	}
}
