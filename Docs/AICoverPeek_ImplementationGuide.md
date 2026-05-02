# MYST AI Cover & Peek System — Implementation Guide

> **Covers:** Cover claiming · EQS filtering · Peek location search · Perception · State observation · Peek willingness scoring  
> **Classes implemented:** `UMYSTCoverClaimSubsystem` · `UMYSTEnvQueryTest_ClaimedSpot` · `UBTTask_FindPeekLocation` · `UBTService_PlayerPerception` · `UBTService_AIStateObserver` · `UBTService_PeekWillingness`

---

## System Overview

The system gives each AI three behaviours — **find cover**, **stay in cover**, and **peek out to shoot** — driven entirely by reactive BT Decorators and three aggregating BTServices. There are no random timers; every state transition is triggered by a perception stimulus, a GAS tag change, a health delta, or a scored threshold crossing.

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
│  [Service] BTService_PlayerPerception  ──► HasSeenPlayer,            │
│                                            HasHeardPlayer,           │
│                                            TargetEnemy,              │
│                                            LastKnownLocation         │
│                                                                      │
│  [Service] BTService_UpdateCombatState ──► HealthPct, IsIsolated,   │
│                                            HasTargetInCover,        │
│                                            TargetTimeInCover,       │
│                                            IsTargetEngagingOther    │
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
| `TargetEnemy` | Object | `BTService_PlayerPerception` |

### Location

| Key | Type | Writer |
|---|---|---|
| `MoveGoal` | Vector | MoveTo tasks |
| `LastKnownLocation` | Vector | `BTService_PlayerPerception` |
| `LookAtLocation` | Vector | Custom |
| `CoverLocation` | Vector | `BTT_ClaimCoverSpot` (BP) |
| `PeekLocation` | Vector | `UBTTask_FindPeekLocation` |

### State *(all Boolean unless noted)*

| Key | Type | Writer | Condition |
|---|---|---|---|
| `HasSeenPlayer` | Bool | `BTService_PlayerPerception` | Sight stimulus received; decays after `LostSightCooldown` s |
| `HasHeardPlayer` | Bool | `BTService_PlayerPerception` | Qualifying noise received; decays after `LostHearingCooldown` s |
| `HasEngagedPlayer` | Bool | Custom | — |
| `OutOfAmmo` | Bool | `BTService_AIStateObserver` | AI ASC has `Event.Movement.Reload` tag |
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

> **To add the new BB keys:** open `BB_ShooterAI` in the BB editor, click **New Key**, and add `HasSeenPlayer` (Bool), `HasHeardPlayer` (Bool), `TargetIsReloading` (Bool), `TargetIsLowHealth` (Bool), and `PeekWillingnessScore` (Float).

---

## C++ Class Inventory

| Class | Module | Location | Role |
|---|---|---|---|
| `UMYSTCoverClaimSubsystem` | `MyShooterFeaturePluginRuntime` | `Public/AI/` | WorldSubsystem tracking which cover spots are claimed |
| `UMYSTEnvQueryTest_ClaimedSpot` | `MyShooterFeaturePluginRuntime` | `Public/AI/` | EQS test: filters points claimed by other AIs |
| `UBTTask_FindPeekLocation` | `MyShooterFeaturePluginRuntime` | `Public/AI/` | Samples strafe positions from cover, NavProjects, LOS-traces, writes `PeekLocation` |
| `UBTService_PlayerPerception` | `MyShooterFeaturePluginRuntime` | `Public/AI/` | Delegate-driven perception (sight/hearing/damage); writes `HasSeenPlayer`, `HasHeardPlayer`, `TargetEnemy`, `LastKnownLocation` |
| `UBTService_AIStateObserver` | `MyShooterFeaturePluginRuntime` | `Public/AI/` | Polls GAS tags; delegate-driven damage arming; writes `OutOfAmmo`, `HasTakenDamageRecently`, `TargetIsReloading`, `TargetIsLowHealth` |
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
2. `BTS_CoverClaimLifetime` service activates → `ClaimSpot(CoverLocation, Self)`.
3. `MoveTo(CoverLocation)`.
4. On Sequence exit (success **or** abort) → `BTS_CoverClaimLifetime` deactivates → `ReleaseSpot(Self)`.

