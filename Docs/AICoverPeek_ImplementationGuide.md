# MYST AI Cover & Peek System — Implementation Guide

> **Covers:** Cover claiming · EQS filtering · Peek location search · State observation · Peek willingness scoring  
> **Classes implemented:** `UMYSTCoverClaimSubsystem` · `UMYSTEnvQueryTest_ClaimedSpot` · `UBTTask_FindPeekLocation` · `UBTService_AIStateObserver` · `UBTService_PeekWillingness`

---

## System Overview

The system gives each AI three behaviours — **find cover**, **stay in cover**, and **peek out to shoot** — driven entirely by reactive BT Decorators and two aggregating BTServices. There are no random timers; every state transition is triggered by a GAS tag change, a health delta, or a scored threshold crossing.

```
┌──────────────────────────────────────────────────────────────────────┐
│  EQS_FindCover                                                       │
│    UMYSTEnvQueryTest_ClaimedSpot  ─── filters occupied spots ──────► │
│                                                                      │
│  UMYSTCoverClaimSubsystem  (WorldSubsystem)                          │
│    ClaimSpot / ReleaseSpot / IsSpotClaimed                           │
└──────────────────┬───────────────────────────────────────────────────┘
                   │ claimed location
                   ▼
┌──────────────────────────────────────────────────────────────────────┐
│  Behavior Tree  [Combat Selector Root]                               │
│                                                                      │
│  [Service] BTService_UpdateCombatState  ──► HealthPct, IsIsolated,   │
│                                             HasTargetInCover,        │
│                                             TargetTimeInCover,       │
│                                             IsTargetEngagingOther    │
│                                                                      │
│  [Service] BTService_AIStateObserver   ──►  OutOfAmmo,               │
│                                             HasTakenDamageRecently,  │
│                                             TargetIsReloading,       │
│                                             TargetIsLowHealth        │
│                                                                      │
│  [Service] BTService_PeekWillingness   ──►  PeekWillingnessScore     │
│                                                                      │
│  ┌─ Sequence [PEEK & SHOOT]  ◄── Decorator: Score >= 0.35           │
│  │    BTTask_FindPeekLocation                                        │
│  │    MoveTo(PeekLocation)                                           │
│  │    BTTask_TryUseAbility [Fire]                                    │
│  │                                                                   │
│  └─ Sequence [TAKE COVER]  (fallback)                                │
│       RunEQSQuery → CoverLocation                                    │
│       BTT_ClaimCoverSpot (BP)                                        │
│       MoveTo(CoverLocation)                                          │
└──────────────────────────────────────────────────────────────────────┘
```

---

## Blackboard Keys Reference (`BB_ShooterAI`)

### Actors

| Key | Type | Writer |
|---|---|---|
| `SelfActor` | Object | AI Controller (built-in) |
| `TargetEnemy` | Object | Perception / custom setter |

### Location

| Key | Type | Writer |
|---|---|---|
| `MoveGoal` | Vector | MoveTo tasks |
| `LastKnownLocation` | Vector | Perception |
| `LookAtLocation` | Vector | Custom |
| `CoverLocation` | Vector | `BTT_ClaimCoverSpot` (BP) |
| `PeekLocation` | Vector | `UBTTask_FindPeekLocation` |

### State *(all Boolean unless noted)*

| Key | Type | Writer | Condition |
|---|---|---|---|
| `OutOfAmmo` | Bool | `BTService_AIStateObserver` | AI ASC has `Event.Movement.Reload` tag |
| `HasSeenPlayer` | Bool | Perception | — |
| `HasEngagedPlayer` | Bool | Custom | — |
| `HasTakenDamageRecently` | Bool | `BTService_AIStateObserver` | Health dropped ≥ threshold; auto-clears after `DamageCooldown` s |
| `TargetIsReloading` | Bool | `BTService_AIStateObserver` | Target ASC has `Event.Movement.Reload` tag |
| `TargetIsLowHealth` | Bool | `BTService_AIStateObserver` | Target health < `LowHealthThreshold` |
| `HasTargetInCover` | Bool | `BTService_UpdateCombatState` | Target ASC has `State.Cover` tag |
| `IsIsolated` | Bool | `BTService_UpdateCombatState` | No ally within `IsolationRadius` |
| `IsTargetEngagingOther` | Bool | `BTService_UpdateCombatState` | Target facing an ally (dot product heuristic) |

