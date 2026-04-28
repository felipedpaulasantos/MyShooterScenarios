// Copyright MyShooterScenarios. All Rights Reserved.

#include "AI/MYSTCoverClaimSubsystem.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"

// ─────────────────────────────────────────────────────────────────────────────
// UWorldSubsystem interface
// ─────────────────────────────────────────────────────────────────────────────

void UMYSTCoverClaimSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	ClaimedSpots.Empty();
}

// ─────────────────────────────────────────────────────────────────────────────
// Cover claim API
// ─────────────────────────────────────────────────────────────────────────────

bool UMYSTCoverClaimSubsystem::ClaimSpot(FVector Location, AActor* Claimer, float Radius)
{
	if (!IsValid(Claimer))
	{
		return false;
	}

	PurgeStaleEntries();

	const float RadiusSq = Radius * Radius;

	for (const TPair<TWeakObjectPtr<AActor>, FVector>& Pair : ClaimedSpots)
	{
		const AActor* OtherClaimer = Pair.Key.Get();

		// Skip our own existing claim so we can move to a new spot
		if (OtherClaimer == Claimer)
		{
			continue;
		}

		if (FVector::DistSquared(Pair.Value, Location) < RadiusSq)
		{
			// Another live agent already owns this spot
			return false;
		}
	}

	// Register (or overwrite) this agent's claim
	ClaimedSpots.FindOrAdd(Claimer) = Location;
	return true;
}

void UMYSTCoverClaimSubsystem::ReleaseSpot(AActor* Claimer)
{
	if (!Claimer)
	{
		return;
	}
	ClaimedSpots.Remove(Claimer);
}

bool UMYSTCoverClaimSubsystem::IsSpotClaimed(FVector Location, float Radius, const AActor* Ignore) const
{
	const float RadiusSq = Radius * Radius;

	for (const TPair<TWeakObjectPtr<AActor>, FVector>& Pair : ClaimedSpots)
	{
		const AActor* Owner = Pair.Key.Get();
		if (!Owner || Owner == Ignore)
		{
			continue;
		}

		if (FVector::DistSquared(Pair.Value, Location) < RadiusSq)
		{
			return true;
		}
	}
	return false;
}

AActor* UMYSTCoverClaimSubsystem::GetClaimant(FVector Location, float Radius) const
{
	const float RadiusSq = Radius * Radius;
	float BestDistSq = FLT_MAX;
	AActor* BestActor = nullptr;

	for (const TPair<TWeakObjectPtr<AActor>, FVector>& Pair : ClaimedSpots)
	{
		AActor* Owner = Pair.Key.Get();
		if (!Owner)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Pair.Value, Location);
		if (DistSq < RadiusSq && DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestActor = Owner;
		}
	}
	return BestActor;
}

// ─────────────────────────────────────────────────────────────────────────────
// Debug
// ─────────────────────────────────────────────────────────────────────────────

void UMYSTCoverClaimSubsystem::DrawDebugClaims(float Duration, float SphereRadius) const
{
#if ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const TPair<TWeakObjectPtr<AActor>, FVector>& Pair : ClaimedSpots)
	{
		if (const AActor* Owner = Pair.Key.Get())
		{
			DrawDebugSphere(World, Pair.Value, SphereRadius, 12, FColor::Orange, false, Duration);
			DrawDebugLine(World, Owner->GetActorLocation(), Pair.Value, FColor::Yellow, false, Duration, 0, 2.f);
			DrawDebugString(World, Pair.Value + FVector(0.f, 0.f, 60.f), Owner->GetName(), nullptr, FColor::White, Duration);
		}
	}
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void UMYSTCoverClaimSubsystem::PurgeStaleEntries()
{
	for (auto It = ClaimedSpots.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