> ⚠️ **Why tasks alone are not enough:** `BTT_ClaimCoverSpot` returns `Success` and moves on. If the Sequence is later aborted (e.g., by the Peek Willingness Decorator), that task's abort event **never fires** — the claim leaks. See [Wiring Release on Abort](#wiring-release-on-abort-bts_coverclaimelifetime) for the correct pattern.

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

## `UBTService_PlayerPerception`

**Node name:** `Player Perception (MYST)`  
Ticks at `Interval = 0.1s` (± `RandomDeviation = 0.05s`). Uses `bCreateNodeInstance = true` — each AI controller owns its own UObject instance so timestamps and cached state are stored as plain member variables.

Detection is **fully delegate-driven**: the service binds to `UAIPerceptionComponent::OnTargetPerceptionUpdated` on `OnBecomeRelevant` (with a lazy-retry in `TickNode` if the component isn't ready yet). Tick logic only handles cooldown/retention **expiry**.

### Key writes

```
HasSeenPlayer         ← true immediately on a sight stimulus; cleared after LostSightCooldown s
                        (expiry polls GetCurrentlyPerceivedActors to avoid false expiry while
                        the player is still continuously inside the sight cone)
HasHeardPlayer        ← true on a qualifying noise (Loudness >= HearingLoudnessThreshold);
                        cleared after LostHearingCooldown s
TargetEnemy           ← set to detected pawn on sight/hearing/damage; cleared together with
                        LastKnownLocation (LastKnownLocationRetention expiry)
LastKnownLocation     ← updated on every sight refresh (live actor location) or hearing event
                        (stimulus location); set to instigator position on damage events;
                        cleared LastKnownLocationRetention s after the last detection
```

### Key properties

| Property | Default | Notes |
|---|---|---|
| `HasSeenPlayerKey` | — | Bool BB key |
| `TargetEnemyKey` | — | Object BB key (`AActor`) |
| `LastKnownLocationKey` | — | Vector BB key |
| `HasHeardPlayerKey` | — | Bool BB key; leave unset to disable hearing |
| `LostSightCooldown` | `5.0 s` | How long `HasSeenPlayer` stays `true` after LOS breaks |
| `LastKnownLocationRetention` | `15.0 s` | How long `LastKnownLocation` + `TargetEnemy` persist; should be ≥ `LostSightCooldown` |
| `bOnlyDetectPlayers` | `true` | Skips non-player-controlled pawns for sight/hearing |
| `LostHearingCooldown` | `3.0 s` | How long `HasHeardPlayer` stays `true` |
| `HearingLoudnessThreshold` | `0.5` | Minimum `FAIStimulus::Strength` to arm `HasHeardPlayer` |
| `bOnlyDamageByPlayers` | `true` | Skips damage from non-player instigators |

### Blueprint hooks (implement in BP subclasses)

| Event | When it fires |
|---|---|
| `OnPlayerFirstSpotted(SpottedPawn)` | First `false → true` transition of `HasSeenPlayer` |
| `OnPlayerLost()` | `HasSeenPlayer` transitions `true → false` (cooldown elapsed and no active perception) |
| `OnNoiseHeard(NoiseMaker, Location, Loudness)` | Every qualifying hearing stimulus |
| `OnDamageTaken(AttackerPawn, InstigatorLocation)` | Every damage stimulus that passes the player filter |

### Setup checklist

1. Add `UAIPerceptionComponent` to the AI Controller Blueprint.
   - **Sight config** — set `SightRadius`, `LoseSightRadius`, `PeripheralVisionAngle`; set `DetectEnemies=true`, `DetectFriendlies=false`.
   - **Hearing config** — set radius and affiliation.
   - **Damage config** (`UAISenseConfig_Damage`) — no affiliation filter needed.
2. Ensure the player Character Blueprint has `UAIPerceptionStimuliSourceComponent` (or enable `bAutoRegisterAllPawnsAsSources` in AI Perception System settings).
3. In your damage-handling code (e.g. inside a GA or `ULyraHealthComponent` callback) call:
   ```cpp
   UAISense_Damage::ReportDamageEvent(World, DamagedActor, Instigator,
                                      Amount, InstigatorLocation, HitLocation);
   ```
4. Place this service on the BT root Selector and wire the four BB key selectors.
5. Tune `LostSightCooldown` and `LastKnownLocationRetention` on the node.

> **Why `UAIPerceptionComponent` over `UPawnSensingComponent`:**  
> `ALyraPlayerBotController` already implements `GetTeamAttitudeTowards`, which `UAIPerceptionComponent` uses natively for affiliation filtering. A single `OnTargetPerceptionUpdated` delegate covers sight, hearing, and damage with full UE5 per-sense config support and editor tooling (AI Perception debug channel).

---

## `UBTService_AIStateObserver`

**Node name:** `AI State Observer (MYST)`  
Ticks at `Interval = 0.15s` (± `RandomDeviation = 0.05s`). Uses `bCreateNodeInstance = true` — each AI controller gets its own UObject instance; per-AI state (`LastDamageTime`, `bWasReloading`) is stored as plain member variables with no manual node-memory management.

**`HasTakenDamageRecently` is delegate-driven, not tick-polled.** The service binds to `ULyraHealthComponent::OnHealthChanged` in `OnBecomeRelevant` (with a lazy-retry in `TickNode` for late pawn possession). The BB key is set to `true` *immediately* when a qualifying hit lands — the tick only handles the cooldown expiry.

### Key writes

```
OutOfAmmo              ← AI ASC HasMatchingGameplayTag(ReloadTag)   [polled each tick]
HasTakenDamageRecently ← armed immediately via OnHealthChanged delegate when damage fraction
                          >= DamageReactionThreshold; cleared DamageCooldown s later by tick
TargetIsReloading      ← Target ASC HasMatchingGameplayTag(ReloadTag)   [polled each tick]
TargetIsLowHealth      ← ULyraHealthComponent::GetHealthNormalized() < LowHealthThreshold
                          [polled each tick, resolved via FindHealthComponent]
```

### Key properties

| Property | Default | Notes |
|---|---|---|
| `ReloadTag` | `Event.Movement.Reload` | Pre-resolved at construction. Must match `ActivationOwnedTags` on the reload GA |
| `DamageReactionThreshold` | `0.04` (4% MaxHP) | Filters DoT micro-pulses |
| `DamageCooldown` | `4.0 s` | How long `HasTakenDamageRecently` stays `true` after a qualifying hit |
| `LowHealthThreshold` | `0.35` (35%) | Target health gate for `TargetIsLowHealth` |

### Blueprint hooks (implement in BP subclasses)

| Event | When it fires |
|---|---|
| `OnAIReloadStateChanged(bool bIsNowReloading)` | Every reload state transition (start & end) |
| `OnDamageDetected(float HealthLostFraction)` | The moment a qualifying damage hit is detected (inside the `OnHealthChanged` delegate, not next tick) |

> **Player ASC resolution note:** `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor` resolves the Lyra player's ASC from `ALyraPlayerState` automatically via `IAbilitySystemInterface`. You only need to pass the pawn (the value stored in `TargetEnemy`).

> **Health component binding:** `ULyraHealthComponent::FindHealthComponent(Pawn)` is used to locate the component. The bind is attempted eagerly in `OnBecomeRelevant`; `TickNode` retries every tick until it succeeds (handles late pawn possession after the service becomes relevant).

---

## `UBTService_PeekWillingness`

**Node name:** `Peek Willingness (MYST)`  
Ticks at `Interval = 0.20s` (± `RandomDeviation = 0.05s`) as a pure aggregator — reads keys written by both upstream services and writes `PeekWillingnessScore` [0, 1].

Uses **per-node instance memory** (`FPeekWillingnessMemory`) — a private struct that tracks `bWasReady` (for the Blueprint threshold transition event) and `LastWrittenScore` (for the dead-band write guard described below). Unlike `BTService_AIStateObserver`, this service does **not** use `bCreateNodeInstance`; the memory is stored in the standard BT node memory block.

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

Score is only written to the BB when it has changed by more than `ScoreWriteDeadBand` (see below).

When `Score >= PeekThreshold` (default `0.35`) the Peek Sequence is unblocked. The BT Decorator (with `AbortSelf+LowerPriority`) makes this fully reactive — no extra C++ event-handling needed.

### Key properties

| Property | Default | Notes |
|---|---|---|
| `PeekThreshold` | `0.35` | Score gate for the BB Decorator |
| `ScoreWriteDeadBand` | `0.02` | Minimum score delta required to write to BB (**critical** — see note below) |
| `TargetIsReloadingBonus` | `0.40` | |
| `TargetExposedBonus` | `0.25` | |
| `TargetLowHealthBonus` | `0.20` | |
| `TargetDistractedBonus` | `0.15` | |
| `TargetOverduePeekBonus` | `0.10` | |
| `TargetOverduePeekTime` | `5.0 s` | |
| `OutOfAmmoPenalty` | `0.60` | |
| `RecentlyHitPenalty` | `0.40` | |
| `LowSelfHealthPenalty` | `0.30` | |
| `LowSelfHealthThreshold` | `0.30` | Self health fraction that triggers the penalty |
| `IsolatedPenalty` | `0.20` | |

> ⚠️ **`ScoreWriteDeadBand` — do not set to 0 in production.** The score is an arithmetic sum of several input floats (`HealthPct`, `TargetCoverTime`). Even micro-changes produce a different float result every tick. Without the dead-band, each tick fires a BB change notification on `PeekWillingnessScore`, which triggers any `Observer-Abort` Decorator watching that key — restarting `EQS_FindCover` before it can finish and causing the BT to loop indefinitely. Default `0.02` suppresses this without meaningfully delaying peeks.

### Blueprint hook

| Event | When it fires |
|---|---|
| `OnPeekReadinessChanged(bool bIsNowReady)` | Every time score crosses `PeekThreshold` in either direction |

---

## Behavior Tree Layout

Attach all four services to the **Combat Selector root** so they tick regardless of which branch is active. Service execution order (top-to-bottom) ensures `PlayerPerception` populates `TargetEnemy` before `AIStateObserver` reads it, and `AIStateObserver` populates its outputs before `PeekWillingness` aggregates them.

```
Root
└── Selector  [Combat Root]
    ├── [Service] BTService_PlayerPerception         ← tick first (populates TargetEnemy)
    ├── [Service] BTService_UpdateCombatState        ← tick second
    ├── [Service] BTService_AIStateObserver          ← tick third
    ├── [Service] BTService_PeekWillingness          ← tick last (reads outputs of all three)
    │
    ├── Sequence  [PEEK & SHOOT]                     HIGH priority
    │   ├── Decorator: BB  TargetEnemy != null            AbortSelf+LowerPriority
    │   ├── Decorator: BB  HasSeenPlayer == true           AbortSelf+LowerPriority
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
        [Service] BTS_CoverClaimLifetime
        ├── RunEQSQuery → CoverLocation
        │     (EQS_FindCover + UMYSTEnvQueryTest_ClaimedSpot)
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
| `HasSeenPlayer` | Bool |
| `HasHeardPlayer` | Bool |
| `TargetIsReloading` | Bool |
| `TargetIsLowHealth` | Bool |
| `PeekWillingnessScore` | Float |

### 2. Configure `BTService_PlayerPerception` on the Combat Selector

Wire each selector to the matching BB key name:

| Selector field | BB key to point at |
|---|---|
| `HasSeenPlayerKey` | `HasSeenPlayer` |
| `TargetEnemyKey` | `TargetEnemy` |
| `LastKnownLocationKey` | `LastKnownLocation` |
| `HasHeardPlayerKey` | `HasHeardPlayer` (or leave `None` to disable hearing) |

Tune `LostSightCooldown` (default `5 s`) and `LastKnownLocationRetention` (default `15 s`) to match your map scale.

### 3. Configure `BTService_AIStateObserver` on the Combat Selector

Wire each selector to the matching BB key name:

| Selector field | BB key to point at |
|---|---|
| `OutOfAmmoKey` | `OutOfAmmo` |
| `HasTakenDamageRecentlyKey` | `HasTakenDamageRecently` |
| `TargetEnemyKey` | `TargetEnemy` |
| `TargetIsReloadingKey` | `TargetIsReloading` |
| `TargetIsLowHealthKey` | `TargetIsLowHealth` |

### 4. Configure `BTService_PeekWillingness` on the Combat Selector

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

Leave `ScoreWriteDeadBand` at `0.02` unless you have a specific reason to change it (see the dead-band note in the service section above).

### 5. Add Decorators to the Peek Sequence

| Decorator | Key | Condition | Observer Aborts |
|---|---|---|---|
| Blackboard | `TargetEnemy` | Is Set | Both |
| Blackboard | `HasSeenPlayer` | Is Set / == true | Both |
| Blackboard | `PeekWillingnessScore` | Float >= `0.35` | Both |
| Blackboard | `OutOfAmmo` | Is Not Set / == false | Self |
| Blackboard | `HasTakenDamageRecently` | Is Not Set / == false | Self |

### 6. Verify `BTT_ReleaseCoverSpot` fires on abort

In the BT editor, confirm the BP task fires when the **TAKE COVER** Sequence is interrupted — not only when it succeeds. The recommended pattern is an **On Abort** service or a **Decorator On Abort** that calls `ReleaseSpot(SelfActor)`. Without this, the `UMYSTCoverClaimSubsystem` will accumulate ghost claims for dead or fleeing AIs.

---

## Wiring Release on Abort: `BTS_CoverClaimLifetime`

### Why tasks alone are not enough

`BTT_ClaimCoverSpot` returns `Success` and the Sequence moves on to `MoveTo`. If the Peek Willingness Decorator then aborts the Sequence, the BT kills `MoveTo` (the currently running node). `BTT_ClaimCoverSpot` already finished — its abort event **never fires**. `BTT_ReleaseCoverSpot` as a downstream task **never runs** either. The claim leaks.

### The fix: a BTService on the Sequence node

A **BTService** attached to a node fires `Event Receive Deactivation` when that node stops being relevant — on **both normal completion and abort**. This is the standard UE5 cleanup pattern.

### Creating `BTS_CoverClaimLifetime` (Blueprint)

**1.** Create a new Blueprint class inheriting `BTService`. Name it `BTS_CoverClaimLifetime`.

**2.** Set `Interval = 0` and uncheck **Call Tick on Search Start** — no ticking needed.

**3.** Implement two events:

```
Event Receive Activation AI
  └─► GetWorldSubsystem (UMYSTCoverClaimSubsystem)
        └─► ClaimSpot(
              Location = GetBlackboardValueAsVector[CoverLocation],
              Claimer  = Controlled Pawn,
              Radius   = 150.0
            )

Event Receive Deactivation AI          ← fires on SUCCESS and ABORT
  └─► GetWorldSubsystem (UMYSTCoverClaimSubsystem)
        └─► ReleaseSpot(Claimer = Controlled Pawn)
```

**4.** In the BT, **drag the service onto the TAKE COVER Sequence header** (not into a child slot). Remove `BTT_ClaimCoverSpot` from inside the Sequence — the service's activation now handles claiming.

```
Sequence  [TAKE COVER]
  [Service] BTS_CoverClaimLifetime   ← Activation=Claim, Deactivation=Release
  ├── RunEQSQuery → CoverLocation
  ├── MoveTo (CoverLocation)
  └── Wait (0.1 s)
```

### Alternative: keep the existing BP tasks, add a thin service

If `BTT_ClaimCoverSpot` is reused elsewhere, keep it as a task and add a minimal service that only handles the release:

**1.** Create `BTS_ReleaseCoverOnExit` Blueprint BTService.

**2.** Implement only:
```
Event Receive Deactivation AI
  └─► GetWorldSubsystem (UMYSTCoverClaimSubsystem)
        └─► ReleaseSpot(Controlled Pawn)
```

**3.** Attach it to the TAKE COVER Sequence. `BTT_ClaimCoverSpot` stays as a task inside.

```
Sequence  [TAKE COVER]
  [Service] BTS_ReleaseCoverOnExit   ← only handles release on exit/abort
  ├── RunEQSQuery → CoverLocation
  ├── BTT_ClaimCoverSpot  (BP)       ← still claims on task execution
  ├── MoveTo (CoverLocation)
  └── Wait (0.1 s)
```

> `ReleaseSpot` is safe to call even when no claim exists, so a normal-completion double-release (task then service) is harmless.

---

## Tuning Guide

### Quick archetype presets

| Archetype | `PeekThreshold` | Key weight changes |
|---|---|---|
| **Aggressive** | `0.20` | Halve `RecentlyHitPenalty` and `LowSelfHealthPenalty` |
| **Cowardly** | `0.55` | Double `RecentlyHitPenalty`; add +0.20 to `OutOfAmmoPenalty` |
| **Flanker** | `0.25` | Increase `TargetDistractedBonus` to `0.35` |
| **Sniper** | `0.40` | `MaxSteps = 1`, `StepSize = 200 cm` in `UBTTask_FindPeekLocation` |

To create a per-archetype variant: **subclass `BTService_PeekWillingness` in Blueprint**, override the weight defaults in Class Defaults. No C++ recompile needed.

### Tick interval vs. responsiveness

| Service | Default interval | Reduce to… | Effect |
|---|---|---|---|
| `BTService_PlayerPerception` | 0.10 s | 0.05 s | Cooldown expiry checks run more frequently (detection is still instant via delegate) |
| `BTService_AIStateObserver` | 0.15 s | 0.08 s | `OutOfAmmo` / `TargetIsReloading` tags polled faster; `HasTakenDamageRecently` arming is already instant |
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

In PIE, open **Window → Behavior Tree Debugger** and select the AI pawn. Watch `PeekWillingnessScore`, `HasSeenPlayer`, and `HasHeardPlayer` update in real time as you (the player) reload, step out of cover, or damage the AI.

### Log tags

Add `LogBehaviorTree Verbose` to `DefaultEngine.ini` if decorators are not aborting as expected:

```ini
[Core.Log]
LogBehaviorTree=Verbose
```

### Check GAS tags are being granted

In PIE, with the player selected: **Gameplay Debugger** → `AbilitySystem` category (`'` key by default). Confirm `Event.Movement.Reload` appears in **Active Tags** while the player is reloading.

### Check perception stimuli

In PIE with the AI pawn selected, enable **Gameplay Debugger** → `Perception` category. Confirm the sight/hearing/damage senses show the expected sensed actors and stimulus ages. If `HasSeenPlayer` never arms, verify the player pawn has `UAIPerceptionStimuliSourceComponent` (or `bAutoRegisterAllPawnsAsSources` is on).

---

## Lifecycle & Safety

This section documents the known lifetime hazards in every service, the fixes applied, and what to watch for when extending the code.

### How the normal shutdown path works

```
PIE stop / level unload
  └─► UWorld::BeginTearingDown()
        └─► Actor::EndPlay (all actors)
              └─► AI Controller: UnPossess / Destroy
                    └─► UBehaviorTreeComponent::StopLogic
                          └─► CeaseRelevantNodes
                                └─► OnCeaseRelevant (each active service)
                                      ├─ BTService_PlayerPerception → UnbindPerceptionDelegates()
                                      └─ BTService_AIStateObserver  → UnbindHealthDelegate()
```

`OnCeaseRelevant` is the **primary** cleanup path and covers the common case. The fixes below harden the edge cases where that path is bypassed or races with in-flight callbacks.

---

### Bug 1 — `BTService_AIStateObserver::TickNode` — unchecked `GetWorld()` dereference *(crash)*

**Was:**
```cpp
const float Elapsed = GetWorld()->GetTimeSeconds() - LastDamageTime;
```
`GetWorld()` can return `nullptr` on a `bCreateNodeInstance` UObject that outlives its outer world — a real scenario during PIE stop when GC ordering is non-deterministic. Dereferencing null is an instant crash.

**Fix applied:** capture + null-check before use:
```cpp
UWorld* World = GetWorld();
if (!World) { return; }
const float Elapsed = World->GetTimeSeconds() - LastDamageTime;
```

---

### Bug 2 — `BTService_AIStateObserver::OnAIHealthChanged` — `OnDamageDetected` fires outside the BT guard *(wrong behaviour)*

**Was:**
```cpp
if (UBehaviorTreeComponent* BTComp = OwnerBTComp.Get())
{
    // ... write BB key ...
}
OnDamageDetected(DamageFraction);   // ← fires even when BTComp is null / BT stopped
```
When an AI pawn dies, `ULyraHealthComponent::OnHealthChanged` fires with the killing blow **before** the controller calls `StopLogic`. `OwnerBTComp` can be null or the BT can be mid-shutdown. The `OnDamageDetected` Blueprint event was reaching BP subclass logic in that incomplete state, making any BP code that accesses BB keys or the AI pawn unsafe.

**Fix applied:** `OnDamageDetected` moved inside the `OwnerBTComp` guard. Two additional early-returns also added:
```cpp
if (NewValue <= 0.f) { return; }                        // skip the killing blow
if (!World || World->IsTearingDown()) { return; }       // skip during world teardown
```

---

### Bug 3 — Both delegate-binding services — no `BeginDestroy` override *(delegate leak / potential crash)*

`BTService_PlayerPerception` binds to `UAIPerceptionComponent::OnTargetPerceptionUpdated`.  
`BTService_AIStateObserver` binds to `ULyraHealthComponent::OnHealthChanged`.  
Both use `bCreateNodeInstance = true` — each is a live `UObject` managed by the BT component.

In an edge case (forced actor destroy, crash recovery, PIE stop that bypasses the normal BT shutdown), GC can collect the node UObject **before** `OnCeaseRelevant` fires. The source component then holds an `FScriptDelegate` whose target UObject has been destroyed. The next stimulus causes Unreal to iterate the multicast binding list and — in the best case — silently skips the stale entry (with a growing binding-list overhead); in the worst case it reaches the destroyed instance in a narrow timing window.

**Fix applied:** `BeginDestroy()` override added to both services, calling the same idempotent `Unbind*()` helper that `OnCeaseRelevant` already uses:
```cpp
void UBTService_XXX::BeginDestroy()
{
    UnbindXDelegates();   // safe to call even if nothing is bound
    Super::BeginDestroy();
}
```

---

### Bug 4 — `BTService_PlayerPerception::HandlePerceptionUpdated` — no `IsTearingDown` guard *(stale processing)*

The perception system dispatches `OnTargetPerceptionUpdated` during `EndPlay` (e.g. sight-loss stimuli triggered by actors being destroyed). At that point `OwnerBTComp` is often still non-null, so the existing weak-pointer guard doesn't protect against writes to a shutting-down BB.

**Fix applied:** explicit early-return at the top of `HandlePerceptionUpdated`:
```cpp
UWorld* World = GetWorld();
if (!World || World->IsTearingDown()) { return; }
```

---

### Issue 5 — `BTService_PeekWillingness` — placement-new with no matching destructor call *(future hygiene)*

`InitializeMemory` allocates `FPeekWillingnessMemory` via placement `new`. The UE 5.7 `UBTNode` API does not expose a `CleanupMemory` virtual, so a matching destructor call cannot be added as an override in this version of the engine.

`FPeekWillingnessMemory` currently holds only `bool bWasReady` and `float LastWrittenScore` — both trivially destructible, so there is **no actual leak today**.

**What to do if you add non-trivial members** (e.g. `FTimerHandle`, `TArray`, `TWeakObjectPtr`):
- Check whether the target UE version has added `virtual CleanupMemory(...)` to `UBTNode` and add the override then.
- Otherwise, wrap non-trivial state in a sub-object held by the service itself (`bCreateNodeInstance = true` + member variable), rather than inside `FPeekWillingnessMemory`.

---

### Summary table

| # | Service | Severity | Root cause | Fix |
|---|---|---|---|---|
| 1 | `AIStateObserver` | **Crash** | `GetWorld()` unchecked in `TickNode` | Captured + null-checked before use |
| 2 | `AIStateObserver` | Wrong behaviour | `OnDamageDetected` outside BT guard | Moved inside guard; added dead-pawn + teardown returns |
| 3 | `PlayerPerception` + `AIStateObserver` | Leak/crash risk | No `BeginDestroy` on `bCreateNodeInstance` UObjects | `BeginDestroy()` added to both; calls existing `Unbind*()` |
| 4 | `PlayerPerception` | Stale processing | No `IsTearingDown` guard in perception delegate | Early-return added at top of `HandlePerceptionUpdated` |
| 5 | `PeekWillingness` | Future hygiene | Placement-new; `CleanupMemory` virtual absent in UE 5.7 | Documented; no action needed while struct is all-POD |

---

## Extending the System

### Add a new scoring signal

1. Write the new BB key from either `BTService_PlayerPerception` (if it needs perception data), `BTService_AIStateObserver` (if it needs GAS/health access), or directly from any other BT node.
2. Add a `FBlackboardKeySelector` property and a weight float to `BTService_PeekWillingness`.
3. Add one `if` branch in `UBTService_PeekWillingness::TickNode`.

### Broadcast cover entry to allies

`UMYSTAICombatComponent` can broadcast `MYST.AI.Event.EnteredCover` via `UGameplayMessageSubsystem` when the TAKE COVER branch's `MoveTo` completes. Other AIs subscribe and update a `bTeammateInCover` BB key, enabling coordinated single-file peeking without any sensor queries.

### Make reload detection two-way

If you want the AI to score on the *player's* reload even when the player is using a different ability, add `Event.Movement.Reload` to the appropriate `ActivationOwnedTags` in every reload variant GA (sniper, shotgun, etc.). `BTService_AIStateObserver` polls the tag regardless of which ability granted it.

### Extend hearing to non-player sources

Set `bOnlyDetectPlayers = false` on `BTService_PlayerPerception` and verify the hearing configs on the AI controller perception component. `HasHeardPlayer` (name is suggestive — rename the BB key if needed) will then arm on any noise maker.
