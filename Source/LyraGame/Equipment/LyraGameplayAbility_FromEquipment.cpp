// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraGameplayAbility_FromEquipment.h"
#include "LyraEquipmentInstance.h"
#include "Inventory/LyraInventoryItemInstance.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraGameplayAbility_FromEquipment)

ULyraGameplayAbility_FromEquipment::ULyraGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

ULyraEquipmentInstance* ULyraGameplayAbility_FromEquipment::GetAssociatedEquipment() const
{
	// This helper is used by cost checks and other pre-activation paths.
	// Those paths can run on the CDO (or on non-instanced abilities) where GetCurrentAbilitySpec() is invalid.
	if (!IsInstantiated())
	{
		// If we’re not instantiated, we can’t safely read CurrentSpecHandle.
		// Callers should use handle+actor info based access instead.
		return nullptr;
	}

	if (FGameplayAbilitySpec* Spec = UGameplayAbility::GetCurrentAbilitySpec())
	{
		return Cast<ULyraEquipmentInstance>(Spec->SourceObject.Get());
	}

	return nullptr;
}

ULyraInventoryItemInstance* ULyraGameplayAbility_FromEquipment::GetAssociatedItem() const
{
	if (ULyraEquipmentInstance* Equipment = GetAssociatedEquipment())
	{
		return Cast<ULyraInventoryItemInstance>(Equipment->GetInstigator());
	}
	return nullptr;
}


#if WITH_EDITOR
EDataValidationResult ULyraGameplayAbility_FromEquipment::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (InstancingPolicy != EGameplayAbilityInstancingPolicy::InstancedPerActor)
	{
		Context.AddError(NSLOCTEXT("Lyra", "EquipmentAbilityMustBeInstanced", "Equipment ability must be instanced"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif
