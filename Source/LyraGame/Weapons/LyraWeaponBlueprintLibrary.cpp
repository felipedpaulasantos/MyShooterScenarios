// Copyright (c) 2025

#include "Weapons/LyraWeaponBlueprintLibrary.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Weapons/LyraWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraWeaponBlueprintLibrary)

bool ULyraWeaponBlueprintLibrary::HasEquippedWeapon(const AActor* Actor, ULyraWeaponInstance*& OutWeaponInstance)
{
	OutWeaponInstance = nullptr;

	if (!IsValid(Actor))
	{
		return false;
	}

	const APawn* Pawn = Cast<APawn>(Actor);
	if (!Pawn)
	{
		if (const AController* Controller = Cast<AController>(Actor))
		{
			Pawn = Controller->GetPawn();
		}
	}
	// As a fallback, try the instigator if not found yet
	if (!Pawn)
	{
		Pawn = Actor->GetInstigator();
	}

	if (!IsValid(Pawn))
	{
		return false;
	}

	if (ULyraEquipmentManagerComponent* EquipmentManager = Pawn->FindComponentByClass<ULyraEquipmentManagerComponent>())
	{
		if (ULyraWeaponInstance* FoundWeapon = EquipmentManager->GetFirstInstanceOfType<ULyraWeaponInstance>())
		{
			OutWeaponInstance = FoundWeapon;
			return true;
		}
	}

	return false;
}

bool ULyraWeaponBlueprintLibrary::HasEquippedWeaponOfClass(const AActor* Actor, TSubclassOf<ULyraWeaponInstance> InstanceClass, ULyraWeaponInstance*& OutWeaponInstance)
{
	OutWeaponInstance = nullptr;

	if (!IsValid(Actor))
	{
		return false;
	}

	const APawn* Pawn = Cast<APawn>(Actor);
	if (!Pawn)
	{
		if (const AController* Controller = Cast<AController>(Actor))
		{
			Pawn = Controller->GetPawn();
		}
	}
	if (!Pawn)
	{
		Pawn = Actor->GetInstigator();
	}

	if (!IsValid(Pawn))
	{
		return false;
	}

	if (ULyraEquipmentManagerComponent* EquipmentManager = Pawn->FindComponentByClass<ULyraEquipmentManagerComponent>())
	{
		// If no class was provided, fall back to checking for any weapon
		if (!InstanceClass)
		{
			return HasEquippedWeapon(Pawn, OutWeaponInstance);
		}

		if (ULyraEquipmentInstance* FoundInstance = EquipmentManager->GetFirstInstanceOfType(InstanceClass))
		{
			OutWeaponInstance = Cast<ULyraWeaponInstance>(FoundInstance);
			return OutWeaponInstance != nullptr;
		}
	}

	return false;
}
