// Copyright MyShooterScenarios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "MYSTChunkDefinition.generated.h"

class AMYSTRoadChunkActor;

// ─────────────────────────────────────────────────────────────────────────────
// UMYSTChunkDefinition
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Data asset that describes one "road chunk" template used by the
 * AMYSTChaseTrackManager treadmill system.
 *
 * Create one asset per unique road segment type (e.g. DA_Chunk_Straight,
 * DA_Chunk_Obstacles, DA_Chunk_EnemyAmbush, DA_Chunk_End).
 *
 * The manager picks chunks from its pool at runtime according to SpawnWeight;
 * the chunk actor class referenced here is the Blueprint that actually places
 * meshes, enemies, and obstacles.
 *
 * ── USAGE ────────────────────────────────────────────────────────────────────
 * 1. Create a Blueprint subclass of AMYSTRoadChunkActor (e.g. B_Chunk_Ambush).
 * 2. Create a UMYSTChunkDefinition data asset (e.g. DA_Chunk_Ambush).
 * 3. Set ChunkClass → B_Chunk_Ambush, ChunkLength, SpawnWeight, etc.
 * 4. Add the asset to AMYSTChaseTrackManager::ChunkPool.
 */
UCLASS(BlueprintType, Const, meta=(DisplayName="MYST Chunk Definition"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API UMYSTChunkDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// ── Chunk Actor ───────────────────────────────────────────────────────────

	/**
	 * Blueprint subclass of AMYSTRoadChunkActor to spawn for this chunk.
	 * Must not be null — chunks with a null class are skipped at spawn time.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Chunk|Actor")
	TSubclassOf<AMYSTRoadChunkActor> ChunkClass;

	// ── Geometry ──────────────────────────────────────────────────────────────

	/**
	 * Length of this chunk along the track forward axis (cm).
	 * The manager uses this to place the next chunk immediately after this one
	 * and to decide when the player has fully passed through it.
	 * Make sure this matches the physical length of the road mesh in the chunk actor.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Chunk|Geometry",
	          meta=(ClampMin=100.f, UIMin=100.f, ForceUnits="cm"))
	float ChunkLength = 5000.f;

	// ── Spawning / Pool Weight ────────────────────────────────────────────────

	/**
	 * Relative weight used when selecting this chunk from the pool at random.
	 * Higher values make this chunk appear more frequently.
	 * Ignored when bIsEndChunk is true (end chunk is never selected from the pool).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Chunk|Pool",
	          meta=(ClampMin=0.f, UIMin=0.f))
	float SpawnWeight = 1.f;

	/**
	 * If true this is the special closing chunk placed by the manager once all
	 * enemies have been defeated.  It is never added to the normal random pool.
	 * Only one definition should have this flag set; if multiple do, the manager
	 * uses its dedicated EndChunkDefinition property instead of this flag.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Chunk|Pool")
	bool bIsEndChunk = false;

	// ── Enemy Accounting ──────────────────────────────────────────────────────

	/**
	 * Number of enemies this chunk is expected to spawn.
	 * The manager adds this value to its total-enemy counter when the chunk is
	 * activated, so that GetRemainingEnemies() is correct from the moment a chunk
	 * appears rather than waiting for each enemy to be created.
	 *
	 * Set to 0 for chunks that contain no enemies (pure road / obstacle chunks).
	 * If your chunk spawns enemies dynamically in Blueprint and the count can vary,
	 * leave this at 0 and call AMYSTChaseTrackManager::RegisterEnemySpawned() from
	 * the chunk's K2_OnChunkActivated event instead.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Chunk|Enemies",
	          meta=(ClampMin=0, UIMin=0))
	int32 ExpectedEnemyCount = 0;

	// ── Metadata / Tagging ────────────────────────────────────────────────────

	/**
	 * Optional free-form display name shown in editor logs and debug HUDs.
	 * Defaults to the asset name if left empty.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Chunk|Meta")
	FName ChunkDisplayName = NAME_None;

	// ── Helpers ───────────────────────────────────────────────────────────────

	/** Returns ChunkDisplayName if set, otherwise the asset object name. */
	UFUNCTION(BlueprintPure, Category="MYST|ChunkDefinition")
	FString GetDisplayName() const;
};

