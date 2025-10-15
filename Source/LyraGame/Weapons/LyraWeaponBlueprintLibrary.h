// Copyright (c) 2025
#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "LyraWeaponBlueprintLibrary.generated.h"

class AActor;
class ULyraWeaponInstance;

/**
 * Blueprint helpers for Lyra weapons.
 */
UCLASS()
class ULyraWeaponBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Returns true if the specified actor (pawn or controller owning a pawn) currently has any Lyra weapon instance equipped.
	 * Optionally returns the first weapon instance found (e.g., a Blueprint like B_WeaponInstanceBase derived from ULyraWeaponInstance).
	 *
	 * - Actor: Defaults to self in Blueprints. Can be a Pawn/Character or a Controller. If a Controller is provided, its Pawn is used.
	 * - OutWeaponInstance: If a weapon is found, this will be set to the first ULyraWeaponInstance; otherwise nullptr.
	 */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Weapons", meta=(DefaultToSelf="Actor", ExpandBoolAsExecs="ReturnValue", DisplayName="Has Equipped Weapon", Keywords="Lyra Weapon Equipped HasWeapon CurrentWeapon B_WeaponInstanceBase"))
	static bool HasEquippedWeapon(const AActor* Actor, ULyraWeaponInstance*& OutWeaponInstance);

	/**
	 * Returns true if the specified actor currently has an equipped weapon that is a child of the provided InstanceClass.
	 * Useful to directly check for a specific Blueprint class such as B_WeaponInstanceBase.
	 */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Weapons", meta=(DefaultToSelf="Actor", ExpandBoolAsExecs="ReturnValue", DisplayName="Has Equipped Weapon Of Class", Keywords="Lyra Weapon Equipped HasWeapon Class B_WeaponInstanceBase"))
	static bool HasEquippedWeaponOfClass(const AActor* Actor, TSubclassOf<ULyraWeaponInstance> InstanceClass, ULyraWeaponInstance*& OutWeaponInstance);
};
