// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameFeatures/GameFeatureAction_AddInputConfig.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/GameInstance.h"
#include "EnhancedInputSubsystems.h"
#include "Character/LyraHeroComponent.h"	// for NAME_BindInputsNow
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "GameFeatures/GameFeatureAction_WorldActionBase.h"
#include "PlayerMappableInputConfig.h"
#include "GameFramework/Pawn.h"
#include "Input/LyraMappableConfigPair.h"
#include "Misc/DataValidation.h"
#include "InputMappingContext.h"
#include "EnhancedActionKeyMapping.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameFeatureAction_AddInputConfig)

#define LOCTEXT_NAMESPACE "GameFeatures_AddInputConfig"

void UGameFeatureAction_AddInputConfig::OnGameFeatureRegistering()
{
	Super::OnGameFeatureRegistering();

	// Register the input configs with the local settings, this way the data inside them is available all the time
	// and not just when this game feature is active. This is necessary for displaying key binding options
	// on the main menu, or other times when the game feature may not be active.
	for (const FMappableConfigPair& Pair : InputConfigs)
	{
		FMappableConfigPair::RegisterPair(Pair);
	}
}

void UGameFeatureAction_AddInputConfig::OnGameFeatureUnregistering()
{
	Super::OnGameFeatureUnregistering();

	for (const FMappableConfigPair& Pair : InputConfigs)
	{
		FMappableConfigPair::UnregisterPair(Pair);
	}
}

void UGameFeatureAction_AddInputConfig::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	FPerContextData& ActiveData = ContextData.FindOrAdd(Context);
	if (!ensure(ActiveData.ExtensionRequestHandles.IsEmpty()) ||
		!ensure(ActiveData.PawnsAddedTo.IsEmpty()))
	{
		Reset(ActiveData);
	}

	// Call super after the above logic so that we have our context before being added to the world
	Super::OnGameFeatureActivating(Context);
}

void UGameFeatureAction_AddInputConfig::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	FPerContextData* ActiveData = ContextData.Find(Context);

	if (ensure(ActiveData))
	{
		Reset(*ActiveData);
	}
}

#if WITH_EDITOR
EDataValidationResult UGameFeatureAction_AddInputConfig::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	int32 EntryIndex = 0;
	for (const FMappableConfigPair& Pair : InputConfigs)
	{
		if (Pair.Config.IsNull())
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::Format(LOCTEXT("NullConfigPointer", "Null Config pointer at index {0} in Pair list"), FText::AsNumber(EntryIndex)));
		}

		++EntryIndex;
	}
	
	return Result;
}
#endif	// WITH_EDITOR

void UGameFeatureAction_AddInputConfig::AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext)
{
	UWorld* World = WorldContext.World();
	UGameInstance* GameInstance = WorldContext.OwningGameInstance;
	FPerContextData& ActiveData = ContextData.FindOrAdd(ChangeContext);
	
	if (GameInstance && World && World->IsGameWorld())
	{
		if (UGameFrameworkComponentManager* ComponentMan = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance))
		{
			UGameFrameworkComponentManager::FExtensionHandlerDelegate AddConfigDelegate =
				UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(this, &ThisClass::HandlePawnExtension, ChangeContext);
			
			TSharedPtr<FComponentRequestHandle> ExtensionRequestHandle = ComponentMan->AddExtensionHandler(APawn::StaticClass(), AddConfigDelegate);
			ActiveData.ExtensionRequestHandles.Add(ExtensionRequestHandle);
		}
	}
}

void UGameFeatureAction_AddInputConfig::Reset(FPerContextData& ActiveData)
{
	ActiveData.ExtensionRequestHandles.Empty();

	while (!ActiveData.PawnsAddedTo.IsEmpty())
	{
		TWeakObjectPtr<APawn> PawnPtr = ActiveData.PawnsAddedTo.Top();
		if (PawnPtr.IsValid())
		{
			RemoveInputConfig(PawnPtr.Get(), ActiveData);
		}
		else
		{
			ActiveData.PawnsAddedTo.Pop();
		}
	}
}

void UGameFeatureAction_AddInputConfig::HandlePawnExtension(AActor* Actor, FName EventName, FGameFeatureStateChangeContext ChangeContext)
{
	APawn* AsPawn = CastChecked<APawn>(Actor);
	FPerContextData& ActiveData = ContextData.FindOrAdd(ChangeContext);

	if (EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded || EventName == ULyraHeroComponent::NAME_BindInputsNow)
	{
		AddInputConfig(AsPawn, ActiveData);
	}
	else if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
	{
		RemoveInputConfig(AsPawn, ActiveData);
	}
}

void UGameFeatureAction_AddInputConfig::AddInputConfig(APawn* Pawn, FPerContextData& ActiveData)
{
	APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());

	if (ULocalPlayer* LP = PlayerController ? PlayerController->GetLocalPlayer() : nullptr)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			FModifyContextOptions Options = {};
			Options.bIgnoreAllPressedKeysUntilRelease = false;
			
			// Add the input mapping contexts from each config
			for (const FMappableConfigPair& Pair : InputConfigs)
			{
				if (Pair.bShouldActivateAutomatically && Pair.CanBeActivated())
				{
					if (UPlayerMappableInputConfig* LoadedConfig = Pair.Config.LoadSynchronous())
					{
						// UE 5.5: GetMappingContexts() returns a TMap<UInputMappingContext*, int32>
						// where the key is the context and the value is the priority
						const TMap<TObjectPtr<UInputMappingContext>, int32>& Contexts = LoadedConfig->GetMappingContexts();
						
						for (const TPair<TObjectPtr<UInputMappingContext>, int32>& ContextPair : Contexts)
						{
							if (ContextPair.Key)
							{
								Subsystem->AddMappingContext(ContextPair.Key, ContextPair.Value, Options);
								UE_LOG(LogGameFeatures, Log, TEXT("Added Input Mapping Context: %s (Priority: %d)"), 
									*ContextPair.Key->GetName(), ContextPair.Value);
							}
						}
					}
				}
			}
			
			ActiveData.PawnsAddedTo.AddUnique(Pawn);
		}		
	}
}

void UGameFeatureAction_AddInputConfig::RemoveInputConfig(APawn* Pawn, FPerContextData& ActiveData)
{
	APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());

	if (ULocalPlayer* LP = PlayerController ? PlayerController->GetLocalPlayer() : nullptr)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			// Remove the input mapping contexts that were added
			for (const FMappableConfigPair& Pair : InputConfigs)
			{
				if (Pair.bShouldActivateAutomatically && Pair.CanBeActivated())
				{
					if (UPlayerMappableInputConfig* LoadedConfig = Pair.Config.LoadSynchronous())
					{
						// UE 5.5: GetMappingContexts() returns a TMap<UInputMappingContext*, int32>
						const TMap<TObjectPtr<UInputMappingContext>, int32>& Contexts = LoadedConfig->GetMappingContexts();
						
						for (const TPair<TObjectPtr<UInputMappingContext>, int32>& ContextPair : Contexts)
						{
							if (ContextPair.Key)
							{
								Subsystem->RemoveMappingContext(ContextPair.Key);
							}
						}
					}
				}
			}
		}
	}
	ActiveData.PawnsAddedTo.Remove(Pawn);
}

#undef LOCTEXT_NAMESPACE