### Float

| Key | Type | Writer |
|---|---|---|
| `HealthPercentage` | Float | `BTService_UpdateCombatState` |
| `TargetTimeInCover` | Float | `BTService_UpdateCombatState` |
| `PeekWillingnessScore` | Float | `BTService_PeekWillingness` |

### Timer / Sync

| Key | Type | Writer |
|---|---|---|
| `TimeSinceLastSpotted` | Float | Custom |
| `HasSpottingBeenReported` | Bool | Custom |

> **To add the 3 new state keys and 2 new float keys:** open `BB_ShooterAI` in the BB editor, click **New Key**, and add `TargetIsReloading` (Bool), `TargetIsLowHealth` (Bool), and `PeekWillingnessScore` (Float).

---

## C++ Class Inventory

| Class | Module | Location | Role |
|---|---|---|---|
| `UMYSTCoverClaimSubsystem` | `MyShooterFeaturePluginRuntime` | `Public/AI/` | WorldSubsystem tracking which cover spots are claimed |
| `UMYSTEnvQueryTest_ClaimedSpot` | `MyShooterFeaturePluginRuntime` | `Public/AI/` | EQS test: filters points claimed by other AIs |
| `UBTTask_FindPeekLocation` | `MyShooterFeaturePluginRuntime` | `Public/AI/` | Samples strafe positions from cover, NavProjects, LOS-traces, writes `PeekLocation` |
| `UBTService_AIStateObserver` | `MyShooterFeaturePluginRuntime` | `Public/AI/` | Polls GAS tags and health; writes `OutOfAmmo`, `HasTakenDamageRecently`, `TargetIsReloading`, `TargetIsLowHealth` |
| `UBTService_PeekWillingness` | `MyShooterFeaturePluginRuntime` | `Public/AI/` | Aggregates all state keys into `PeekWillingnessScore` [0–1] |

Blueprint-only (no C++ replacement needed):

| Asset | Role |
|---|---|
| `BTT_ClaimCoverSpot` (BP) | Calls `UMYSTCoverClaimSubsystem::ClaimSpot` |
| `BTT_ReleaseCoverSpot` (BP) | Calls `UMYSTCoverClaimSubsystem::ReleaseSpot` |

---

## `UMYSTCoverClaimSubsystem`

A `UWorldSubsystem` that owns a `TMap<TWeakObjectPtr<AActor>, FVector>` of claimed spots. Stale entries (destroyed actors) are purged each `ClaimSpot` / `IsSpotClaimed` call.

```
ClaimSpot(Location, Claimer, Radius=150)
  → true  if no other live actor's claim is within Radius of Location
  → false if spot is occupied (Claimer's previous claim unchanged)

ReleaseSpot(Claimer)
  → removes Claimer's entry; safe even if Claimer has no active claim

IsSpotClaimed(Location, Radius, Ignore=nullptr) → bool
GetClaimant(Location, Radius)                   → AActor*
DrawDebugClaims(Duration, SphereRadius)          → debug draw (dev only)
```

**Typical BT flow:**
1. Run `EQS_FindCover` → `UMYSTEnvQueryTest_ClaimedSpot` filters occupied points.
2. `BTT_ClaimCoverSpot` → `ClaimSpot(CoverLocation, Self)`.
3. `MoveTo(CoverLocation)`.
4. On sequence abort / AI death → `BTT_ReleaseCoverSpot` → `ReleaseSpot(Self)`.

> ⚠️ **Important:** `BTT_ReleaseCoverSpot` must fire on **sequence abort**, not only on successful completion. Verify this is wired via an **Abort Decorator** or the task's **Abort** pin in the BP Graph — otherwise destroyed/dead AIs will leave ghost claims.

---

## `UBTTask_FindPeekLocation`

Runs synchronously and returns `Success` or `Failure`.

### Algorithm

```
1. StrafeAxis = CrossProduct(normalize2D(CoverLocation → TargetLocation), UpVector)
2. For Step in [1 … MaxSteps], for Sign in [+1, −1]:
     Candidate = CoverLocation + StrafeAxis * (StepSize * Step * Sign)
     [Optional] Project Candidate onto NavMesh (bRequireNavMesh)
     LineTrace (eye height) from Candidate to Target
     if ClearLOS → write PeekLocation, return Success
3. return Failure (AI stays in cover this tick; BT retries next frame)
```

