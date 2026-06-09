# AI Cover Tasks — Quick Start Guide

## What Was Implemented

Four new C++ BT tasks that integrate AI characters with the CMC-based cover system:

| Task | Purpose |
|---|---|
| **`UBTTask_EnterCover`** | Atomic cover entry: claims spot, moves AI, traces wall, calls `CMC->EnterCoverMode()` |
| **`UBTTask_ExitCover`** | Exits cover mode, retreats from wall, releases claim |
| **`UBTTask_FindPeekLocation`** | Samples strafe positions from cover, finds clear LOS to target |
| **`UBTTask_PeekFromCover`** | Integrated peek: strafes along cover to peek location, activates ADS ability, holds position |

---

## Key Benefits Over MoveTo Approach

### Old Approach (MoveTo with tight margin)
❌ No guarantee AI faces wall correctly  
❌ Race condition between MoveTo completion and overlap trigger  
❌ Separate claim/release tasks → leak risk on abort  
❌ Manual rotation management  
❌ No built-in exit logic  

### New Approach (C++ Tasks)
✅ **Atomic operations** — claim + move + glue in one task  
✅ **Guaranteed correct orientation** — `EnterCoverMode` enforces wall-facing rotation  
✅ **Safe abort cleanup** — `AbortTask()` releases claim and exits cover automatically  
✅ **CMC integration** — uses the cover movement mode gluing (no drift)  
✅ **Integrated peek cycle** — strafe + ADS + hold in a single task  
✅ **Reuses existing systems** — works with `UMYSTCoverClaimSubsystem`, works with peek willingness scoring  

---

## Behavior Tree Structure

```
Selector [Combat Root]
  ├─ [Services: PlayerPerception, AIStateObserver, PeekWillingness]
  │
  ├─ Sequence [PEEK FROM COVER]                      ← HIGH priority
  │   ├─ Decorator: IsInCoverMode == true
  │   ├─ Decorator: PeekWillingnessScore >= 0.35
  │   ├─ BTTask_FindPeekLocation
  │   └─ BTTask_PeekFromCover
  │
  ├─ Sequence [ENTER COVER]                          ← MID priority
  │   ├─ Decorator: IsInCoverMode == false
  │   ├─ Decorator: HasSeenPlayer == true
  │   ├─ RunEQSQuery → CoverLocation
  │   └─ BTTask_EnterCover
  │
  └─ Sequence [PATROL / IDLE]                        ← LOW priority
```

---

## Setup Steps

### 0. Tag Your Cover Meshes (CRITICAL!)

**Before setting up the BT**, you must tag all cover meshes in your level:

1. Select your cover mesh in the level
2. In the Details panel, scroll to **Tags**
3. Add an **Actor Tag** or **Component Tag** named `Cover`

**Why this is required:** The AI wall trace validates that it's hitting actual cover (not the ground, player, or other objects) by checking for this tag. Without it, AIs will fail to enter cover or may try to "cover" against inappropriate surfaces.

**Alternative tag name:** If you want to use a different tag (e.g., `AICover`), set the `RequiredCoverTag` property on the `BTTask_EnterCover` node in your Behavior Tree.

### 1. Add New Blackboard Keys to `BB_ShooterAI`

| Key Name | Type |
|---|---|
| `IsInCoverMode` | Bool |
| `PeekLocation` | Vector |

*(If not already present)*
| `CoverLocation` | Vector |
| `TargetEnemy` | Object |
| `HasSeenPlayer` | Bool |
| `PeekWillingnessScore` | Float |

### 2. Add BTService_CoverModeSync to the BT Root/Selector

Add **`BTService_CoverModeSync`** to the same Selector (or root composite) that holds your ENTER/PEEK/EXIT sequences.

- **IsInCoverModeKey:** `IsInCoverMode`

This service polls `ULyraCharacterMovementComponent::IsInCoverMode()` on every tick (~0.1 s interval) and keeps the BB key in sync automatically. It is the **authoritative source** for `IsInCoverMode` — do **not** wire that key on the tasks themselves.

### 3. Set Up ENTER COVER Sequence

1. Add `RunEQSQuery` task
   - Query Asset: `EQS_FindCover`
   - Result Key: `CoverLocation`

2. Add `BTTask_EnterCover`
   - **CoverLocationKey:** `CoverLocation`
   - **CoverMaxSpeed:** `250`
   - **ClaimRadius:** `150` (default)
   - *(No IsInCoverModeKey — managed by BTService_CoverModeSync)*

3. Add decorators:
   - `IsInCoverMode == false` (inverse check, AbortBoth)
   - `HasSeenPlayer == true` (AbortBoth)
   - `TargetEnemy != null` (optional, AbortBoth)

### 4. Set Up PEEK FROM COVER Sequence

1. Add `BTTask_FindPeekLocation`
   - **CoverLocationKey:** `CoverLocation`
   - **TargetActorKey:** `TargetEnemy`
   - **PeekLocationKey:** `PeekLocation`
   - **StepSize:** `80` (tune for cover width)
   - **MaxSteps:** `3`

