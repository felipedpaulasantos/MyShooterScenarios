// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_AlertAllies.generated.h"

/**
 * Task that finds nearby ally AI and alerts them of the current target by injecting the target into their blackboard.
 * This is used for calling reinforcements.
 */
UCLASS()
class LYRAGAME_API UBTTask_AlertAllies : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AlertAllies();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetToAlertKey;

	UPROPERTY(EditAnywhere, Category = "Alert Settings")
	float AlertRadius;
};