### Key properties

| Property | Default | Notes |
|---|---|---|
| `CoverLocationKey` | — | Vector BB key set by `BTT_ClaimCoverSpot` |
| `TargetActorKey` | — | Object BB key (`TargetEnemy`) |
| `PeekLocationKey` | — | Output Vector BB key |
| `StepSize` | 80 cm | Tune to cover geometry width |
| `MaxSteps` | 3 | Total candidates = 2 × MaxSteps |
| `EyeHeightOffset` | 160 cm | Match character eye height |
| `TraceChannel` | `ECC_Visibility` | — |
| `bRequireNavMesh` | `true` | Skip un-navigable positions |
| `NavProjectionExtentZ` | 100 cm | Vertical search range for NavMesh projection |

---

## `UBTService_AIStateObserver`

Ticks at `Interval = 0.15s` (± `RandomDeviation = 0.05s`). Uses **per-node instance memory** (`FAIStateObserverMemory`) — no delegates or extra components.

### Key writes

```
OutOfAmmo              ← AI ASC HasMatchingGameplayTag(ReloadTag)
HasTakenDamageRecently ← health delta >= DamageReactionThreshold → armed for DamageCooldown s
TargetIsReloading      ← Target ASC HasMatchingGameplayTag(ReloadTag)
TargetIsLowHealth      ← Target GetHealthNormalized() < LowHealthThreshold
```

### Key properties

| Property | Default | Notes |
|---|---|---|
| `ReloadTag` | `Event.Movement.Reload` | Pre-resolved at construction. Must match `ActivationOwnedTags` on the reload GA |
| `DamageReactionThreshold` | `0.04` (4 % MaxHP) | Filters DoT micro-pulses |
| `DamageCooldown` | `4.0 s` | How long `HasTakenDamageRecently` stays `true` after a qualifying hit |
| `LowHealthThreshold` | `0.35` (35 %) | Target health gate for `TargetIsLowHealth` |

### Blueprint hooks (implement in BP subclasses)

| Event | When it fires |
|---|---|
| `OnAIReloadStateChanged(bool bIsNowReloading)` | Every reload state transition (start & end) |
| `OnDamageDetected(float HealthLostFraction)` | The tick a qualifying damage hit is detected |

> **Player ASC resolution note:** `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor` resolves the Lyra player's ASC from `ALyraPlayerState` automatically via `IAbilitySystemInterface`. You only need to pass the pawn (the value stored in `TargetEnemy`).

---

## `UBTService_PeekWillingness`

Ticks at `Interval = 0.20s` as a pure aggregator — reads keys from both upstream services and writes `PeekWillingnessScore` [0, 1].

### Score formula

```
Score = 0.0

── Offensive bonuses (player is vulnerable) ──────────────────────────
if TargetIsReloading:           Score += TargetIsReloadingBonus   (0.40)
if !HasTargetInCover:           Score += TargetExposedBonus       (0.25)
if TargetIsLowHealth:           Score += TargetLowHealthBonus     (0.20)
if IsTargetEngagingOther:       Score += TargetDistractedBonus    (0.15)
if TargetTimeInCover >= 5s:     Score += TargetOverduePeekBonus   (0.10)

── Defensive penalties (AI is unsafe) ────────────────────────────────
if OutOfAmmo:                   Score -= OutOfAmmoPenalty         (0.60)
if HasTakenDamageRecently:      Score -= RecentlyHitPenalty       (0.40)
if HealthPercentage < 0.30:     Score -= LowSelfHealthPenalty     (0.30)
if IsIsolated:                  Score -= IsolatedPenalty          (0.20)

Score = Clamp(Score, 0.0, 1.0)
```

When `Score >= PeekThreshold` (default `0.35`) the Peek Sequence is unblocked. The BT Decorator (with `AbortSelf+LowerPriority`) makes this fully reactive — no extra C++ event-handling needed.

### Key properties

