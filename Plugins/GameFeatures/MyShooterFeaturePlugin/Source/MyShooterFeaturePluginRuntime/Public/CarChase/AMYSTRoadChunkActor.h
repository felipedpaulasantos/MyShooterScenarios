// Copyright MyShooterScenarios. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AMYSTRoadChunkActor.generated.h"
class AMYSTChaseTrackManager;
class UMYSTChunkDefinition;
class UArrowComponent;
/**
 * AMYSTRoadChunkActor
 *
 * Base C++ class for all road segment actors used by the treadmill system.
 * Create Blueprint subclasses (e.g. B_Chunk_Straight, B_Chunk_Ambush) and
 * override the BlueprintImplementableEvents to add meshes, obstacles, enemy
 * spawners, Niagara effects, etc.
 *
 * LIFECYCLE:
 *  1. Manager spawns the actor at the next spawn position on the track.
 *  2. Manager calls ActivateChunk() -> triggers K2_OnChunkActivated().
 *  3. Player travels through the chunk.
 *  4. When the chunk falls behind the recycle boundary, DeactivateChunk() is
 *     called -> triggers K2_OnChunkDeactivated(), then the actor is destroyed.
 *
 * ENEMY TRACKING:
 * When an enemy spawned inside this chunk dies, call NotifyEnemyKilled() from
 * Blueprint.  The chunk forwards the event to the owning manager.
 * If the chunk spawns a variable number of enemies at runtime, also call
 * NotifyEnemySpawned() so the manager can widen its total count.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, meta=(DisplayName="MYST Road Chunk Actor"))
class MYSHOOTERFEATUREPLUGINRUNTIME_API AMYSTRoadChunkActor : public AActor
{
GENERATED_BODY()
public:
AMYSTRoadChunkActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
//~AActor interface
virtual void BeginPlay() override;
virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
//~End of AActor interface
// ---- Activation ----------------------------------------------------------
/** Called by the manager immediately after spawning. Fires K2_OnChunkActivated. */
void ActivateChunk(AMYSTChaseTrackManager* InManager, const UMYSTChunkDefinition* InDefinition);
/** Called by the manager just before destruction. Fires K2_OnChunkDeactivated. */
void DeactivateChunk();
// ---- Enemy Accounting ----------------------------------------------------
/**
 * Call from Blueprint when an enemy spawned by this chunk is killed.
 * Forwards to the owning manager's RegisterEnemyKilled().
 */
UFUNCTION(BlueprintCallable, Category="MYST|RoadChunk", meta=(DisplayName="Notify Enemy Killed"))
void NotifyEnemyKilled(int32 Count = 1);
/**
 * Call from Blueprint after dynamically spawning an enemy whose count was
 * NOT pre-declared in ExpectedEnemyCount. Expands the manager's total counter.
 */
UFUNCTION(BlueprintCallable, Category="MYST|RoadChunk", meta=(DisplayName="Notify Enemy Spawned"))
void NotifyEnemySpawned(int32 Count = 1);
// ---- Queries -------------------------------------------------------------
/** Returns the chunk definition asset assigned when this chunk was activated. */
UFUNCTION(BlueprintPure, Category="MYST|RoadChunk")
const UMYSTChunkDefinition* GetChunkDefinition() const { return ChunkDefinition; }
/** Returns the manager that owns this chunk, or null if not yet activated. */
UFUNCTION(BlueprintPure, Category="MYST|RoadChunk")
AMYSTChaseTrackManager* GetOwningManager() const { return OwningManager; }
/**
 * Returns the world-space location at the exit (far end) of this chunk.
 * Default: ActorLocation + GetActorForwardVector() * GetChunkLength().
 */
UFUNCTION(BlueprintPure, Category="MYST|RoadChunk")
FVector GetChunkExitWorldLocation() const;
/**
 * Effective length of this chunk along the track axis (cm).
 * Calls K2_GetChunkLength for a Blueprint override; otherwise uses Definition.ChunkLength.
 */
UFUNCTION(BlueprintPure, Category="MYST|RoadChunk")
float GetChunkLength() const;
// ---- Blueprint Implementable Events --------------------------------------
/**
 * Called after this chunk is spawned and positioned on the track.
 * Use this event to spawn obstacles, configure enemy spawners, play VFX, etc.
 *
 * @param Definition  The data asset describing this chunk's configuration.
 */
UFUNCTION(BlueprintImplementableEvent, Category="MYST|RoadChunk", meta=(DisplayName="On Chunk Activated"))
void K2_OnChunkActivated(const UMYSTChunkDefinition* Definition);
/**
 * Called just before this chunk is destroyed (recycled off the back of the track).
 * Use this to stop looping effects or destroy any lingering child actors.
 */
UFUNCTION(BlueprintImplementableEvent, Category="MYST|RoadChunk", meta=(DisplayName="On Chunk Deactivated"))
void K2_OnChunkDeactivated();
/**
 * Optional Blueprint override: return a custom chunk length for this instance.
 * If not implemented in Blueprint, the definition's ChunkLength is used.
 *
 * @param DefinitionLength  Length stored in the definition asset (cm).
 * @return                  Effective length to use (cm).
 */
UFUNCTION(BlueprintImplementableEvent, Category="MYST|RoadChunk", meta=(DisplayName="Get Chunk Length Override"))
float K2_GetChunkLength(float DefinitionLength) const;
protected:
#if WITH_EDITORONLY_DATA
/** Arrow pointing in the track forward direction.  Editor-only visual aid. */
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Chunk|Components")
TObjectPtr<UArrowComponent> ForwardArrow;
#endif
private:
/** The manager that spawned this chunk.  Not replicated. */
UPROPERTY()
TObjectPtr<AMYSTChaseTrackManager> OwningManager;
/** The definition asset for this instance.  Not replicated. */
UPROPERTY()
TObjectPtr<const UMYSTChunkDefinition> ChunkDefinition;
};
