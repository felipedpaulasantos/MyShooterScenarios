// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_FlankTarget.generated.h"

/**
 * Custom EQS test to evaluate how good a location is for flanking a target.
 * It scores points based on the angle between the querier's approach direction 
 * and the target's current facing direction.
 */
UCLASS()
class LYRAGAME_API UEnvQueryTest_FlankTarget : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UEnvQueryTest_FlankTarget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditDefaultsOnly, Category=FlankTarget)
	TSubclassOf<class UEnvQueryContext> TargetContext;

protected:
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};

