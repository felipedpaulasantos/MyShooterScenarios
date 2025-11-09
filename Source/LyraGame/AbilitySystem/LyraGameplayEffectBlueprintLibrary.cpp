// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraGameplayEffectBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectExecutionCalculation.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "AbilitySystem/Executions/LyraDamageExecution.h"

DEFINE_LOG_CATEGORY_STATIC(LogLyraGEBlueprintLib, Log, All);

bool ULyraGameplayEffectBlueprintLibrary::GetModifierMagnitudeByAttributeFromClass(TSubclassOf<UGameplayEffect> GameplayEffectClass, FGameplayAttribute Attribute, float& OutMinMagnitude, float& OutMaxMagnitude)
{
	if (!GameplayEffectClass)
	{
		OutMinMagnitude = 0.0f;
		OutMaxMagnitude = 0.0f;
		return false;
	}

	// Get the Class Default Object (CDO)
	const UGameplayEffect* GameplayEffectCDO = GameplayEffectClass->GetDefaultObject<UGameplayEffect>();
	
	return GetModifierMagnitudeByAttribute(GameplayEffectCDO, Attribute, OutMinMagnitude, OutMaxMagnitude);
}

bool ULyraGameplayEffectBlueprintLibrary::GetModifierMagnitudeByAttribute(const UGameplayEffect* GameplayEffect, FGameplayAttribute Attribute, float& OutMinMagnitude, float& OutMaxMagnitude)
{
	OutMinMagnitude = 0.0f;
	OutMaxMagnitude = 0.0f;
	
	if (!GameplayEffect || !Attribute.IsValid())
	{
		return false;
	}

	// Validate the object
	if (!IsValid(GameplayEffect))
	{
		UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("GetModifierMagnitudeByAttribute: GameplayEffect is not valid"));
		return false;
	}

	// Log debug info
	UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("Searching for attribute %s (Owner: %s) in GameplayEffect %s"), 
		*Attribute.GetName(), 
		Attribute.GetAttributeSetClass() ? *Attribute.GetAttributeSetClass()->GetName() : TEXT("None"),
		*GameplayEffect->GetName());
	
	UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("  GameplayEffect has %d Modifiers and %d Executions"), 
		GameplayEffect->Modifiers.Num(), 
		GameplayEffect->Executions.Num());

	// Search through all modifiers
	for (int32 i = 0; i < GameplayEffect->Modifiers.Num(); ++i)
	{
		const FGameplayModifierInfo& Modifier = GameplayEffect->Modifiers[i];
		
		UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("  Modifier[%d]: Attribute=%s (Owner: %s)"), 
			i,
			*Modifier.Attribute.GetName(),
			Modifier.Attribute.GetAttributeSetClass() ? *Modifier.Attribute.GetAttributeSetClass()->GetName() : TEXT("None"));
		
		if (Modifier.Attribute == Attribute)
		{
			UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("Found modifier for attribute %s in GameplayEffect %s"), 
				*Attribute.GetName(), *GameplayEffect->GetName());
			
			// We found the attribute, but we cannot safely extract the value without runtime context
			// Return true to indicate success, but values remain 0
			UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("Found attribute but cannot extract magnitude safely without full runtime context (ASC, Source, Target, etc)"));
			return true;
		}
	}

	UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("Attribute %s not found in GameplayEffect %s Modifiers. This GameplayEffect likely uses Executions instead."), 
		*Attribute.GetName(), *GameplayEffect->GetName());
	
	// Log execution classes if any
	if (GameplayEffect->Executions.Num() > 0)
	{
		UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("  GameplayEffect has Execution Calculations:"));
		for (int32 i = 0; i < GameplayEffect->Executions.Num(); ++i)
		{
			if (GameplayEffect->Executions[i].CalculationClass)
			{
				UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("    [%d] %s"), i, *GameplayEffect->Executions[i].CalculationClass->GetName());
			}
		}
	}
	
	return false;
}

TArray<FGameplayTag> ULyraGameplayEffectBlueprintLibrary::GetGameplayCueTagsFromClass(TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	if (!GameplayEffectClass)
	{
		return TArray<FGameplayTag>();
	}

	const UGameplayEffect* GameplayEffectCDO = GameplayEffectClass->GetDefaultObject<UGameplayEffect>();
	return GetGameplayCueTags(GameplayEffectCDO);
}

TArray<FGameplayTag> ULyraGameplayEffectBlueprintLibrary::GetGameplayCueTags(const UGameplayEffect* GameplayEffect)
{
	TArray<FGameplayTag> GameplayCueTags;
	
	if (!GameplayEffect)
	{
		return GameplayCueTags;
	}

	// Get all GameplayCue tags from the effect
	for (const FGameplayEffectCue& Cue : GameplayEffect->GameplayCues)
	{
		TArray<FGameplayTag> Tags;
		Cue.GameplayCueTags.GetGameplayTagArray(Tags);
		GameplayCueTags.Append(Tags);
	}

	return GameplayCueTags;
}

