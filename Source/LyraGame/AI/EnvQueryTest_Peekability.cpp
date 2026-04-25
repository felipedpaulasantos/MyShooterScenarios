// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/EnvQueryTest_Peekability.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UEnvQueryTest_Peekability::UEnvQueryTest_Peekability(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Cost = EEnvTestCost::High;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	TargetContext = UEnvQueryContext_Querier::StaticClass();
	PeekOffset.DefaultValue = 70.0f; // E.g., slightly larger than typical capsule radius (40-50 units)
}

void UEnvQueryTest_Peekability::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!QueryOwner)
	{
		return;
	}

	PeekOffset.BindData(QueryOwner, QueryInstance.QueryID);
	float CurrentPeekOffset = PeekOffset.GetValue();

	FloatValueMin.BindData(QueryOwner, QueryInstance.QueryID);
	float MinThresholdValue = FloatValueMin.GetValue();

	FloatValueMax.BindData(QueryOwner, QueryInstance.QueryID);
	float MaxThresholdValue = FloatValueMax.GetValue();

	TArray<FVector> ContextLocations;
	if (!QueryInstance.PrepareContext(TargetContext, ContextLocations))
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(QueryOwner, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(EnvQueryTrace), true, Cast<AActor>(QueryOwner));

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());

		bool bCanPeekLeft = false;
		bool bCanPeekRight = false;
		
		for (const FVector& TargetLoc : ContextLocations)
		{
			FVector DirToTarget = (TargetLoc - ItemLocation).GetSafeNormal2D();
			FVector RightVector = FVector::CrossProduct(FVector::UpVector, DirToTarget).GetSafeNormal2D();

			// Peek Left
			FVector LeftPeekLoc = ItemLocation - (RightVector * CurrentPeekOffset);
			bool bHitLeft = World->LineTraceTestByChannel(LeftPeekLoc, TargetLoc, ECC_GameTraceChannel6, TraceParams); // Try ECC_GameTraceChannel6 or 7

			// Peek Right
			FVector RightPeekLoc = ItemLocation + (RightVector * CurrentPeekOffset);
			bool bHitRight = World->LineTraceTestByChannel(RightPeekLoc, TargetLoc, ECC_GameTraceChannel6, TraceParams);

			// Not hitting means line of sight is clear from that peek spot!
			if (!bHitLeft) bCanPeekLeft = true;
			if (!bHitRight) bCanPeekRight = true;
		}

		// AI can peek if at least one side has no geometry blocking sight
		bool bCanPeek = bCanPeekLeft || bCanPeekRight;

		float Score = bCanPeek ? 1.0f : 0.0f;
		It.SetScore(TestPurpose, FilterType, Score, MinThresholdValue, MaxThresholdValue);
	}
}

FText UEnvQueryTest_Peekability::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Peekability Test"));
}

FText UEnvQueryTest_Peekability::GetDescriptionDetails() const
{
	return FText::FromString(TEXT("Offsets L/R and tests LOS to target. Needs 1 unblocked trace."));
}
