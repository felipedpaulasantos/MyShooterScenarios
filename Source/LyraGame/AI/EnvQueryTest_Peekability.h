// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "DataProviders/AIDataProvider.h"
#include "EnvQueryTest_Peekability.generated.h"

/**
 * Custom EQS Test to verify if an AI can lean/peek left or right from a cover point
 * to gain Line of Sight to the Target.
 */
UCLASS()
class LYRAGAME_API UEnvQueryTest_Peekability : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UEnvQueryTest_Peekability(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditDefaultsOnly, Category = Peek)
	FAIDataProviderFloatValue PeekOffset;

	UPROPERTY(EditDefaultsOnly, Category = Peek)
	TSubclassOf<class UEnvQueryContext> TargetContext;

protected:
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};

