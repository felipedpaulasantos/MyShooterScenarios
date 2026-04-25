// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LyraCoverSubsystem.generated.h"

class UNavigationSystemV1;
class ARecastNavMesh;

/** Represents a single cover point location generated from the NavMesh */
USTRUCT(BlueprintType)
struct FLyraCoverPoint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cover")
	FVector Location;

	/** Direction pointing AWAY from the wall (the normal) */
	UPROPERTY(BlueprintReadOnly, Category = "Cover")
	FVector WallNormal;

	FLyraCoverPoint() : Location(FVector::ZeroVector), WallNormal(FVector::ZeroVector) {}
	FLyraCoverPoint(FVector InLoc, FVector InNormal) : Location(InLoc), WallNormal(InNormal) {}
};

/**
 * World subsystem to asynchronously generate, cache, and provide cover points
 * based on NavMesh boundaries (walls/obstacles). 
 */
UCLASS()
class LYRAGAME_API ULyraCoverSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	ULyraCoverSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Call to start generating cover points in a specific area */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Cover")
	void StartCachingArea(FBox Bounds);

	/** Clear cache for a specific area (usually when leaving) */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Cover")
	void ClearCachedArea(FBox Bounds);

	/** Called by Destructibles upon death to invalidate any cover nearby */
	UFUNCTION(BlueprintCallable, Category = "Lyra|Cover")
	void OnDestructibleDestroyed(AActor* DestroyedActor);

	/** Retrieve cover points safely within a radius */
	void GetCoverPointsInRadius(const FVector& Center, float Radius, TArray<FLyraCoverPoint>& OutPoints) const;

protected:
	void GenerateCoverPointsInBox(const FBox& Bounds);

	UPROPERTY()
	TMap<FIntPoint, FBox> ActiveCachingAreas;

private:
	// Chunk size for the grid
	const float ClusterSize = 1000.0f;

	FIntPoint GetChunkFromLocation(const FVector& Location) const;

	// In-memory cache of generated points per chunk
	TMap<FIntPoint, TArray<FLyraCoverPoint>> CachedCoverPoints;
};

