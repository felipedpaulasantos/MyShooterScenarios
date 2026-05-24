// Copyright MyShooterScenarios. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AMYSTChaseTrackManager.generated.h"
class AMYSTRoadChunkActor;
class UMYSTChunkDefinition;
// -----------------------------------------------------------------------------
// Internal bookkeeping struct -- not exposed to Blueprint
// -----------------------------------------------------------------------------
USTRUCT()
struct FMYSTActiveChunkEntry
{
GENERATED_BODY()
/** The live actor for this chunk. */
UPROPERTY()
TObjectPtr<AMYSTRoadChunkActor> Actor = nullptr;
/** Distance along the track at which this chunk begins (cm). */
float StartDistance = 0.f;
/** Distance along the track at which this chunk ends (cm). */
float EndDistance = 0.f;
};
// -----------------------------------------------------------------------------
// EChaseState
// -----------------------------------------------------------------------------
/**
 * Current state of the chase track manager.
 */
UENUM(BlueprintType)
enum class EMYSTChaseState : uint8
{
/** BeginPlay has not finished or StartChase() was never called. */
Idle        UMETA(DisplayName="Idle"),
/** Chase is actively running -- chunks are being spawned and recycled. */
Running     UMETA(DisplayName="Running"),
/**
 * All registered enemies are dead.  The end chunk has been spawned.
 * The state transitions to Complete once the player passes through it.
 */
Finishing   UMETA(DisplayName="Finishing"),
/** End chunk has been fully traversed.  The manager stops ticking. */
Complete    UMETA(DisplayName="Complete"),
/** StopChase() was called explicitly before natural completion. */
Stopped     UMETA(DisplayName="Stopped"),
};
// -----------------------------------------------------------------------------
// AMYSTChaseTrackManager
// -----------------------------------------------------------------------------
/**
 * AMYSTChaseTrackManager
 *
 * Place one of these actors in your car-chase map.  It maintains a sliding
 * window of AMYSTRoadChunkActor instances, spawning new chunks ahead of the
 * tracked pawn and recycling old ones as they fall behind.
 *
 * SETUP CHECKLIST:
 *  1. Place AMYSTChaseTrackManager in the level, face it in the travel direction.
 *  2. Assign ChunkPool (array of UMYSTChunkDefinition assets).
 *  3. Optionally assign EndChunkDefinition (spawned when all enemies are killed).
 *  4. Set TrackedPawn in the Details panel, OR leave null to auto-grab the first
 *     local player pawn at BeginPlay.
 *  5. Set bAutoStartOnBeginPlay=true, or call StartChase() from Blueprint.
 *
 * ENEMY ACCOUNTING:
 *  The manager totals enemies from two sources:
 *    a) UMYSTChunkDefinition::ExpectedEnemyCount -- added when each chunk spawns.
 *    b) AMYSTRoadChunkActor::NotifyEnemySpawned() -- for dynamic spawn counts.
 *  Deaths are registered via AMYSTRoadChunkActor::NotifyEnemyKilled(), which
 *  calls RegisterEnemyKilled() here.  The manager fires K2_OnAllEnemiesDefeated
 *  and transitions to Finishing when TotalKilled >= TotalTracked and
 *  TotalTracked > 0.
 *
 *  If your game does not track enemies, leave ExpectedEnemyCount = 0 on all
 *  definitions and call ForceFinish() from Blueprint when you want to end.
 */
UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="MYST Chase Track Manager"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API AMYSTChaseTrackManager : public AActor
{
GENERATED_BODY()
public:
AMYSTChaseTrackManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
//~AActor interface
virtual void BeginPlay() override;
virtual void Tick(float DeltaSeconds) override;
//~End of AActor interface
// =========================================================================
// Configuration (edit in Details panel or via Blueprint CDO)
// =========================================================================
// ---- Pool ----------------------------------------------------------------
/**
 * Pool of chunk definitions to draw from at random (weighted).
 * At least one entry is required.  Entries with ChunkClass=null are skipped.
 * Entries with bIsEndChunk=true are skipped here (use EndChunkDefinition instead).
 */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chase|Pool")
TArray<TObjectPtr<UMYSTChunkDefinition>> ChunkPool;
/**
 * The special closing chunk spawned once all enemies are defeated.
 * If null, the chase completes immediately after all enemies die without
 * spawning an extra closing segment.
 */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chase|Pool")
TObjectPtr<UMYSTChunkDefinition> EndChunkDefinition;
// ---- Track ---------------------------------------------------------------
/**
 * Minimum number of chunks to keep ahead of (or around) the tracked pawn.
 * Increasing this value pre-loads more geometry in exchange for more memory.
 */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chase|Track",
          meta=(ClampMin=1, UIMin=1))
