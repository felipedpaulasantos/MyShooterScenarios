// Copyright MyShooterScenarios. All Rights Reserved.

#include "AI/MYSTEnvQueryContext_CoverLocation.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"

void UMYSTEnvQueryContext_CoverLocation::ProvideContext(
	FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	UObject* Owner = QueryInstance.Owner.Get();

	// Owner is the querier — either the AIController itself or the controlled pawn.
	AAIController* AIC = Cast<AAIController>(Owner);
	if (!AIC)
	{
		if (APawn* Pawn = Cast<APawn>(Owner))
		{
			AIC = Cast<AAIController>(Pawn->GetController());
		}
	}

	if (!AIC)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UMYSTEnvQueryContext_CoverLocation: could not resolve AIController from query owner."));
		return;
	}

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	const FVector CoverLoc = BB->GetValueAsVector(CoverLocationKeyName);
	UEnvQueryItemType_Point::SetContextHelper(ContextData, CoverLoc);
}

