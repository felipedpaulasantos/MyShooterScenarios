# MYST Car Chase / Treadmill System — Implementation Guide
## Overview
The Car Chase system provides a **procedural "infinite road" treadmill** for a constant-speed chase level.
A sliding window of road-segment actors is kept in front of the player; as the player moves forward, segments
behind them are destroyed and new ones are spawned ahead — creating the illusion of an endless track.
The system is built from three C++ classes that you subclass and configure entirely in **Blueprint and Data Assets**,
with no C++ changes required for typical gameplay authoring.
```
UMYSTChunkDefinition  (Data Asset)
        │  describes ──►  AMYSTRoadChunkActor  (Blueprint subclass)
        │
AMYSTChaseTrackManager  (placed in level)
        │  spawns / recycles ──►  AMYSTRoadChunkActor instances
        │  reports ──►  Blueprint events (OnChaseStarted, OnChunkSpawned, OnAllEnemiesDefeated, …)
```
---
## Core Classes
### `UMYSTChunkDefinition`
**Type:** `UPrimaryDataAsset`  
**Create:** *Content Browser → Miscellaneous → Data Asset → MYST Chunk Definition*
| Property | Type | Purpose |
|---|---|---|
| `ChunkClass` | `TSubclassOf<AMYSTRoadChunkActor>` | Which Blueprint actor to spawn for this segment. |
| `ChunkLength` | `float` (cm) | Physical length along the travel axis. Must match the road mesh. |
| `SpawnWeight` | `float` | Relative probability in the random pool. Higher = appears more often. |
| `bIsEndChunk` | `bool` | Marks this as the closing segment. Never selected from the random pool. |
| `ExpectedEnemyCount` | `int32` | Pre-declared enemy count added to the global total when the chunk activates. Set to 0 when enemies are spawned dynamically. |
| `ChunkDisplayName` | `FName` | Optional label for debug logs. Defaults to the asset name. |
---
### `AMYSTRoadChunkActor`
**Type:** `AActor` (Abstract)  
**Subclass in Blueprint:** `B_Chunk_Straight`, `B_Chunk_Ambush`, `B_Chunk_End`, etc.
#### Blueprint Events to implement
| Event | When It Fires | Typical Use |
|---|---|---|
| `On Chunk Activated (Definition)` | After the manager spawns and positions this chunk. | Spawn obstacles, configure AI spawn points, play intro Niagara burst. |
| `On Chunk Deactivated` | Just before the actor is destroyed (recycled). | Stop looping VFX, destroy any child actors you own. |
| `Get Chunk Length Override (DefinitionLength)` | When the manager queries the chunk's length. | Return a custom value for procedurally-sized chunks. Leave unimplemented to use `ChunkLength` from the definition. |
#### Blueprint Callable Functions
| Function | Purpose |
|---|---|
| `Notify Enemy Killed (Count)` | Call from an enemy's death event. Forwards to the manager's kill counter. |
| `Notify Enemy Spawned (Count)` | Call after dynamically spawning enemies whose count wasn't pre-declared in `ExpectedEnemyCount`. |
| `Get Chunk Definition` | Returns the `UMYSTChunkDefinition` asset assigned to this instance. |
| `Get Owning Manager` | Returns the `AMYSTChaseTrackManager` that spawned this chunk. |
| `Get Chunk Exit World Location` | World-space position at the far end of the chunk (useful for placing spawners relative to the track exit). |
---
### `AMYSTChaseTrackManager`
**Type:** `AActor`  
**Subclass in Blueprint:** `B_ChaseTrackManager` (recommended)  
**Placement:** Drop one instance into the chase map, **facing the travel direction** (the actor's +X axis becomes the track forward).
#### Configuration (Details Panel)
| Property | Default | Purpose |
|---|---|---|
| `ChunkPool` | *(empty)* | Array of `UMYSTChunkDefinition` assets to pick from at random. |
| `EndChunkDefinition` | `null` | The special closing segment spawned after all enemies die. Optional. |
| `ChunksToKeepAhead` | `3` | How many chunk actors to maintain in the world at once. |
| `RecycleBuffer` | `500 cm` | Extra distance behind the pawn before a chunk is destroyed. Prevents pop-out of destruction effects. |
| `TrackedPawn` | `null` | The pawn whose position drives the travel calculation. Auto-grabs the first local player pawn at BeginPlay if left null. |
| `bAutoStartOnBeginPlay` | `true` | Call `StartChase()` automatically. Set false to trigger via Blueprint (e.g. after a cutscene). |
| `bDebugDraw` | `false` | Print spawn / recycle / kill events to log and screen during PIE. |
#### Blueprint Events to implement
| Event | When It Fires | Typical Use |
|---|---|---|
| `On Chase Started` | When `StartChase()` begins. | Start vehicle movement, fade in music. |
| `On Chunk Spawned (NewChunk)` | Each time a new chunk is activated. | HUD counter, spawn debug visualisers. |
| `On Chunk Recycled (OldChunk)` | Just before a chunk is destroyed. | Accumulate procedural history, analytics. |
| `On All Enemies Defeated` | When `TotalKilled >= TotalTracked` (and `TotalTracked > 0`). | Play a "all clear" sting, remove enemy-count HUD, start end sequence. |
| `On Chase Complete` | After the end chunk is fully passed (or immediately if no `EndChunkDefinition` is set). | Load next map, show score screen, roll credits. |
#### Blueprint Callable Functions
| Function | Purpose |
|---|---|
| `StartChase()` | Begin the treadmill. Resets all counters and spawns the initial chunk window. |
| `StopChase()` | Pause spawning/recycling. Active chunks remain in the world. |
| `ResumeChase()` | Continue after `StopChase()`. |
| `ForceFinish()` | Skip enemy accounting and go straight to Finishing. |
| `SetTrackedPawn(Pawn)` | Switch the tracked pawn at runtime (e.g. player enters a second vehicle). |
| `RegisterEnemyKilled(Count)` | Directly notify the manager of kills (use if bypassing the chunk layer). |
| `RegisterEnemySpawned(Count)` | Directly notify the manager of new enemies (use if bypassing the chunk layer). |
| `GetChaseState()` | Returns `EMYSTChaseState` (Idle / Running / Finishing / Complete / Stopped). |
| `GetTravelDistance()` | Cumulative cm the tracked pawn has traveled along the track axis. |
| `GetRemainingEnemies()` | `TotalTracked - TotalKilled`, clamped to 0. |
| `GetActiveChunkCount()` | Number of chunk actors currently alive in the world. |
| `PickNextChunkDefinition()` | Protected. Callable from Blueprint subclass to customise selection logic. |
---
## State Machine
```
          StartChase()
  [Idle] ──────────────► [Running]
                              │
                              │  All enemies killed  ──► K2_OnAllEnemiesDefeated
                              │  OR ForceFinish()
                              ▼
                         [Finishing]  ── spawns EndChunkDefinition
                              │
                              │  Player passes end chunk
                              │  (or EndChunkDefinition is null)
                              ▼
                         [Complete]  ──► K2_OnChaseComplete
          StopChase()          ResumeChase()
  [Running/Finishing] ◄──── [Stopped] ────►  [Running/Finishing]
```
---
## Enemy Tracking
The manager maintains two counters: `TotalEnemiesTracked` and `TotalEnemiesKilled`.
### How enemies are counted IN
Two mutually-exclusive methods (you can mix them per-chunk):
**Method A — Static count (preferred for fixed spawners)**
Set `ExpectedEnemyCount` on the `UMYSTChunkDefinition`.  
The manager adds this value when the chunk activates.
**Method B — Dynamic count (use when spawn count varies at runtime)**
Leave `ExpectedEnemyCount = 0` and call `Notify Enemy Spawned` from the chunk's
`On Chunk Activated` event (or whenever an enemy is created).
### How enemies are counted OUT
From inside a BP chunk actor (or directly from an enemy's death event):
```
On Death (enemy BP)
    └─► Get Owning Chunk Reference
            └─► Notify Enemy Killed (Count = 1)
```
Or if you don't have a chunk reference:
```
On Death (enemy BP)
    └─► Get Chase Track Manager (e.g. via GameplayMessageSubsystem or a stored reference)
            └─► Register Enemy Killed (Count = 1)
```
### Completion trigger
When `TotalKilled >= TotalTracked` AND `TotalTracked > 0`:
1. `K2_OnAllEnemiesDefeated` fires on the manager.
2. State transitions to `Finishing`.
3. `EndChunkDefinition` is spawned (if set).
4. Normal chunk spawning stops.
5. Once the player passes the end chunk (all active chunks are recycled), `K2_OnChaseComplete` fires.
> If you never want enemy-based completion, leave `ExpectedEnemyCount = 0` on all definitions and call `ForceFinish()` from your own trigger (timer, volume, cinematic, etc.).
---
## Setup Walkthrough
### Step 1 — Create Chunk Actors
1. In the Content Browser, create a **Blueprint Class** based on `AMYSTRoadChunkActor`  
   e.g. `B_Chunk_Straight`, `B_Chunk_Ambush`.
2. In the Class Defaults, add your road mesh as a Static Mesh Component; orient it along +X.
3. Implement `On Chunk Activated`:
   - Spawn obstacle actors, configure AI spawners, etc.
   - Call `Notify Enemy Spawned` if you spawn enemies dynamically.
4. Implement `On Chunk Deactivated` if you have anything to clean up.
5. In enemy death events inside the chunk: call `Notify Enemy Killed`.
### Step 2 — Create Chunk Definitions
For each chunk Blueprint, create a **Data Asset** (`UMYSTChunkDefinition`):
| Asset | `ChunkClass` | `ChunkLength` | `SpawnWeight` | `ExpectedEnemyCount` |
|---|---|---|---|---|
| `DA_Chunk_Straight` | `B_Chunk_Straight` | 5000 | 2 | 0 |
| `DA_Chunk_Ambush` | `B_Chunk_Ambush` | 6000 | 1 | 4 |
| `DA_Chunk_End` | `B_Chunk_End` | 8000 | — | 0 |  ← `bIsEndChunk = true` |
### Step 3 — Set Up the Manager
1. Create a **Blueprint Class** based on `AMYSTChaseTrackManager` → `B_ChaseTrackManager`.
2. Drop it into your map, **rotate it to face the travel direction**.
3. In the **Details panel**:
   - Add your DA assets to `ChunkPool`.
   - Assign `DA_Chunk_End` to `EndChunkDefinition`.
   - Leave `TrackedPawn` null (auto-grabs player) or point it at your vehicle actor.
   - Set `bAutoStartOnBeginPlay = true`.
4. Override `On Chase Started` to begin vehicle movement / music.
5. Override `On All Enemies Defeated` to play a stinger and update the HUD.
6. Override `On Chase Complete` to load the next map or show the victory screen.
### Step 4 — HUD / Progress Bar (optional)
Bind to the manager's query functions each tick (or via a timer):
```
Get Chase Track Manager reference
    ├─► Get Remaining Enemies   ──► drive enemy-count widget
    ├─► Get Travel Distance     ──► drive a "distance traveled" bar
    └─► Get Chase State         ──► show/hide "FINISH" overlay
```
---
## Tips & Edge Cases
| Situation | Solution |
|---|---|
| Road visually gaps between chunks | Make sure `ChunkLength` matches the physical mesh length exactly. Use `Get Chunk Length Override` in Blueprint if the mesh is not a round number. |
| Track curves or goes uphill | Override `Get Chunk Length Override` and handle positioning manually; or pre-rotate individual chunk meshes — the manager always spawns at +X of the previous exit. |
| Player can fall off the road | Add an overlap volume to the chunk that pushes or teleports the pawn back to the center lane. |
| Need a guaranteed first chunk | Add it as the first entry in `ChunkPool` with a very high `SpawnWeight` and remove it from the pool after the first spawn via `PickNextChunkDefinition` override. |
| Want ordered chunks instead of random | Subclass `AMYSTChaseTrackManager` in Blueprint, override `PickNextChunkDefinition` (it is `BlueprintCallable`+`BlueprintProtected`), and return chunks from a sequenced array. |
| Very long chase (>50 km) | Enable **Large World Coordinates** (already on in UE5). No changes needed to this system. |
| Multiplayer | `AMYSTChaseTrackManager` does not replicate chunks. For multiplayer, spawn chunks on the server and replicate their positions, or use a time-based travel distance shared via `GameState`. |
---
## File Reference
| File | Location |
|---|---|
| `MYSTChunkDefinition.h/.cpp` | `Public/CarChase/` · `Private/CarChase/` |
| `AMYSTRoadChunkActor.h/.cpp` | `Public/CarChase/` · `Private/CarChase/` |
| `AMYSTChaseTrackManager.h/.cpp` | `Public/CarChase/` · `Private/CarChase/` |
All files live inside `Plugins/GameFeatures/MyShooterFeaturePlugin/Source/MyShooterFeaturePluginRuntime/`.
No new module dependencies are needed — the `Engine` module (already in `PublicDependencyModuleNames`) covers all types used.