int32 ChunksToKeepAhead = 3;
/**
 * Extra distance behind the tracked pawn (cm) before a chunk is recycled.
 * A small positive value prevents visible pop-in of destruction effects.
 */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chase|Track",
          meta=(ClampMin=0.f, UIMin=0.f, ForceUnits="cm"))
float RecycleBuffer = 500.f;
// ---- Pawn Reference ------------------------------------------------------
/**
 * The pawn whose world position drives the travel distance calculation.
 * If null at BeginPlay, the manager auto-assigns the first local player pawn.
 * You can also call SetTrackedPawn() at runtime from Blueprint.
 */
UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Chase|Pawn")
TObjectPtr<APawn> TrackedPawn;
// ---- Misc ----------------------------------------------------------------
/**
 * When true, StartChase() is called automatically during BeginPlay.
 * Set to false if you want to trigger the start from Blueprint
 * (e.g. after a cutscene finishes).
 */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chase|Misc")
bool bAutoStartOnBeginPlay = true;
/**
 * When true, detailed chunk spawn/recycle events are printed to the screen
 * and log during PIE.  Has no effect in shipping builds.
 */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chase|Misc")
bool bDebugDraw = false;
// =========================================================================
// Public API
// =========================================================================
// ---- Control -------------------------------------------------------------
/**
 * Begins the chase: spawns the initial window of chunks and starts ticking.
 * No-op if the chase is already running.
 */
UFUNCTION(BlueprintCallable, Category="MYST|ChaseTrackManager")
void StartChase();
/**
 * Immediately stops spawning/recycling and freezes the current state.
 * Does NOT destroy active chunks.  Call ResumeChase() to continue.
 */
UFUNCTION(BlueprintCallable, Category="MYST|ChaseTrackManager")
void StopChase();
/**
 * Resumes a stopped chase.  No-op unless the current state is Stopped.
 */
UFUNCTION(BlueprintCallable, Category="MYST|ChaseTrackManager")
void ResumeChase();
/**
 * Forces the manager into the Finishing state regardless of enemy count.
 * Use this when your game logic decides the chase should end for a reason
 * other than killing all enemies (e.g. time limit, scripted trigger).
 */
UFUNCTION(BlueprintCallable, Category="MYST|ChaseTrackManager")
void ForceFinish();
// ---- Enemy Accounting ----------------------------------------------------
/**
 * Inform the manager that one or more enemies have been spawned by a chunk.
 * Call this from AMYSTRoadChunkActor::NotifyEnemySpawned() (already wired)
 * when the chunk spawns enemies dynamically at runtime.
 */
UFUNCTION(BlueprintCallable, Category="MYST|ChaseTrackManager")
void RegisterEnemySpawned(int32 Count = 1);
/**
 * Inform the manager that one or more enemies have been killed.
 * Normally called via AMYSTRoadChunkActor::NotifyEnemyKilled() from Blueprint
 * inside the enemy's death event.
 *
 * Fires K2_OnAllEnemiesDefeated and transitions to Finishing when the kill
 * count reaches the total tracked count.
 */
UFUNCTION(BlueprintCallable, Category="MYST|ChaseTrackManager")
void RegisterEnemyKilled(int32 Count = 1);
// ---- Pawn ----------------------------------------------------------------
/**
 * Overrides the pawn used for travel-distance calculations at runtime.
 * Useful when the player switches vehicles mid-chase.
 */
UFUNCTION(BlueprintCallable, Category="MYST|ChaseTrackManager")
void SetTrackedPawn(APawn* NewPawn);
// ---- Queries -------------------------------------------------------------
/** Returns the current state of the chase. */
UFUNCTION(BlueprintPure, Category="MYST|ChaseTrackManager")
EMYSTChaseState GetChaseState() const { return ChaseState; }
/**
 * Returns the total distance (cm) traveled by the tracked pawn along the
 * track since the chase started.
 */
UFUNCTION(BlueprintPure, Category="MYST|ChaseTrackManager")
float GetTravelDistance() const { return TravelDistance; }
/**
 * Returns the total number of enemies registered (spawned) so far.
 * This grows as new enemy-containing chunks are activated.
 */
UFUNCTION(BlueprintPure, Category="MYST|ChaseTrackManager")
int32 GetTotalEnemiesTracked() const { return TotalEnemiesTracked; }
/** Returns the total number of enemies killed so far. */
UFUNCTION(BlueprintPure, Category="MYST|ChaseTrackManager")
int32 GetTotalEnemiesKilled() const { return TotalEnemiesKilled; }
/**
 * Returns the number of enemies still alive (tracked minus killed).
 * Returns 0 after all enemies are defeated or when none are tracked.
 */
UFUNCTION(BlueprintPure, Category="MYST|ChaseTrackManager")
int32 GetRemainingEnemies() const;
/** Returns the number of currently active (visible) chunks. */
UFUNCTION(BlueprintPure, Category="MYST|ChaseTrackManager")
int32 GetActiveChunkCount() const { return ActiveChunks.Num(); }
/**
 * Returns the world-space origin of the track (the manager's location at
 * BeginPlay) used as the zero-distance reference point.
 */