2. Add `BTTask_PeekFromCover`
   - **PeekLocationKey:** `PeekLocation`
   - **TargetActorKey:** `TargetEnemy`
   - **ADSAbilityTag:** `Ability.Type.Action.ADS`
   - **ADSHoldDuration:** `2.0` seconds
   - **StrafeSpeedScale:** `0.8`

3. Add decorators:
   - `IsInCoverMode == true` (AbortBoth)
   - `PeekWillingnessScore >= 0.35` (AbortBoth)
   - `OutOfAmmo == false` (AbortSelf)
   - `HasTakenDamageRecently == false` (AbortSelf)

### 5. Optional: Set Up EXIT COVER Sequence

Add a low-priority sequence with exit conditions (e.g., no targets for 10s):

1. Add `BTTask_ExitCover`
   - **RetreatDistance:** `150` cm
   - **AcceptanceRadius:** `50`
   - *(No IsInCoverModeKey — managed by BTService_CoverModeSync)*

2. Add decorators for your exit trigger conditions

---

## How It Works

### Entry Flow
1. AI sees player → **ENTER COVER** branch activates
2. `RunEQSQuery` finds the nearest unclaimed cover spot
3. `BTTask_EnterCover`:
   - Claims the spot via `UMYSTCoverClaimSubsystem`
   - Moves AI to the location
   - Traces toward wall to get `FHitResult`
   - Calls `CMC->EnterCoverMode(WallHit, MaxSpeed)` then completes
4. `BTService_CoverModeSync` detects the CMC state change and sets `IsInCoverMode = true` in BB
5. AI is now glued to cover, back facing wall

### Peek Flow
1. `IsInCoverMode == true` + `PeekWillingnessScore >= 0.35` → **PEEK FROM COVER** activates
2. `BTTask_FindPeekLocation`:
   - Samples strafe positions left/right from cover
   - Traces LOS to target
   - Writes first valid position to `PeekLocation`
3. `BTTask_PeekFromCover`:
   - Drives lateral movement via `AddMovementInput` along `CoverSurfaceTangent`
   - When within 20 cm of peek location → activates ADS ability
   - Waits for `Event.Movement.ADS` tag (confirms ADS started)
   - Holds position for `ADSHoldDuration` or until ADS tag removed
   - Returns Success
4. Sequence repeats (if score still high) or AI stays in neutral cover

### Return to Cover (Auto)
- If `OutOfAmmo` or `HasTakenDamageRecently` flips `true` mid-peek:
  - Peek sequence **aborts** (AbortSelf decorators)
  - AI stops peeking but **stays in cover** (`IsInCoverMode` still true)
  - Once reload finishes and cooldown expires → peek can re-activate

### Exit Flow (Optional)
- When explicit exit conditions met → **EXIT COVER** branch runs
- `BTTask_ExitCover`:
  - Reads `CMC->CoverSurfaceNormal` before exiting
  - Calls `CMC->ExitCoverMode()` → restores MOVE_Walking
  - Computes retreat point: `CurrentLocation + Normal * RetreatDistance`
  - Moves AI away from wall
  - Releases cover claim
- `BTService_CoverModeSync` detects the CMC state change and clears `IsInCoverMode` automatically

---

## Tuning Guide

### Cover Entry

| Parameter | Default | When to Change |
|---|---|---|
| `CoverMaxSpeed` | 250 cm/s | Increase for faster lateral movement along cover; decrease for cautious AIs |
| `ClaimRadius` | 150 cm | Increase if AIs are packing too tightly; decrease for tighter formations |
| `AcceptanceRadius` | 50 cm | Increase if AIs are stopping too far from cover; decrease for precision |
| `RequiredCoverTag` | "Cover" | Change if you want to use a different tag name for your cover meshes |

### Peek Search

| Parameter | Default | When to Change |
|---|---|---|
| `StepSize` | 80 cm | Match to cover geometry width; smaller = more precise, more traces |
| `MaxSteps` | 3 | Increase for wider cover surfaces (allows farther strafe); reduce for narrow walls |
| `bRequireNavMesh` | true | Disable if cover is off-navmesh (stairs, ledges) but reachable |

### Peek Behavior

| Parameter | Default | When to Change |
|---|---|---|
| `ADSHoldDuration` | 2.0 s | Increase for longer aim windows; decrease for quick pop-shots |
| `StrafeSpeedScale` | 0.8 | Reduce for slow, cautious peeks; increase for aggressive behavior |
| `PeekArrivalThreshold` | 20 cm | Increase if AI is activating ADS too early while still moving |

### Exit Behavior

| Parameter | Default | When to Change |
|---|---|---|
| `RetreatDistance` | 150 cm | Increase to back away farther from cover; decrease for minimal retreat |

---

## Troubleshooting

### AI walks close to cover but just stands there / IsInCoverMode keeps flickering
The `IsInCoverMode` BB key is now driven exclusively by **`BTService_CoverModeSync`**, which polls `CMC::IsInCoverMode()` on a ~0.1 s interval. The tasks no longer set this key directly, eliminating the stabilization-delay workaround.

