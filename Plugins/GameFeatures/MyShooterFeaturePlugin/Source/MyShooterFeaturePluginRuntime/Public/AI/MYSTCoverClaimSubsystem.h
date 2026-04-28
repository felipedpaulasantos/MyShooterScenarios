// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"

#include "MYSTCoverClaimSubsystem.generated.h"

/**
 * Tracks which cover spots are currently "claimed" by AI agents, preventing
 * multiple AIs from pathing to the same location simultaneously.
 *
 * Typical BT flow:
 *   1. Run EQS_FindCover  →  UMYSTEnvQueryTest_ClaimedSpot filters occupied points.
 *   2. BTT_ClaimCoverSpot →  ClaimSpot(ChosenLocation, Self).
 *   3. BTT_MoveTo cover   →  AI moves.
 *   4. On abort / death   →  ReleaseSpot(Self).
 *
 * The subsystem auto-removes stale entries for destroyed actors each tick.
 */
UCLASS()
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMYSTCoverClaimSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// UWorldSubsystem interface
	// -----------------------------------------------------------------------

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// -----------------------------------------------------------------------
	// Cover claim API
	// -----------------------------------------------------------------------

	/**
	 * Attempt to claim a cover spot at Location for Claimer.
	 *
	 * Returns true  – spot was free (no other live agent claimed within Radius) and
	 *                  is now registered.  Any previous claim by Claimer is replaced.
	 * Returns false – spot is already claimed by a different live agent; Claimer's
	 *                  previous claim (if any) is left unchanged.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Cover")
	bool ClaimSpot(FVector Location, AActor* Claimer, float Radius = 150.f);

	/**
	 * Release whatever cover spot Claimer currently holds.
	 * Safe to call even if Claimer has no active claim.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Cover")
	void ReleaseSpot(AActor* Claimer);

	/**
	 * Returns true if a live agent (other than Ignore) has claimed a spot
	 * within Radius of Location.
	 *
	 * Pass the querying actor as Ignore so an agent can re-query its own spot
	 * without self-blocking.
	 */
	UFUNCTION(BlueprintPure, Category = "AI|Cover")
	bool IsSpotClaimed(FVector Location, float Radius = 150.f, const AActor* Ignore = nullptr) const;

	/**
	 * Returns the actor that claimed the spot nearest to Location within Radius,
	 * or nullptr if the area is free.
	 */
	UFUNCTION(BlueprintPure, Category = "AI|Cover")
	AActor* GetClaimant(FVector Location, float Radius = 150.f) const;

	/** Returns how many spots are currently claimed. */
	UFUNCTION(BlueprintPure, Category = "AI|Cover")
	int32 GetClaimCount() const { return ClaimedSpots.Num(); }

	// -----------------------------------------------------------------------
	// Debug
	// -----------------------------------------------------------------------

	/**
	 * Draws all active claims as spheres for Duration seconds
	 * (0 = one frame, useful in Tick).
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Cover|Debug", meta = (DevelopmentOnly))
	void DrawDebugClaims(float Duration = 3.f, float SphereRadius = 40.f) const;

private:

	/** Remove map entries whose owner actor has been destroyed. */
	void PurgeStaleEntries();

	/** Location keyed by the actor that claimed it. */
	TMap<TWeakObjectPtr<AActor>, FVector> ClaimedSpots;
};

