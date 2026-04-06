// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/AsyncAction_ObserveASCGameplayTag.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "Logging/LogMacros.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Character/LyraPawnExtensionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_ObserveASCGameplayTag)

DEFINE_LOG_CATEGORY_STATIC(LogObserveASCGameplayTag, Log, All);

UAsyncAction_ObserveASCGameplayTag::UAsyncAction_ObserveASCGameplayTag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAsyncAction_ObserveASCGameplayTag* UAsyncAction_ObserveASCGameplayTag::ObserveASCGameplayTag(UAbilitySystemComponent* AbilitySystemComponent, FGameplayTag Tag)
{
	UAsyncAction_ObserveASCGameplayTag* Action = nullptr;

	if (IsValid(AbilitySystemComponent) && Tag.IsValid())
	{
		Action = NewObject<UAsyncAction_ObserveASCGameplayTag>();
		Action->ASC = AbilitySystemComponent;
		Action->ObservedTag = Tag;

		// Keep this action alive as long as the game instance is alive.
		Action->RegisterWithGameInstance(AbilitySystemComponent);
	}

	return Action;
}

UAsyncAction_ObserveASCGameplayTag* UAsyncAction_ObserveASCGameplayTag::ObserveGameplayTag_ResolveASC(UObject* AbilitySystemSource, FGameplayTag Tag)
{
	UAbilitySystemComponent* ResolvedASC = ResolveASCFromSourceObject(AbilitySystemSource);

	return ObserveASCGameplayTag(ResolvedASC, Tag);
}

UAbilitySystemComponent* UAsyncAction_ObserveASCGameplayTag::ResolveASCFromSourceObject(UObject* AbilitySystemSource)
{
	if (!IsValid(AbilitySystemSource))
	{
		return nullptr;
	}

	// 1) Direct ASC
	if (UAbilitySystemComponent* ASC = Cast<UAbilitySystemComponent>(AbilitySystemSource))
	{
		return ASC;
	}

	// 2) Anything implementing IAbilitySystemInterface (Lyra Character, Lyra PlayerState, etc.)
	if (AbilitySystemSource->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(AbilitySystemSource))
		{
			return ASI->GetAbilitySystemComponent();
		}
	}

	// 3) Lyra pawn extension component
	if (ULyraPawnExtensionComponent* PawnExt = Cast<ULyraPawnExtensionComponent>(AbilitySystemSource))
	{
		return PawnExt->GetLyraAbilitySystemComponent();
	}

	// 4) Actor: try pawn extension first, then ability system interface.
	if (AActor* Actor = Cast<AActor>(AbilitySystemSource))
	{
		if (ULyraPawnExtensionComponent* PawnExt = ULyraPawnExtensionComponent::FindPawnExtensionComponent(Actor))
		{
			if (UAbilitySystemComponent* ASC = PawnExt->GetLyraAbilitySystemComponent())
			{
				return ASC;
			}
		}

		if (Actor->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
		{
			if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor))
			{
				return ASI->GetAbilitySystemComponent();
			}
		}
	}

	// 5) Controller: in Lyra, ASC is typically on PlayerState.
	if (AController* Controller = Cast<AController>(AbilitySystemSource))
	{
		if (APlayerState* PS = Controller->PlayerState)
		{
			if (PS->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
			{
				if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
				{
					return ASI->GetAbilitySystemComponent();
				}
			}
		}
	}

	return nullptr;
}

void UAsyncAction_ObserveASCGameplayTag::Activate()
{
	if (!IsValid(ASC) || !ObservedTag.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	// Cache initial count, but do not broadcast; users typically want "future" add/remove.
	LastKnownCount = ASC->GetGameplayTagCount(ObservedTag);

	DelegateHandle =
		ASC->RegisterGameplayTagEvent(ObservedTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleGameplayTagChanged);
}

void UAsyncAction_ObserveASCGameplayTag::SetReadyToDestroy()
{
	Cleanup();
	Super::SetReadyToDestroy();
}

void UAsyncAction_ObserveASCGameplayTag::Cleanup()
{
	if (IsValid(ASC) && DelegateHandle.IsValid())
	{
		ASC->RegisterGameplayTagEvent(ObservedTag, EGameplayTagEventType::NewOrRemoved).Remove(DelegateHandle);
		DelegateHandle.Reset();
	}
}

void UAsyncAction_ObserveASCGameplayTag::HandleGameplayTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// We only care about edge transitions.
	const bool bWasPresent = (LastKnownCount > 0);
	const bool bIsPresent = (NewCount > 0);

	LastKnownCount = NewCount;

	if (!bWasPresent && bIsPresent)
	{
		OnTagAdded.Broadcast(Tag);
	}
	else if (bWasPresent && !bIsPresent)
	{
		OnTagRemoved.Broadcast(Tag);
	}
}

