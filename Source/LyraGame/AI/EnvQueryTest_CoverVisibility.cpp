// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/EnvQueryTest_CoverVisibility.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UEnvQueryTest_CoverVisibility::UEnvQueryTest_CoverVisibility(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Cost = EEnvTestCost::High;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	Context = UEnvQueryContext_Querier::StaticClass();

	// Usually configured in the Editor, but this defaults to blocking our specific Cover Channel
	TraceData.SetGeometryOnly();
	TraceData.TraceChannel = UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel6); // Lyra_ObjectChannel_Geometry or 7 for Lyra_TraceChannel_Cover
	FilterType = EEnvTestFilterType::Match;
	BoolValue.DefaultValue = false; // We WANT a block! If there is no block (hit), it's not cover. So we want Hit = true, meaning Visibility = false.
}

void UEnvQueryTest_CoverVisibility::RunTest(FEnvQueryInstance& QueryInstance) const
{
	// This uses native asynchronous tracing mechanisms when configured to do so.
	// Since we derive from UEnvQueryTest_Trace, doing a simple override of RunTest or relying on Super::RunTest is possible.
	// To enforce "Must hit geometry to be safe", we just utilize the parent logic which natively handles the TraceData 
	// and contexts (like Target) async batches.
	
	// We want to return TRUE if line of sight is BLOCKED. 
	// The parent Trace test returns TRUE if the trace was a HIT (blocked).
	// So we just configure BoolValue = true (Must Hit).
	BoolValue.BindData(QueryInstance.Owner.Get(), QueryInstance.QueryID);
	bool bWantsHit = BoolValue.GetValue();

	Super::RunTest(QueryInstance);
}

FText UEnvQueryTest_CoverVisibility::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Cover Visibility Test (Blocks LOS)"));
}