float ULyraGameplayEffectBlueprintLibrary::GetGameplayEffectDurationFromClass(TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	if (!GameplayEffectClass)
	{
		return 0.0f;
	}

	const UGameplayEffect* GameplayEffectCDO = GameplayEffectClass->GetDefaultObject<UGameplayEffect>();
	return GetGameplayEffectDuration(GameplayEffectCDO);
}

float ULyraGameplayEffectBlueprintLibrary::GetGameplayEffectDuration(const UGameplayEffect* GameplayEffect)
{
	if (!GameplayEffect)
	{
		return 0.0f;
	}

	// Duration policy check
	if (GameplayEffect->DurationPolicy == EGameplayEffectDurationType::Instant)
	{
		return 0.0f;
	}
	else if (GameplayEffect->DurationPolicy == EGameplayEffectDurationType::Infinite)
	{
		return -1.0f; // Use -1 to indicate infinite
	}

	// For HasDuration type, would need to evaluate the magnitude
	// This is simplified - in practice you'd need runtime evaluation
	return 0.0f;
}

float ULyraGameplayEffectBlueprintLibrary::GetGameplayEffectPeriodFromClass(TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	if (!GameplayEffectClass)
	{
		return 0.0f;
	}

	const UGameplayEffect* GameplayEffectCDO = GameplayEffectClass->GetDefaultObject<UGameplayEffect>();
	return GetGameplayEffectPeriod(GameplayEffectCDO);
}

float ULyraGameplayEffectBlueprintLibrary::GetGameplayEffectPeriod(const UGameplayEffect* GameplayEffect)
{
	if (!GameplayEffect)
	{
		return 0.0f;
	}

	// Period would also need runtime evaluation for complex setups
	// This is a simplified placeholder
	return 0.0f;
}

bool ULyraGameplayEffectBlueprintLibrary::GetBaseDamageFromDamageEffectClass(TSubclassOf<UGameplayEffect> GameplayEffectClass, float& OutBaseDamage)
{
	OutBaseDamage = 0.0f;
	
	if (!GameplayEffectClass)
	{
		UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("GetBaseDamageFromDamageEffectClass: GameplayEffectClass is null"));
		return false;
	}

	// Get the Class Default Object (CDO) safely
	const UGameplayEffect* GameplayEffectCDO = GameplayEffectClass.GetDefaultObject();
	
	if (!GameplayEffectCDO)
	{
		UE_LOG(LogLyraGEBlueprintLib, Error, TEXT("GetBaseDamageFromDamageEffectClass: Failed to get CDO for class %s"), 
			*GameplayEffectClass->GetName());
		return false;
	}
	
	if (!IsValid(GameplayEffectCDO))
	{
		UE_LOG(LogLyraGEBlueprintLib, Error, TEXT("GetBaseDamageFromDamageEffectClass: CDO is not valid for class %s"), 
			*GameplayEffectClass->GetName());
		return false;
	}
	
	return GetBaseDamageFromDamageEffect(GameplayEffectCDO, OutBaseDamage);
}