UFUNCTION(BlueprintPure, Category="MYST|ChaseTrackManager")
FVector GetTrackOrigin() const { return TrackOrigin; }
/**
 * Returns the normalised forward direction of the track
 * (the manager's forward vector at BeginPlay).
 */
UFUNCTION(BlueprintPure, Category="MYST|ChaseTrackManager")
FVector GetTrackForward() const { return TrackForward; }
// =========================================================================
// Blueprint Implementable Events
// =========================================================================
/**
 * Called when StartChase() begins running.
 * Use this to start vehicle movement, play music, etc.
 */
UFUNCTION(BlueprintImplementableEvent, Category="MYST|ChaseTrackManager",
          meta=(DisplayName="On Chase Started"))
void K2_OnChaseStarted();
/**
 * Called every time a new chunk is spawned and activated.
 *
 * @param NewChunk  The freshly activated chunk actor.
 */
UFUNCTION(BlueprintImplementableEvent, Category="MYST|ChaseTrackManager",
          meta=(DisplayName="On Chunk Spawned"))
void K2_OnChunkSpawned(AMYSTRoadChunkActor* NewChunk);
/**
 * Called every time a chunk is recycled (deactivated and destroyed).
 *
 * @param OldChunk  The chunk actor that is about to be destroyed.
 *                  Still valid inside this event; do not store a reference.
 */
UFUNCTION(BlueprintImplementableEvent, Category="MYST|ChaseTrackManager",
          meta=(DisplayName="On Chunk Recycled"))
void K2_OnChunkRecycled(AMYSTRoadChunkActor* OldChunk);
/**
 * Called when the last tracked enemy is killed.
 * The manager will spawn the EndChunkDefinition (if set) and transition
 * to the Finishing state immediately after this event returns.
 */
UFUNCTION(BlueprintImplementableEvent, Category="MYST|ChaseTrackManager",
          meta=(DisplayName="On All Enemies Defeated"))
void K2_OnAllEnemiesDefeated();
/**
 * Called when the player has fully passed through the end chunk (or
 * immediately after K2_OnAllEnemiesDefeated when EndChunkDefinition is null).
 * Use this to trigger a victory screen, stop music, load the next map, etc.
 */
UFUNCTION(BlueprintImplementableEvent, Category="MYST|ChaseTrackManager",
          meta=(DisplayName="On Chase Complete"))
void K2_OnChaseComplete();
protected:
// =========================================================================
// Internal helpers
// =========================================================================
/**
 * Selects the next chunk definition from ChunkPool using weighted random
 * selection.  Returns null if the pool is empty or all entries are invalid.
 */
UFUNCTION(BlueprintCallable, Category="MYST|ChaseTrackManager",
          meta=(BlueprintProtected, DisplayName="Pick Next Chunk Definition"))
UMYSTChunkDefinition* PickNextChunkDefinition() const;
/** Spawns and activates the given definition at NextSpawnDistance. */
void SpawnChunk(UMYSTChunkDefinition* Definition);
/** Recycles (deactivates + destroys) all chunks that have fallen behind. */
void RecycleStaleChunks();
/**
 * Ensures at least ChunksToKeepAhead chunks exist ahead of TravelDistance.
 * Skips spawning if the chase is in Finishing/Complete/Stopped state.
 */
void MaintainChunkWindow();
/**
 * Computes the current travel distance from the tracked pawn's world location
 * projected onto the track axis.
 */
float ComputeTravelDistance() const;
/** Transitions to the Finishing state and spawns the end chunk. */
void BeginFinishing();
/** Transitions to Complete and fires K2_OnChaseComplete. */
void CompleteChase();
private:
// ---- Track metadata (cached at BeginPlay) --------------------------------
/** World-space position of the manager actor when the chase started. */
FVector TrackOrigin = FVector::ZeroVector;
/** Normalised forward vector of the manager actor when the chase started. */
FVector TrackForward = FVector::ForwardVector;
// ---- Runtime state -------------------------------------------------------
EMYSTChaseState ChaseState = EMYSTChaseState::Idle;
/** Cumulative distance the tracked pawn has traveled along the track axis (cm). */
float TravelDistance = 0.f;
/** World-space distance along the track at which the next chunk will be placed. */
float NextSpawnDistance = 0.f;
/** Whether the end chunk has already been queued during the Finishing phase. */
bool bEndChunkSpawned = false;
// ---- Enemy counters ------------------------------------------------------
int32 TotalEnemiesTracked = 0;
int32 TotalEnemiesKilled  = 0;
// ---- Active chunk pool ---------------------------------------------------
/** Ordered list of currently active chunks (oldest first). */
UPROPERTY()
TArray<FMYSTActiveChunkEntry> ActiveChunks;
};
