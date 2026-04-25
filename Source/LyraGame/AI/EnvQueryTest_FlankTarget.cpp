// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/EnvQueryTest_FlankTarget.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"

UEnvQueryTest_FlankTarget::UEnvQueryTest_FlankTarget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Cost = EEnvTestCost::Low;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	TargetContext = UEnvQueryContext_Querier::StaticClass();
}

void UEnvQueryTest_FlankTarget::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!QueryOwner)
	{
		return;
	}

	FloatValueMin.BindData(QueryOwner, QueryInstance.QueryID);
	float MinThresholdValue = FloatValueMin.GetValue();

	FloatValueMax.BindData(QueryOwner, QueryInstance.QueryID);
	float MaxThresholdValue = FloatValueMax.GetValue();

	TArray<FVector> ContextLocations;
	if (!QueryInstance.PrepareContext(TargetContext, ContextLocations))
	{
		return;
	}
	
	// We only take the first context location/rotation for simplicity
	TArray<AActor*> ContextActors;
	QueryInstance.PrepareContext(TargetContext, ContextActors);

	FVector TargetForward = FVector::ForwardVector;
	FVector TargetLoc = FVector::ZeroVector;

	if (ContextActors.Num() > 0 && ContextActors[0])
	{
		TargetForward = ContextActors[0]->GetActorForwardVector();
		TargetLoc = ContextActors[0]->GetActorLocation();
	}
	else if (ContextLocations.Num() > 0)
	{
		TargetLoc = ContextLocations[0];
	}
	else
	{
		return;
	}

	TargetForward.Z = 0.0f;
	TargetForward.Normalize();

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());
		
		FVector TargetToItem = ItemLocation - TargetLoc;
		TargetToItem.Z = 0.0f;
		TargetToItem.Normalize();

		// Dot product gives us the angle.
		// Front = 1.0, Side = 0.0, Back = -1.0
		// We want to encourage flanks (Side/Back), so -1.0 is best score.
		// Value = 1.0 (Front) -> Score = 0
		// Value = -1.0 (Back) -> Score = 1
		
		float DotProduct = FVector::DotProduct(TargetForward, TargetToItem);
		
		// Map [-1.0, 1.0] to [1.0, 0.0]
		float NormalizedScore = (1.0f - DotProduct) / 2.0f;

		It.SetScore(TestPurpose, FilterType, NormalizedScore, MinThresholdValue, MaxThresholdValue);
	}
}

FText UEnvQueryTest_FlankTarget::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Flank Target"));
}

FText UEnvQueryTest_FlankTarget::GetDescriptionDetails() const
{
	return FText::FromString(TEXT("Scores points based on angle relative to target forward vector."));
}

