// Copyright MyShooterScenarios. All Rights Reserved.
#include "CarChase/AMYSTChaseTrackManager.h"
#include "CarChase/AMYSTRoadChunkActor.h"
#include "CarChase/MYSTChunkDefinition.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(AMYSTChaseTrackManager)
DEFINE_LOG_CATEGORY_STATIC(LogMYSTChaseTrack, Log, All);
AMYSTChaseTrackManager::AMYSTChaseTrackManager(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
PrimaryActorTick.bCanEverTick = true;
PrimaryActorTick.bStartWithTickEnabled = false; // enabled by StartChase()
}
// =============================================================================
// AActor interface
// =============================================================================
void AMYSTChaseTrackManager::BeginPlay()
{
Super::BeginPlay();
// Cache track basis vectors from the actor's current transform.
TrackOrigin  = GetActorLocation();
TrackForward = GetActorForwardVector();
// Auto-assign tracked pawn from the first local player if not set manually.
if (!TrackedPawn)
{
if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
{
TrackedPawn = PC->GetPawnOrSpectator();
}
}
if (bAutoStartOnBeginPlay)
{
StartChase();
}
}
void AMYSTChaseTrackManager::Tick(float DeltaSeconds)
{
Super::Tick(DeltaSeconds);
if (ChaseState != EMYSTChaseState::Running && ChaseState != EMYSTChaseState::Finishing)
{
return;
}
// Update travel distance from pawn position.
TravelDistance = ComputeTravelDistance();
// Recycle chunks that have fallen behind the tracked pawn.
RecycleStaleChunks();
// Spawn new chunks to maintain the lookahead window (Running state only).
if (ChaseState == EMYSTChaseState::Running)
{
MaintainChunkWindow();
}
// In Finishing: check whether the player has passed through the end chunk.
if (ChaseState == EMYSTChaseState::Finishing && bEndChunkSpawned)
{
// All active chunks have been recycled -> the player passed the last one.
if (ActiveChunks.Num() == 0)
{
CompleteChase();
}
}
}
// =============================================================================
// Public API
// =============================================================================
void AMYSTChaseTrackManager::StartChase()
{
if (ChaseState == EMYSTChaseState::Running)
{
return;
}
ChaseState         = EMYSTChaseState::Running;
TravelDistance     = 0.f;
NextSpawnDistance  = 0.f;
bEndChunkSpawned   = false;
TotalEnemiesTracked = 0;
TotalEnemiesKilled  = 0;
ActiveChunks.Reset();
SetActorTickEnabled(true);
MaintainChunkWindow();
K2_OnChaseStarted();
UE_LOG(LogMYSTChaseTrack, Log, TEXT("Chase started. Origin=%s Forward=%s"),
       *TrackOrigin.ToCompactString(), *TrackForward.ToCompactString());
}
void AMYSTChaseTrackManager::StopChase()
{
if (ChaseState == EMYSTChaseState::Running || ChaseState == EMYSTChaseState::Finishing)
{
ChaseState = EMYSTChaseState::Stopped;
SetActorTickEnabled(false);
}
}
void AMYSTChaseTrackManager::ResumeChase()
{
if (ChaseState == EMYSTChaseState::Stopped)
{
ChaseState = EMYSTChaseState::Running;
SetActorTickEnabled(true);
}
}
void AMYSTChaseTrackManager::ForceFinish()
{
if (ChaseState == EMYSTChaseState::Running || ChaseState == EMYSTChaseState::Stopped)
{
BeginFinishing();
}
}
// ---- Enemy Accounting -------------------------------------------------------
void AMYSTChaseTrackManager::RegisterEnemySpawned(int32 Count)
{
TotalEnemiesTracked += FMath::Max(0, Count);
}
void AMYSTChaseTrackManager::RegisterEnemyKilled(int32 Count)
{
TotalEnemiesKilled = FMath::Min(TotalEnemiesKilled + FMath::Max(0, Count), TotalEnemiesTracked);
UE_CLOG(bDebugDraw, LogMYSTChaseTrack, Log,
        TEXT("Enemy killed. Killed=%d / Tracked=%d"), TotalEnemiesKilled, TotalEnemiesTracked);
// Trigger completion only when we have tracked at least one enemy and all are dead.
if (TotalEnemiesTracked > 0 && TotalEnemiesKilled >= TotalEnemiesTracked)
{
if (ChaseState == EMYSTChaseState::Running)
{
K2_OnAllEnemiesDefeated();
BeginFinishing();
}
}
}
// ---- Pawn -------------------------------------------------------------------
void AMYSTChaseTrackManager::SetTrackedPawn(APawn* NewPawn)
{
TrackedPawn = NewPawn;
}
// ---- Queries ----------------------------------------------------------------
int32 AMYSTChaseTrackManager::GetRemainingEnemies() const
{
return FMath::Max(0, TotalEnemiesTracked - TotalEnemiesKilled);
}
// =============================================================================
// Protected helpers
// =============================================================================
UMYSTChunkDefinition* AMYSTChaseTrackManager::PickNextChunkDefinition() const
{
// Build a list of valid (non-end-chunk, non-null-class) entries and their
// accumulated weights for weighted random selection.
float TotalWeight = 0.f;
struct FWeightedEntry
{
UMYSTChunkDefinition* Def;
float Weight;
};
TArray<FWeightedEntry> ValidEntries;
ValidEntries.Reserve(ChunkPool.Num());
for (UMYSTChunkDefinition* Def : ChunkPool)
{
if (!Def || !Def->ChunkClass || Def->bIsEndChunk || Def->SpawnWeight <= 0.f)
{
continue;
}
TotalWeight += Def->SpawnWeight;
ValidEntries.Add({ Def, Def->SpawnWeight });
}
if (ValidEntries.IsEmpty())
{
UE_LOG(LogMYSTChaseTrack, Warning, TEXT("PickNextChunkDefinition: no valid entries in ChunkPool!"));
return nullptr;
}
float Roll = FMath::FRandRange(0.f, TotalWeight);
for (const FWeightedEntry& Entry : ValidEntries)
{
Roll -= Entry.Weight;
if (Roll <= 0.f)
{
return Entry.Def;
}
}
// Fallback (floating-point edge case): return the last valid entry.
return ValidEntries.Last().Def;
}
void AMYSTChaseTrackManager::SpawnChunk(UMYSTChunkDefinition* Definition)
{
if (!Definition || !Definition->ChunkClass)
{
return;
}
UWorld* World = GetWorld();
if (!World)
{
return;
}
// Compute spawn location: origin + forward * nextSpawnDistance, at our Z and rotation.
const FVector SpawnLocation = TrackOrigin + TrackForward * NextSpawnDistance;
const FRotator SpawnRotation = TrackForward.Rotation();
FActorSpawnParameters SpawnParams;
SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
SpawnParams.Owner = this;
AMYSTRoadChunkActor* NewChunk = World->SpawnActor<AMYSTRoadChunkActor>(
Definition->ChunkClass, SpawnLocation, SpawnRotation, SpawnParams);
if (!NewChunk)
{
UE_LOG(LogMYSTChaseTrack, Error, TEXT("SpawnChunk: failed to spawn actor for definition '%s'"),
       *GetNameSafe(Definition));
return;
}
// Record in active list before calling ActivateChunk so Blueprint events
// can safely query the manager during activation.
FMYSTActiveChunkEntry& Entry = ActiveChunks.AddDefaulted_GetRef();
Entry.Actor         = NewChunk;
Entry.StartDistance = NextSpawnDistance;
// Activate (fires K2_OnChunkActivated, which may call RegisterEnemySpawned).
NewChunk->ActivateChunk(this, Definition);
// After activation the chunk knows its effective length (Blueprint may override).
const float ChunkLength = NewChunk->GetChunkLength();
Entry.EndDistance = NextSpawnDistance + ChunkLength;
// Add pre-declared enemy count AFTER activation so any dynamic additions
// via NotifyEnemySpawned() inside K2_OnChunkActivated are not double-counted.
if (Definition->ExpectedEnemyCount > 0)
{
TotalEnemiesTracked += Definition->ExpectedEnemyCount;
}
NextSpawnDistance += ChunkLength;
UE_CLOG(bDebugDraw, LogMYSTChaseTrack, Log,
        TEXT("Spawned chunk '%s' at dist=%.0f .. %.0f"),
        *GetNameSafe(Definition), Entry.StartDistance, Entry.EndDistance);
K2_OnChunkSpawned(NewChunk);
}
void AMYSTChaseTrackManager::RecycleStaleChunks()
{
// Chunks are stored oldest-first.  Only the front of the array can be stale.
while (ActiveChunks.Num() > 0)
{
FMYSTActiveChunkEntry& Front = ActiveChunks[0];
// Recycle once the chunk's exit is behind the player (plus RecycleBuffer).
if (Front.EndDistance > TravelDistance - RecycleBuffer)
{
break; // This chunk -- and all behind it -- are still relevant.
}
if (AMYSTRoadChunkActor* Chunk = Front.Actor.Get())
{
UE_CLOG(bDebugDraw, LogMYSTChaseTrack, Log,
        TEXT("Recycling chunk '%s' (endDist=%.0f, travel=%.0f)"),
        *GetNameSafe(Chunk), Front.EndDistance, TravelDistance);
K2_OnChunkRecycled(Chunk);
Chunk->DeactivateChunk();
Chunk->Destroy();
}
ActiveChunks.RemoveAt(0, 1, EAllowShrinking::No);
}
}
void AMYSTChaseTrackManager::MaintainChunkWindow()
{
// Keep spawning until we have at least ChunksToKeepAhead total active chunks.
// This is simple and correct: the pool defines all spacing.
while (ActiveChunks.Num() < ChunksToKeepAhead)
{
UMYSTChunkDefinition* NextDef = PickNextChunkDefinition();
if (!NextDef)
{
break;
}
SpawnChunk(NextDef);
}
}
float AMYSTChaseTrackManager::ComputeTravelDistance() const
{
if (!TrackedPawn)
{
return TravelDistance; // Keep previous value if pawn is missing.
}
const float Projected = FVector::DotProduct(
TrackedPawn->GetActorLocation() - TrackOrigin, TrackForward);
// Clamp to 0 so we never report negative travel (e.g. if pawn spawns slightly behind origin).
return FMath::Max(0.f, Projected);
}
void AMYSTChaseTrackManager::BeginFinishing()
{
ChaseState = EMYSTChaseState::Finishing;
if (EndChunkDefinition && !bEndChunkSpawned)
{
bEndChunkSpawned = true;
SpawnChunk(EndChunkDefinition);
}
else if (!EndChunkDefinition)
{
// No end chunk -- complete immediately.
CompleteChase();
}
}
void AMYSTChaseTrackManager::CompleteChase()
{
ChaseState = EMYSTChaseState::Complete;
SetActorTickEnabled(false);
UE_LOG(LogMYSTChaseTrack, Log, TEXT("Chase complete. Total travel=%.0f cm, enemies=%d/%d"),
       TravelDistance, TotalEnemiesKilled, TotalEnemiesTracked);
K2_OnChaseComplete();
}
