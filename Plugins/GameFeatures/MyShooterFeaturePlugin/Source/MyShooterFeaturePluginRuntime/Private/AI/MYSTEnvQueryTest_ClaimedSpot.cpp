// Copyright MyShooterScenarios. All Rights Reserved.

#include "AI/MYSTEnvQueryTest_ClaimedSpot.h"

#include "AI/MYSTCoverClaimSubsystem.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "MYSTEnvQueryTest_ClaimedSpot"

UMYSTEnvQueryTest_ClaimedSpot::UMYSTEnvQueryTest_ClaimedSpot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Points / locations only (not actor items)
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();

	// Low cost: just a map lookup, no physics
	Cost = EEnvTestCost::Low;

	// Default: act as a pure filter (drop claimed spots, keep free ones)
	TestPurpose = EEnvTestPurpose::Filter;
	SetWorkOnFloatValues(false);
}

void UMYSTEnvQueryTest_ClaimedSpot::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!QueryOwner)
	{
		return;
	}

	// The querying actor — its own claim should not block itself
	const AActor* QuerierActor = Cast<AActor>(QueryOwner);

	UWorld* World = GEngine->GetWorldFromContextObject(QueryOwner, EGetWorldErrorMode::LogAndReturnNull);
	UMYSTCoverClaimSubsystem* ClaimSubsystem = World ? World->GetSubsystem<UMYSTCoverClaimSubsystem>() : nullptr;

	if (!ClaimSubsystem)
	{
		// Subsystem unavailable — let all items through rather than silently blocking them
		UE_LOG(LogTemp, Warning, TEXT("UMYSTEnvQueryTest_ClaimedSpot: UMYSTCoverClaimSubsystem not found — skipping test."));
		return;
	}

	BoolValue.BindData(QueryOwner, QueryInstance.QueryID);
	// BoolValue default = true  →  we want spots that are NOT claimed (i.e. bIsFree == true)
	const bool bWantFree = BoolValue.GetValue();

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector CandidateLoc = GetItemLocation(QueryInstance, It.GetIndex());
		const bool bIsFree = !ClaimSubsystem->IsSpotClaimed(CandidateLoc, ClaimRadius, QuerierActor);

		It.SetScore(TestPurpose, FilterType, bIsFree, bWantFree);
	}
}

FText UMYSTEnvQueryTest_ClaimedSpot::GetDescriptionTitle() const
{
	return LOCTEXT("Title", "Cover Spot Claimed");
}

FText UMYSTEnvQueryTest_ClaimedSpot::GetDescriptionDetails() const
{
	return FText::Format(
		LOCTEXT("Details", "Filters out spots already claimed by another AI (radius: {0} cm)."),
		FText::AsNumber(FMath::RoundToInt(ClaimRadius))
	);
}

#undef LOCTEXT_NAMESPACE

