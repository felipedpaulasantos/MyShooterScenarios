// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/LyraCoverSubsystem.h"
#include "Engine/World.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "NavigationSystem.h"

ULyraCoverSubsystem::ULyraCoverSubsystem()
{
}

void ULyraCoverSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void ULyraCoverSubsystem::Deinitialize()
{
	Super::Deinitialize();
	CachedCoverPoints.Empty();
}

FIntPoint ULyraCoverSubsystem::GetChunkFromLocation(const FVector& Location) const
{
	return FIntPoint(
		FMath::FloorToInt(Location.X / ClusterSize),
		FMath::FloorToInt(Location.Y / ClusterSize)
	);
}

void ULyraCoverSubsystem::StartCachingArea(FBox Bounds)
{
	// Calculate Which chunks overlap the box.
	FIntPoint MinChunk = GetChunkFromLocation(Bounds.Min);
	FIntPoint MaxChunk = GetChunkFromLocation(Bounds.Max);

	for (int32 X = MinChunk.X; X <= MaxChunk.X; ++X)
	{
		for (int32 Y = MinChunk.Y; Y <= MaxChunk.Y; ++Y)
		{
			FIntPoint Chunk(X, Y);
			if (!CachedCoverPoints.Contains(Chunk))
			{
				FBox ChunkBox(
					FVector(X * ClusterSize, Y * ClusterSize, Bounds.Min.Z),
					FVector((X + 1) * ClusterSize, (Y + 1) * ClusterSize, Bounds.Max.Z)
				);
				GenerateCoverPointsInBox(ChunkBox);
			}
		}
	}
}

void ULyraCoverSubsystem::ClearCachedArea(FBox Bounds)
{
	FIntPoint MinChunk = GetChunkFromLocation(Bounds.Min);
	FIntPoint MaxChunk = GetChunkFromLocation(Bounds.Max);

	for (int32 X = MinChunk.X; X <= MaxChunk.X; ++X)
	{
		for (int32 Y = MinChunk.Y; Y <= MaxChunk.Y; ++Y)
		{
			CachedCoverPoints.Remove(FIntPoint(X, Y));
		}
	}
}

void ULyraCoverSubsystem::GenerateCoverPointsInBox(const FBox& Bounds)
{
	// This function extracts raw Navigation polygons bounds inside the area.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		return;
	}

	ARecastNavMesh* NavMesh = Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate));
	if (!NavMesh)
	{
		return;
	}

	FIntPoint ChunkCenter = GetChunkFromLocation(Bounds.GetCenter());
	TArray<FLyraCoverPoint>& OutPoints = CachedCoverPoints.FindOrAdd(ChunkCenter);

	// Get poly edges. A simple approximate implementation of wall boundaries.
	TArray<FNavPoly> Polys;
	// Recast doesn't have an easily exposed Box Query for polygons to the BP/Simple API in UE5 without detouring deeply into Detour internals.
	// We'll perform a simplified simulation by casting lines in a grid and asking NavSys to project points to walls.
	
	const float ProbeGridSize = 250.0f; // Probe every 2.5 meters.
	
	for (float X = Bounds.Min.X; X <= Bounds.Max.X; X += ProbeGridSize)
	{
		for (float Y = Bounds.Min.Y; Y <= Bounds.Max.Y; Y += ProbeGridSize)
		{
			FVector TestPoint(X, Y, Bounds.GetCenter().Z);
			FNavLocation ProjectedLoc;

			// Projecting the test point down to nav mesh
			if (NavSys->ProjectPointToNavigation(TestPoint, ProjectedLoc))
			{
				// Raycast from that point in cardinal directions looking for nav boundaries
				TArray<FVector> Directions = { FVector::ForwardVector, FVector::BackwardVector, FVector::RightVector, FVector::LeftVector };
				
				for (const FVector& Dir : Directions)
				{
					FVector EndTest = ProjectedLoc.Location + (Dir * ProbeGridSize);
					FVector OutHit;
					if (NavMesh->Raycast(ProjectedLoc.Location, EndTest, OutHit, UNavigationQueryFilter::GetQueryFilter(*NavMesh, nullptr, nullptr)))
					{
						// It hit a wall boundary on the navmesh!
						// Calculate normal pointing AWAY from the wall (which would be -Dir or the hit boundary normal)
						FLyraCoverPoint FoundCover(OutHit, -Dir);
						OutPoints.Add(FoundCover);
					}
				}
			}
		}
	}
}

void ULyraCoverSubsystem::OnDestructibleDestroyed(AActor* DestroyedActor)
{
	if (!DestroyedActor)
		return;

	FIntPoint Chunk = GetChunkFromLocation(DestroyedActor->GetActorLocation());
	
	if (CachedCoverPoints.Contains(Chunk))
	{
		CachedCoverPoints.Remove(Chunk);
		
		// Regenerate immediately or wait for the next call
		FBox RegenBox(
			FVector(Chunk.X * ClusterSize, Chunk.Y * ClusterSize, DestroyedActor->GetActorLocation().Z - 200.f),
			FVector((Chunk.X + 1) * ClusterSize, (Chunk.Y + 1) * ClusterSize, DestroyedActor->GetActorLocation().Z + 200.f)
		);
		GenerateCoverPointsInBox(RegenBox);
	}
}

void ULyraCoverSubsystem::GetCoverPointsInRadius(const FVector& Center, float Radius, TArray<FLyraCoverPoint>& OutPoints) const
{
	// Calculate the bounding box for the radius
	FBox RadiusBox(Center - FVector(Radius, Radius, Radius), Center + FVector(Radius, Radius, Radius));
	
	FIntPoint MinChunk = GetChunkFromLocation(RadiusBox.Min);
	FIntPoint MaxChunk = GetChunkFromLocation(RadiusBox.Max);

	const float RadiusSq = Radius * Radius;

	for (int32 X = MinChunk.X; X <= MaxChunk.X; ++X)
	{
		for (int32 Y = MinChunk.Y; Y <= MaxChunk.Y; ++Y)
		{
			FIntPoint Chunk(X, Y);
			if (const TArray<FLyraCoverPoint>* ChunkPoints = CachedCoverPoints.Find(Chunk))
			{
				for (const FLyraCoverPoint& Pt : *ChunkPoints)
				{
					if (FVector::DistSquaredXY(Center, Pt.Location) <= RadiusSq)
					{
						OutPoints.Add(Pt);
					}
				}
			}
		}
	}
}

