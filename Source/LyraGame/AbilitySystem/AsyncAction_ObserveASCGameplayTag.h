// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/CancellableAsyncAction.h"
#include "GameplayTagContainer.h"

#include "AsyncAction_ObserveASCGameplayTag.generated.h"

class UAbilitySystemComponent;
class ULyraAbilitySystemComponent;

/** Fires when the observed tag is added/removed on the specified ASC. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FASCGameplayTagAsyncDelegate, FGameplayTag, Tag);

/**
 * Observes a single gameplay tag on a specific AbilitySystemComponent.
 *
 * This listens to GAS tag counts (GameplayEffects / Abilities), not Actor owned tags.
 */
UCLASS()
class LYRAGAME_API UAsyncAction_ObserveASCGameplayTag : public UCancellableAsyncAction
{
	GENERATED_UCLASS_BODY()

public:
	/** Called when the tag count transitions from 0 -> 1. */
	UPROPERTY(BlueprintAssignable)
	FASCGameplayTagAsyncDelegate OnTagAdded;

	/** Called when the tag count transitions from 1 -> 0. */
	UPROPERTY(BlueprintAssignable)
	FASCGameplayTagAsyncDelegate OnTagRemoved;

	/**
	 * Start observing a single gameplay tag on an ASC.
	 * @param AbilitySystemComponent The ASC to observe.
	 * @param Tag The tag to observe.
	 */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", Keywords="Watch Observe Listen", DisplayName="Observe ASC Gameplay Tag"), Category="AbilitySystem|Tags")
	static UAsyncAction_ObserveASCGameplayTag* ObserveASCGameplayTag(UAbilitySystemComponent* AbilitySystemComponent, FGameplayTag Tag);

	/**
	 * Convenience overload for Blueprints: pass in *anything* (Pawn/Character/PlayerState/Controller/PawnExtComponent/ASC).
	 * We'll resolve the actual ASC if possible.
	 */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", Keywords="Watch Observe Listen", DisplayName="Observe Gameplay Tag (Resolve ASC)"), Category="AbilitySystem|Tags")
	static UAsyncAction_ObserveASCGameplayTag* ObserveGameplayTag_ResolveASC(UObject* AbilitySystemSource, FGameplayTag Tag);

	//~UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;
	//~End of UBlueprintAsyncActionBase interface

private:
	static UAbilitySystemComponent* ResolveASCFromSourceObject(UObject* AbilitySystemSource);

	UFUNCTION()
	void HandleGameplayTagChanged(const FGameplayTag Tag, int32 NewCount);

	/**
	 * Fires when any UWorld begins tearing down.  If it is the world that owns
	 * our observed ASC we cancel ourselves immediately so the reference chain to
	 * that world is broken before GC runs (avoids "Previously active world not
	 * cleaned up" errors in PIE).
	 */
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	void Cleanup();

private:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASC;

	FGameplayTag ObservedTag;
	FDelegateHandle DelegateHandle;

	/** Handle for our FWorldDelegates::OnWorldCleanup subscription. */
	FDelegateHandle WorldCleanupHandle;

	int32 LastKnownCount = 0;
};

