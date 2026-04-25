// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTTask_TryUseAbility.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UBTTask_TryUseAbility::UBTTask_TryUseAbility()
{
	NodeName = "Try Use Ability By Tag";
}

EBTNodeResult::Type UBTTask_TryUseAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	if (!ASC)
	{
		return EBTNodeResult::Failed;
	}

	if (!AbilityTagToActivate.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	// Try to activate the ability using the tag
	FGameplayTagContainer TagsContainer;
	TagsContainer.AddTag(AbilityTagToActivate);
	bool bActivated = ASC->TryActivateAbilitiesByTag(TagsContainer, true);

	if (bActivated)
	{
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}

