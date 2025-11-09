// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "LyraGameplayEffectBlueprintLibrary.generated.h"

/**
 * ULyraGameplayEffectBlueprintLibrary
 *
 * Blueprint function library to expose GameplayEffect data that is normally hard to access in Blueprints
 */
UCLASS()
class LYRAGAME_API ULyraGameplayEffectBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Get the magnitude of a specific modifier in a GameplayEffect by attribute (using Class) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|GameplayEffect", meta = (DisplayName = "Get Modifier Magnitude By Attribute (Class)"))
	static bool GetModifierMagnitudeByAttributeFromClass(TSubclassOf<UGameplayEffect> GameplayEffectClass, FGameplayAttribute Attribute, float& OutMinMagnitude, float& OutMaxMagnitude);

	/** Get the magnitude of a specific modifier in a GameplayEffect by attribute (using Instance) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|GameplayEffect", meta = (DisplayName = "Get Modifier Magnitude By Attribute (Instance)"))
	static bool GetModifierMagnitudeByAttribute(const UGameplayEffect* GameplayEffect, FGameplayAttribute Attribute, float& OutMinMagnitude, float& OutMaxMagnitude);

	/** Get all GameplayCue tags from a GameplayEffect (using Class) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|GameplayEffect", meta = (DisplayName = "Get Gameplay Cue Tags (Class)"))
	static TArray<FGameplayTag> GetGameplayCueTagsFromClass(TSubclassOf<UGameplayEffect> GameplayEffectClass);

	/** Get all GameplayCue tags from a GameplayEffect (using Instance) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|GameplayEffect", meta = (DisplayName = "Get Gameplay Cue Tags (Instance)"))
	static TArray<FGameplayTag> GetGameplayCueTags(const UGameplayEffect* GameplayEffect);

	/** Get the duration of a GameplayEffect (using Class) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|GameplayEffect", meta = (DisplayName = "Get Gameplay Effect Duration (Class)"))
	static float GetGameplayEffectDurationFromClass(TSubclassOf<UGameplayEffect> GameplayEffectClass);

	/** Get the duration of a GameplayEffect (using Instance) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|GameplayEffect", meta = (DisplayName = "Get Gameplay Effect Duration (Instance)"))
	static float GetGameplayEffectDuration(const UGameplayEffect* GameplayEffect);

	/** Get the period of a GameplayEffect (using Class) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|GameplayEffect", meta = (DisplayName = "Get Gameplay Effect Period (Class)"))
	static float GetGameplayEffectPeriodFromClass(TSubclassOf<UGameplayEffect> GameplayEffectClass);

	/** Get the period of a GameplayEffect (using Instance) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|GameplayEffect", meta = (DisplayName = "Get Gameplay Effect Period (Instance)"))
	static float GetGameplayEffectPeriod(const UGameplayEffect* GameplayEffect);

	/** Get the base damage value from a GameplayEffect that uses LyraDamageExecution (Class version) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|GameplayEffect|Damage", meta = (DisplayName = "Get Base Damage From Damage Effect (Class)"))
	static bool GetBaseDamageFromDamageEffectClass(TSubclassOf<UGameplayEffect> GameplayEffectClass, float& OutBaseDamage);

	/** Get the base damage value from a GameplayEffect that uses LyraDamageExecution (Instance version) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lyra|GameplayEffect|Damage", meta = (DisplayName = "Get Base Damage From Damage Effect (Instance)"))
	static bool GetBaseDamageFromDamageEffect(const UGameplayEffect* GameplayEffect, float& OutBaseDamage);
};