**If still flickering:**
- Verify `BTService_CoverModeSync` is attached to the correct Selector/composite node (it must be active while the ENTER/PEEK branches run)
- Check your BT decorators — make sure PEEK sequence has `IsInCoverMode == true` decorator with `AbortBoth` (not `AbortLowerPriority`)
- Ensure ENTER COVER sequence has `IsInCoverMode == false` decorator to prevent re-running while already in cover

### AI gets stuck in awkward position / enters cover on ground
- **Missing cover tags!** This is the most common issue — add the `Cover` tag to your cover mesh's Actor Tags or Component Tags
- The wall trace is hitting the ground or another non-cover surface
- Increase `WallTraceDistance` if the cover mesh is far from the EQS query result point
- Check your EQS query is actually finding cover meshes (not random floor points)

### AI enters cover but immediately exits
- **Check cover mesh tags** — ensure your cover meshes have the `Cover` tag (Actor or Component Tags)
- Verify `BTService_CoverModeSync` is attached and running (it is the sole writer of `IsInCoverMode`)
- Verify no conflicting decorators forcing exit
- Check the wall trace succeeds (the AI must be close enough to the cover mesh for the trace to hit it)

### AI never peeks
- Verify `PeekWillingnessScore` is being written and is >= 0.35
- Check `BTTask_FindPeekLocation` is succeeding (enable visual logging on EQS)
- Confirm ADS ability exists and has `Ability.Type.Action.ADS` asset tag

### AI gets stuck in peek position
- Reduce `ADSHoldDuration`
- Check ADS ability properly removes `Event.Movement.ADS` tag on end
- Verify abort decorators are set to `AbortSelf` (not `None`)

### Multiple AIs claim the same cover
- Verify `UMYSTEnvQueryTest_ClaimedSpot` is added to `EQS_FindCover`
- Check `ClaimRadius` is large enough to encompass the cover geometry
- Confirm `BTTask_EnterCover` is the one claiming (not old BP tasks)

### AI rotation is wrong when entering cover
- The wall trace in `BTTask_EnterCover` must hit the actual cover surface
- Increase `WallTraceDistance` if cover is far from the EQS result point
- Verify `CMC->EnterCoverMode()` is being called (check logs)

---

## Integration with Existing Systems

### Works With
✅ `UMYSTCoverClaimSubsystem` — claim/release handled automatically  
✅ `BTService_PeekWillingness` — peek activation is score-driven  
✅ `BTService_PlayerPerception` — drive cover entry on sight  
✅ `BTService_AIStateObserver` — auto-abort peek on reload/damage  
✅ CMC cover movement mode + rotation lock — no drift, perfect wall-facing  
✅ ADS/lean rotation exception — AI can aim freely during peek  

### Does Not Require
❌ `BP_AutoCoverComponent` on AI — this is player-only  
❌ Manual rotation management — `EnterCoverMode` handles it  
❌ Custom movement components — `AddMovementInput` → CMC `PhysCustom` handles gluing  

---

## Next Steps

1. **Compile the project** to generate the new BT task nodes
2. **Open your AI's Behavior Tree** and add the sequences as described above
3. **Test with one AI** first — enable Visual Logging (`ShowDebug BEHAVIR`) to watch state transitions
4. **Tune parameters** based on your cover geometry and desired AI aggression
5. **Check the updated `AICoverPeek_ImplementationGuide.md`** for full integration with peek willingness scoring and other services

---

## File Locations

**Headers:**
- `Public/AI/BTService_CoverModeSync.h`
- `Public/AI/BTTask_FindPeekLocation.h`
- `Public/AI/BTTask_EnterCover.h`
- `Public/AI/BTTask_ExitCover.h`
- `Public/AI/BTTask_PeekFromCover.h`

**Implementation:**
- `Private/AI/BTService_CoverModeSync.cpp`
- `Private/AI/BTTask_FindPeekLocation.cpp`
- `Private/AI/BTTask_EnterCover.cpp`
- `Private/AI/BTTask_ExitCover.cpp`
- `Private/AI/BTTask_PeekFromCover.cpp`

**Module:** `MyShooterFeaturePluginRuntime` (already has `AIModule` and `NavigationSystem` dependencies)

---

## Migration from Old BP Tasks

If you have existing BT graphs using `BTT_ClaimCoverSpot` / `BTT_ReleaseCoverSpot`:

| Old Pattern | New Pattern |
|---|---|
| `RunEQSQuery` + `BTT_ClaimCoverSpot` + `MoveTo` | `RunEQSQuery` + `BTTask_EnterCover` |
| `BTS_CoverClaimLifetime` service | *(Remove — claim handled in task)* |
| `BTT_ReleaseCoverSpot` | `BTTask_ExitCover` *(or automatic on abort)* |
| `BTTask_FindPeekLocation` + `MoveTo` + `BTTask_TryUseAbility` | `BTTask_FindPeekLocation` + `BTTask_PeekFromCover` |

The new tasks handle abort cleanup automatically, so you **do not** need separate release logic.