| Property | Default |
|---|---|
| `PeekThreshold` | `0.35` |
| `TargetIsReloadingBonus` | `0.40` |
| `TargetExposedBonus` | `0.25` |
| `TargetLowHealthBonus` | `0.20` |
| `TargetDistractedBonus` | `0.15` |
| `TargetOverduePeekBonus` | `0.10` |
| `TargetOverduePeekTime` | `5.0 s` |
| `OutOfAmmoPenalty` | `0.60` |
| `RecentlyHitPenalty` | `0.40` |
| `LowSelfHealthPenalty` | `0.30` |
| `LowSelfHealthThreshold` | `0.30` |
| `IsolatedPenalty` | `0.20` |

### Blueprint hook

| Event | When it fires |
|---|---|
| `OnPeekReadinessChanged(bool bIsNowReady)` | Every time score crosses `PeekThreshold` in either direction |

---

## Behavior Tree Layout

Attach all three services to the **Combat Selector root** so they tick regardless of which branch is active.

```
Root
└── Selector  [Combat Root]
    ├── [Service] BTService_UpdateCombatState        ← tick first
    ├── [Service] BTService_AIStateObserver          ← tick second
    ├── [Service] BTService_PeekWillingness          ← tick last (reads outputs of both)
    │
    ├── Sequence  [PEEK & SHOOT]                     HIGH priority
    │   ├── Decorator: BB  TargetEnemy != null            AbortSelf+LowerPriority
    │   ├── Decorator: BB  PeekWillingnessScore >= 0.35   AbortSelf+LowerPriority ◄ reactive gate
    │   ├── Decorator: BB  OutOfAmmo == false              AbortSelf
    │   ├── Decorator: BB  HasTakenDamageRecently == false AbortSelf
    │   ├── BTTask_FindPeekLocation
    │   │     CoverLocationKey → CoverLocation
    │   │     TargetActorKey   → TargetEnemy
    │   │     PeekLocationKey  → PeekLocation
    │   ├── MoveTo (PeekLocation)
    │   └── BTTask_TryUseAbility  [Fire ability tag]
    │
    └── Sequence  [TAKE COVER]                       LOW priority (fallback)
        ├── RunEQSQuery → CoverLocation
        │     (EQS_FindCover + UMYSTEnvQueryTest_ClaimedSpot)
        ├── BTT_ClaimCoverSpot  (BP)
        ├── MoveTo (CoverLocation)
        └── Wait (0.1 s)        ← prevents EQS from hammering every frame
```

### How return-to-cover works (no extra C++ needed)

When `OutOfAmmo` or `HasTakenDamageRecently` flips `true` mid-peek, the `AbortSelf` Decorators on the Peek Sequence fire immediately. The Selector falls through to the **TAKE COVER** branch, which runs `MoveTo(CoverLocation)`. Once the reload finishes and the damage cooldown expires, the Decorators re-allow the Peek Sequence and the cycle repeats.

---

## Editor Setup Checklist

### 1. Add missing BB keys to `BB_ShooterAI`

Open `BB_ShooterAI` → **New Key** for each:

| Key Name | Key Type |
|---|---|
| `TargetIsReloading` | Bool |
| `TargetIsLowHealth` | Bool |
| `PeekWillingnessScore` | Float |

### 2. Configure `BTService_AIStateObserver` on the Combat Selector

Wire each selector to the matching BB key name:

| Selector field | BB key to point at |
|---|---|
| `OutOfAmmoKey` | `OutOfAmmo` |
| `HasTakenDamageRecentlyKey` | `HasTakenDamageRecently` |
| `TargetEnemyKey` | `TargetEnemy` |
| `TargetIsReloadingKey` | `TargetIsReloading` |
| `TargetIsLowHealthKey` | `TargetIsLowHealth` |

### 3. Configure `BTService_PeekWillingness` on the Combat Selector

| Selector field | BB key to point at |
|---|---|
| `HealthPercentageKey` | same key as `BTService_UpdateCombatState` |
| `IsIsolatedKey` | same key |
| `HasTargetInCoverKey` | same key |
| `TargetTimeInCoverKey` | same key |
| `IsTargetEngagingOtherKey` | same key |
| `OutOfAmmoKey` | `OutOfAmmo` |
| `HasTakenDamageRecentlyKey` | `HasTakenDamageRecently` |
| `TargetIsReloadingKey` | `TargetIsReloading` |
| `TargetIsLowHealthKey` | `TargetIsLowHealth` |
| `PeekWillingnessScoreKey` | `PeekWillingnessScore` |

