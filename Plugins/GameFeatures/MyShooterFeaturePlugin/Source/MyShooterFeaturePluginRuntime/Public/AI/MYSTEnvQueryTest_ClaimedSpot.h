// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "EnvironmentQuery/EnvQueryTest.h"

#include "MYSTEnvQueryTest_ClaimedSpot.generated.h"

/**
 * EQS Test: filters out candidate locations that are already claimed by another AI
 * via UMYSTCoverClaimSubsystem.
 *
 * Add this test to EQS_FindCover (Test Purpose: Filter).
 * Claimed spots score 0 and are discarded; free spots score 1 and pass.
 *
 * ClaimRadius should match the radius you pass to ClaimSpot / IsSpotClaimed at runtime.
 */
UCLASS(meta = (DisplayName = "Cover: Spot Claimed (MYST)"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMYSTEnvQueryTest_ClaimedSpot : public UEnvQueryTest
{
	GENERATED_BODY()

public:

	UMYSTEnvQueryTest_ClaimedSpot(const FObjectInitializer& ObjectInitializer);

	// UEnvQueryTest interface
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

	/**
	 * Radius (cm) within which a candidate point is considered "the same spot"
	 * as an existing claim.  Should match the radius used in BTT_ClaimCoverSpot.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Cover Claim", meta = (ClampMin = "10.0", ForceUnits = "cm"))
	float ClaimRadius = 150.f;
};