bool ULyraGameplayEffectBlueprintLibrary::GetBaseDamageFromDamageEffect(const UGameplayEffect* GameplayEffect, float& OutBaseDamage)
{
	OutBaseDamage = 0.0f;
	
	if (!GameplayEffect)
	{
		UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("GetBaseDamageFromDamageEffect: GameplayEffect is null"));
		return false;
	}

	// Validate the object is not pending kill or garbage
	if (!IsValid(GameplayEffect))
	{
		UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("GetBaseDamageFromDamageEffect: GameplayEffect is not valid"));
		return false;
	}

	UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("Attempting to extract BaseDamage from GameplayEffect %s"), *GameplayEffect->GetName());

	// Check if this GameplayEffect uses LyraDamageExecution
	const FGameplayEffectExecutionDefinition* DamageExecution = nullptr;
	for (const FGameplayEffectExecutionDefinition& Execution : GameplayEffect->Executions)
	{
		if (Execution.CalculationClass && Execution.CalculationClass->IsChildOf(ULyraDamageExecution::StaticClass()))
		{
			DamageExecution = &Execution;
			UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("  Found LyraDamageExecution"));
			break;
		}
	}

	if (!DamageExecution)
	{
		UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("  GameplayEffect does not use LyraDamageExecution. Cannot extract BaseDamage."));
		return false;
	}

	// LyraDamageExecution reads BaseDamage from LyraCombatSet via Captured Attribute
	// The actual damage value is set in the Execution's CalculationModifiers
	FGameplayAttribute BaseDamageAttribute = ULyraCombatSet::GetBaseDamageAttribute();
	
	if (!BaseDamageAttribute.IsValid())
	{
		UE_LOG(LogLyraGEBlueprintLib, Error, TEXT("  BaseDamage attribute is invalid!"));
		return false;
	}
	
	UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("  Execution has %d CalculationModifiers"), DamageExecution->CalculationModifiers.Num());
	
	// Check CalculationModifiers for BaseDamage
	for (const FGameplayEffectExecutionScopedModifierInfo& CalcModifier : DamageExecution->CalculationModifiers)
	{
		// Log each modifier we find
		UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("    CalcModifier: Captured Attribute from %s"), 
			CalcModifier.CapturedAttribute.AttributeSource == EGameplayEffectAttributeCaptureSource::Source ? TEXT("Source") : TEXT("Target"));
		
		// Check if this modifier targets BaseDamage
		// The CapturedAttribute should match the BaseDamage attribute definition
		if (CalcModifier.CapturedAttribute.AttributeToCapture == BaseDamageAttribute)
		{
			UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("  Found BaseDamage in CalculationModifiers"));
			
			// Try to extract the magnitude using reflection to safely access private members
			const FGameplayEffectModifierMagnitude& ModMagnitude = CalcModifier.ModifierMagnitude;
			
			// Use reflection to determine the magnitude calculation type
			UScriptStruct* MagnitudeStruct = FGameplayEffectModifierMagnitude::StaticStruct();
			if (MagnitudeStruct)
			{
				// Try to find and read the MagnitudeCalculationType enum
				const FProperty* CalcTypeProp = MagnitudeStruct->FindPropertyByName(TEXT("MagnitudeCalculationType"));
				if (CalcTypeProp)
				{
					const FEnumProperty* EnumProp = CastField<FEnumProperty>(CalcTypeProp);
					if (EnumProp)
					{
						const void* MagnitudePtr = &ModMagnitude;
						const void* ValuePtr = EnumProp->ContainerPtrToValuePtr<void>(MagnitudePtr);
						int64 EnumValue = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
						
						UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("  MagnitudeCalculationType = %lld"), EnumValue);
						
						// EGameplayEffectMagnitudeCalculation enum values:
						// ScalableFloat = 0
						// AttributeBased = 1
						// CustomCalculationClass = 2
						// SetByCaller = 3
						
						if (EnumValue == 3) // SetByCaller
						{
							UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("  BaseDamage uses SetByCaller."));
							UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("  The actual damage value is set at runtime via SetSetByCallerMagnitude()."));
							UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("  Check the weapon/ability code (e.g., GA_Weapon_Fire) to see the damage value being set."));
							return false;
						}
						else if (EnumValue == 0) // ScalableFloat
						{
							// Try to extract ScalableFloat value
							const FProperty* ScalableFloatProp = MagnitudeStruct->FindPropertyByName(TEXT("ScalableFloatMagnitude"));
							if (ScalableFloatProp)
							{
								FScalableFloat ScalableValue;
								ScalableFloatProp->CopyCompleteValue(&ScalableValue, ScalableFloatProp->ContainerPtrToValuePtr<void>(MagnitudePtr));
								
								OutBaseDamage = ScalableValue.GetValueAtLevel(1.0f);
								UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("  Successfully extracted BaseDamage from ScalableFloat: %f"), OutBaseDamage);
								return true;
							}
						}
						else if (EnumValue == 1) // AttributeBased
						{
							UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("  BaseDamage uses AttributeBased calculation. Cannot extract static value."));
							return false;
						}
						else if (EnumValue == 2) // CustomCalculationClass
						{
							UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("  BaseDamage uses CustomCalculationClass. Cannot extract static value."));
							return false;
						}
					}
				}
			}
			
			UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("  Could not determine magnitude calculation type for BaseDamage"));
			return false;
		}
	}

	// Fallback: Check regular Modifiers (in case some GEs use this approach)
	UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("  BaseDamage not found in CalculationModifiers, checking regular Modifiers..."));
	
	for (const FGameplayModifierInfo& Modifier : GameplayEffect->Modifiers)
	{
		if (Modifier.Attribute == BaseDamageAttribute)
		{
			UE_LOG(LogLyraGEBlueprintLib, Log, TEXT("  Found BaseDamage in regular Modifier"));
			
			// Same issue - cannot safely extract without runtime context
			UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("  BaseDamage found in Modifier but cannot extract safely without runtime context"));
			return false;
		}
	}

	UE_LOG(LogLyraGEBlueprintLib, Warning, TEXT("  BaseDamage attribute not found in CalculationModifiers or Modifiers"));
	return false;
}