### 4. Add Decorators to the Peek Sequence

| Decorator | Key | Condition | Observer Aborts |
|---|---|---|---|
| Blackboard | `TargetEnemy` | Is Set | Both |
| Blackboard | `PeekWillingnessScore` | Float >= `0.35` | Both |
| Blackboard | `OutOfAmmo` | Is Not Set / == false | Self |
| Blackboard | `HasTakenDamageRecently` | Is Not Set / == false | Self |

### 5. Verify `BTT_ReleaseCoverSpot` fires on abort

In the BT editor, confirm the BP task fires when the **TAKE COVER** Sequence is interrupted — not only when it succeeds. The recommended pattern is an **On Abort** service or a **Decorator On Abort** that calls `ReleaseSpot(SelfActor)`. Without this, the `UMYSTCoverClaimSubsystem` will accumulate ghost claims for dead or fleeing AIs.

---

## Tuning Guide

### Quick archetype presets

| Archetype | `PeekThreshold` | Key weight changes |
|---|---|---|
| **Aggressive** | `0.20` | Halve `RecentlyHitPenalty` and `LowSelfHealthPenalty` |
| **Cowardly** | `0.55` | Double `RecentlyHitPenalty`; add +0.20 to `OutOfAmmoPenalty` |
| **Flanker** | `0.25` | Increase `TargetDistractedBonus` to `0.35` |
| **Sniper** | `0.40` | `MaxSteps = 1`, `StepSize = 200 cm` in `BTTask_FindPeekLocation` |

To create a per-archetype variant: **subclass `BTService_PeekWillingness` in Blueprint**, override the weight defaults in Class Defaults. No C++ recompile needed.

### Tick interval vs. responsiveness

| Service | Default interval | Reduce to… | Effect |
|---|---|---|---|
| `BTService_AIStateObserver` | 0.15 s | 0.08 s | `HasTakenDamageRecently` reacts < 1 frame faster |
| `BTService_PeekWillingness` | 0.20 s | 0.10 s | Score updates snap to state changes more tightly |

Higher intervals are cheaper on CPU-heavy encounters (many AIs); increase them for background/distant enemies.

---

## Debugging

### Visualize cover claims

Call `UMYSTCoverClaimSubsystem::DrawDebugClaims(Duration, SphereRadius)` from any Blueprint or from the console:

```
# In PIE console:
ke * DrawDebugClaims
```

Each claimed spot renders as an **orange sphere** with a **yellow line** to the owning AI and a name label.

### Blackboard monitor

In PIE, open **Window → Behavior Tree Debugger** and select the AI pawn. Watch `PeekWillingnessScore` update in real time as you (the player) reload, step out of cover, or damage the AI.

### Log tags

Add `LogBehaviorTree Verbose` to `DefaultEngine.ini` if decorators are not aborting as expected:

```ini
[Core.Log]
LogBehaviorTree=Verbose
```

### Check GAS tags are being granted

In PIE, with the player selected: **Gameplay Debugger** → `AbilitySystem` category (`'` key by default). Confirm `Event.Movement.Reload` appears in **Active Tags** while the player is reloading.

---

## Extending the System

### Add a new scoring signal

1. Write the new BB key from either `BTService_AIStateObserver` (if it needs GAS/health access) or directly from any other BT node.
2. Add a `FBlackboardKeySelector` property and a weight float to `BTService_PeekWillingness`.
3. Add one `if` branch in `UBTService_PeekWillingness::TickNode`.

### Broadcast cover entry to allies

`UMYSTAICombatComponent` can broadcast `MYST.AI.Event.EnteredCover` via `UGameplayMessageSubsystem` when the TAKE COVER branch's `MoveTo` completes. Other AIs subscribe and update a `bTeammateInCover` BB key, enabling coordinated single-file peeking without any sensor queries.

### Make reload detection two-way

If you want the AI to score on the *player's* reload even when the player is using a different ability, add `Event.Movement.Reload` to the appropriate `ActivationOwnedTags` in every reload variant GA (sniper, shotgun, etc.). `BTService_AIStateObserver` polls the tag regardless of which ability granted it.

