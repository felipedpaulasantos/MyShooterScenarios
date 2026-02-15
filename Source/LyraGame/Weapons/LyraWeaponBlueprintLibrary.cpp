// Copyright (c) 2025

#include "Weapons/LyraWeaponBlueprintLibrary.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Weapons/LyraWeaponInstance.h"
#include "Character/LyraCharacter.h"
#include "AbilitySystemComponent.h"
#include "Weapons/LyraRangedWeaponInstance.h"
#include "Player/LyraPlayerState.h"
#include "GameplayTagsManager.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Weapons/LyraWeaponSpawner.h"

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

bool ULyraWeaponBlueprintLibrary::AddAmmoToWeaponByClass(ALyraCharacter* LyraCharacter, TSubclassOf<ULyraRangedWeaponInstance> WeaponClass, float AmmoToAdd, ULyraRangedWeaponInstance*& OutWeaponInstance, float& OutActualAmmoAdded)
{
	OutWeaponInstance = nullptr;
	OutActualAmmoAdded = 0.0f;

	if (!IsValid(LyraCharacter) || !WeaponClass)
	{
		return false;
	}

	if (!LyraCharacter->HasAuthority())
	{
		// Must be authoritative to change stats/ammo.
		return false;
	}

	if (AmmoToAdd <= 0.0f)
	{
		return false;
	}

	ULyraEquipmentManagerComponent* EquipmentManager = LyraCharacter->FindComponentByClass<ULyraEquipmentManagerComponent>();
	if (!EquipmentManager)
	{
		return false;
	}

	ULyraRangedWeaponInstance* FoundWeapon = nullptr;
	const TArray<ULyraEquipmentInstance*> RangedInstances = EquipmentManager->GetEquipmentInstancesOfType(ULyraRangedWeaponInstance::StaticClass());
	for (ULyraEquipmentInstance* Instance : RangedInstances)
	{
		ULyraRangedWeaponInstance* Ranged = Cast<ULyraRangedWeaponInstance>(Instance);
		if (Ranged && Ranged->IsA(WeaponClass))
		{
			FoundWeapon = Ranged;
			break;
		}
	}

	if (!FoundWeapon)
	{
		return false;
	}

	OutWeaponInstance = FoundWeapon;

	ALyraPlayerState* LyraPS = LyraCharacter->GetPlayerState<ALyraPlayerState>();
	if (!LyraPS)
	{
		return false;
	}

	// The reload ability uses stack counts on these 3 tags:
	// - Lyra.ShooterGame.Weapon.MagazineAmmo
	// - Lyra.ShooterGame.Weapon.MagazineSize
	// - Lyra.ShooterGame.Weapon.SpareAmmo
	// We only add ammo to SpareAmmo (player must reload manually).
	const FGameplayTag SpareAmmoTag = UGameplayTagsManager::Get().RequestGameplayTag(FName("Lyra.ShooterGame.Weapon.SpareAmmo"), /*ErrorIfNotFound=*/ false);
	if (!SpareAmmoTag.IsValid())
	{
		return false;
	}

	const int32 CurrentSpareAmmo = LyraPS->GetStatTagStackCount(SpareAmmoTag);

	// We interpret the input float as "ammo units" and round to the nearest whole number.
	const int32 RequestedAdd = FMath::RoundToInt(AmmoToAdd);
	if (RequestedAdd <= 0)
	{
		return false;
	}

	int32 DesiredNewSpareAmmo = CurrentSpareAmmo + RequestedAdd;

	// Lyra's ShooterCore treats the default SpareAmmo stat defined on the weapon's inventory item definition
	// as the *max* spare ammo (there is no separate MaxSpareAmmo stat).
	if (const ULyraInventoryItemInstance* WeaponItemInstance = Cast<ULyraInventoryItemInstance>(FoundWeapon->GetInstigator()))
	{
		const TSubclassOf<ULyraInventoryItemDefinition> WeaponItemDef = WeaponItemInstance->GetItemDef();
		const int32 DefaultSpareAmmo = ALyraWeaponSpawner::GetDefaultStatFromItemDef(WeaponItemDef, SpareAmmoTag);
		if (DefaultSpareAmmo > 0)
		{
			DesiredNewSpareAmmo = FMath::Min(DesiredNewSpareAmmo, DefaultSpareAmmo);
		}
	}

	DesiredNewSpareAmmo = FMath::Max(DesiredNewSpareAmmo, 0);

	const int32 ActualAdded = DesiredNewSpareAmmo - CurrentSpareAmmo;
	OutActualAmmoAdded = static_cast<float>(ActualAdded);
	if (ActualAdded <= 0)
	{
		return false;
	}

	LyraPS->AddStatTagStack(SpareAmmoTag, ActualAdded);
	return true;
}
