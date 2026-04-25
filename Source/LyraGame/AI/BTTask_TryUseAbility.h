// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTTask_TryUseAbility.generated.h"

/**
 * Task that tells the AI to use a Gameplay Ability by Tag (e.g. Throw Grenade, Rush, Call Reinforcements).
 */
UCLASS()
class LYRAGAME_API UBTTask_TryUseAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_TryUseAbility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Abilities")
	FGameplayTag AbilityTagToActivate;
};

