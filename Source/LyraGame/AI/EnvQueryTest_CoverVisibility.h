// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Trace.h"
#include "EnvQueryTest_CoverVisibility.generated.h"

UCLASS()
class LYRAGAME_API UEnvQueryTest_CoverVisibility : public UEnvQueryTest_Trace
{
	GENERATED_BODY()

public:
	UEnvQueryTest_CoverVisibility(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
};

