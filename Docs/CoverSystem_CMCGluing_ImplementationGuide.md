# Cover System — CMC Gluing Implementation Guide

## Problem Statement

The existing `BP_AutoCoverComponent` uses `Add Movement Input` with a World Direction derived from live shoulder-trace normals to move the character along cover. While it avoids raw `Set Actor Location`, three issues remain:

1. **Animation drift on edge traces** — shoulder socket positions shift with animation poses, causing the trace origin to be unreliable for lean/edge detection.
2. **Mesh alignment variance** — the hit normal captured per-frame from the character's current position can vary as the CMC resolves collisions differently frame-to-frame.
3. **No true perpendicular glue** — `Add Movement Input` along the tangent relies on the capsule physically pressing against the wall for perpendicular constraint. If there is any gap (slight misalignment, sloped geometry), the character drifts away.

## Solution Overview

Move the surface constraint into `ULyraCharacterMovementComponent` via a **custom movement mode** (`MOVE_Custom`, index `0`). On cover entry, the CMC stores the wall surface data once from the initial hit result. `PhysCustom` then:

- Projects player input (`Acceleration`, fed by `Add Movement Input`) onto the **stored** surface tangent (not a live trace).
- Fires a single perpendicular trace each physics step to snap the character to exactly `DistanceFromWall` from the wall surface.
- Handles gravity and floor detection natively.

The BP component delegates all movement math to the CMC. It only needs to call `EnterCoverMode` / `ExitCoverMode` and keep managing the GAS cover status tags (`Status.Cover`, `Status.Cover.CanLean*`) as before.

Lyra's existing `OnMovementModeChanged → SetMovementModeTag` wiring automatically fires/removes the `Movement.Mode.Cover` GAS tag on the character's ASC whenever the mode changes, at no extra cost.

---

## Architecture Diagram

```
BP_AutoCoverComponent
  │  On overlap / tag hit:
  ├─► EnterCoverMode(WallHit, DistanceFromWall, MaxSpeed)   ← NEW call
  │     └─ CMC stores: SurfaceNormal, SurfaceTangent,
  │                    DistanceFromWall, CoverComponent
  │        SetMovementMode(MOVE_Custom, COVER_CUSTOM_MODE=0)
  │        Lyra tag wiring: fires "Movement.Mode.Cover" on ASC
  │
  │  MoveInCover event (each input action tick):
  ├─► Add Movement Input (World Direction = raw input dir, Scale = ActionValue)
  │     └─ CMC PhysCustom:
  │          1. Project Acceleration onto CoverSurfaceTangent → lateral scalar
  │          2. Compute lateral XY velocity
  │          3. Perpendicular line trace → snap to HitPoint + Normal * DistFromWall
  │          4. Apply gravity + floor sweep (Z axis)
  │          5. SafeMoveUpdatedComponent(FinalDelta, sweep=true)
  │
  │  Lean / Edge detection (unchanged owner, improved trace anchor):
  ├─► Line trace START = GetActorLocation() ± CoverSurfaceTangent * EdgeDist
  │     (was: shoulder socket world position → animation-dependent)
  │
  │  On exit cover:
  └─► ExitCoverMode()
        └─ CMC clears state, SetMovementMode(MOVE_Walking)
           Lyra tag wiring: removes "Movement.Mode.Cover" from ASC
```

---

## C++ Changes

### 1. `Source/LyraGame/LyraGameplayTags.h`

Add the new cover movement mode tag declaration alongside the other `Movement_Mode_*` extern declarations:

```cpp
// ...existing movement mode tag declarations...
LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Cover);
```

---

### 2. `Source/LyraGame/LyraGameplayTags.cpp`

#### 2a. Define the tag

Alongside the other `Movement_Mode_*` definitions:

```cpp
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Cover, "Movement.Mode.Cover", "Character is hugging cover.");
```

#### 2b. Register it in `CustomMovementModeTagMap`

Index `0` is the first free custom mode slot. This is what `ALyraCharacter::SetMovementModeTag` reads when `MovementMode == MOVE_Custom`:

```cpp
// Custom Movement Modes
const TMap<uint8, FGameplayTag> CustomMovementModeTagMap =
{
    { 0, Movement_Mode_Cover }   // COVER_CUSTOM_MODE = 0
};
```

> **Note:** `Movement.Mode.Cover` does **not** need to be added to `DefaultGameplayTags.ini` — it is defined natively here via `UE_DEFINE_GAMEPLAY_TAG_COMMENT`, exactly like the other `Movement.Mode.*` tags.

---

### 3. `Source/LyraGame/Character/LyraCharacterMovementComponent.h`

Full additions to the class:

```cpp
// --- Cover Mode constant ---
public:
    /** Index used for MOVE_Custom when in cover. Matches the entry in CustomMovementModeTagMap. */
    static constexpr uint8 COVER_CUSTOM_MODE = 0;

// --- Cover mode API (BlueprintCallable) ---
public:
    /**
     * Enters cover mode using the provided wall hit result.
     * Stores surface data once; PhysCustom uses it every step.
     * @param WallHit       The initial hit result from the cover detection trace/overlap.
     * @param DistFromWall  How far the capsule center stays from the wall surface (cm).
     * @param MaxSpeed      Lateral movement speed along cover (cm/s).
     */
    UFUNCTION(BlueprintCallable, Category = "Lyra|CharacterMovement|Cover")
    void EnterCoverMode(const FHitResult& WallHit, float DistFromWall, float MaxSpeed);

    /** Exits cover mode and restores MOVE_Walking. */
    UFUNCTION(BlueprintCallable, Category = "Lyra|CharacterMovement|Cover")
    void ExitCoverMode();

    /** Returns true if currently in the cover custom movement mode. */
    UFUNCTION(BlueprintPure, Category = "Lyra|CharacterMovement|Cover")
    bool IsInCoverMode() const;

// --- Cover state (read from BP for lean/edge traces) ---
public:
    /** Wall surface normal, captured once on EnterCoverMode. */
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Cover State")
    FVector CoverSurfaceNormal = FVector::ZeroVector;

    /**
     * Lateral axis along the wall surface.
     * Computed as Cross(UpVector, CoverSurfaceNormal) on entry.
     * Use this as the trace direction for lean/edge detection instead of shoulder sockets.
     */
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Cover State")
    FVector CoverSurfaceTangent = FVector::ZeroVector;

    /** Target perpendicular distance from wall surface (cm). */
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Cover State")
    float CoverDistanceFromWall = 50.0f;

    /** Maximum lateral speed while in cover (cm/s). */
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Cover State")
    float CoverMaxMoveSpeed = 250.0f;

    /** The primitive component whose surface we are attached to. Used to validate wall snaps. */
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Cover State")
    TObjectPtr<UPrimitiveComponent> CoverComponent = nullptr;

// --- CMC overrides ---
protected:
    virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

    // GetMaxSpeed() is already overridden — add a cover branch at the top (see .cpp).
```

---

### 4. `Source/LyraGame/Character/LyraCharacterMovementComponent.cpp`

#### 4a. `EnterCoverMode`

```cpp
void ULyraCharacterMovementComponent::EnterCoverMode(const FHitResult& WallHit, float DistFromWall, float MaxSpeed)
{
    if (!CharacterOwner)
    {
        return;
    }

    // Store surface data from the initial hit — never updated from animations.
    CoverSurfaceNormal    = WallHit.Normal;
    CoverSurfaceTangent   = FVector::CrossProduct(FVector::UpVector, WallHit.Normal).GetSafeNormal();
    CoverDistanceFromWall = DistFromWall;
    CoverMaxMoveSpeed     = MaxSpeed;
    CoverComponent        = WallHit.GetComponent();

    // Rotate character to face away from the cover wall.
    const FRotator FaceAwayFromWall = UKismetMathLibrary::MakeRotFromX(-CoverSurfaceNormal);
    CharacterOwner->SetActorRotation(FaceAwayFromWall);

    // Switch movement mode — triggers Lyra's OnMovementModeChanged → SetMovementModeTag
    // which fires the "Movement.Mode.Cover" GAS tag on the ASC automatically.
    SetMovementMode(MOVE_Custom, COVER_CUSTOM_MODE);
}
```

> Add `#include "Kismet/KismetMathLibrary.h"` to the .cpp if not already present.

#### 4b. `ExitCoverMode`

```cpp
void ULyraCharacterMovementComponent::ExitCoverMode()
{
    CoverSurfaceNormal    = FVector::ZeroVector;
    CoverSurfaceTangent   = FVector::ZeroVector;
    CoverDistanceFromWall = 50.0f;
    CoverMaxMoveSpeed     = 250.0f;
    CoverComponent        = nullptr;

    // Restores MOVE_Walking — tag wiring removes "Movement.Mode.Cover" from ASC.
    SetMovementMode(MOVE_Walking);
}
```

#### 4c. `IsInCoverMode`

```cpp
bool ULyraCharacterMovementComponent::IsInCoverMode() const
{
    return MovementMode == MOVE_Custom && CustomMovementMode == COVER_CUSTOM_MODE;
}
```

#### 4d. `GetMaxSpeed` — add cover branch

```cpp
float ULyraCharacterMovementComponent::GetMaxSpeed() const
{
    // Cover mode has its own speed; check before the Gameplay tag path.
    if (IsInCoverMode())
    {
        return CoverMaxMoveSpeed;
    }

    // ...existing TAG_Gameplay_MovementStopped check and Super::GetMaxSpeed()...
}
```

#### 4e. `PhysCustom`

This is the core of the feature. It handles:
- Lateral movement constrained to `CoverSurfaceTangent`
- Perpendicular snap via a wall trace every step
- Gravity and floor contact

```cpp
void ULyraCharacterMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
    Super::PhysCustom(DeltaTime, Iterations);

    // Only handle our cover mode; ignore other potential future custom modes.
    if (CustomMovementMode != COVER_CUSTOM_MODE)
    {
        return;
    }

    if (DeltaTime < MIN_TICK_TIME || !CharacterOwner || CoverSurfaceTangent.IsNearlyZero())
    {
        return;
    }

    // -----------------------------------------------------------------------
    // 1. LATERAL: project input acceleration onto the stored tangent axis.
    //    Acceleration is populated from Add Movement Input before PhysCustom runs.
    // -----------------------------------------------------------------------
    const float LateralScale = FVector::DotProduct(Acceleration, CoverSurfaceTangent);
    // Map to [-1, 1] relative to MaxAcceleration, then scale by CoverMaxMoveSpeed.
    const float LateralSpeed = FMath::Clamp(LateralScale / FMath::Max(GetMaxAcceleration(), 1.0f), -1.0f, 1.0f)
                               * CoverMaxMoveSpeed;

    Velocity = CoverSurfaceTangent * LateralSpeed;
    Velocity.Z = 0.0f;  // Vertical handled separately below.

    // -----------------------------------------------------------------------
    // 2. PERPENDICULAR SNAP: from the desired lateral position, trace toward
    //    the wall and snap to exactly CoverDistanceFromWall.
    // -----------------------------------------------------------------------
    const FVector CurrentLocation = UpdatedComponent->GetComponentLocation();
    const FVector LateralDelta    = FVector(Velocity.X, Velocity.Y, 0.0f) * DeltaTime;
    const FVector ProbeOriginXY   = FVector(CurrentLocation.X + LateralDelta.X,
                                            CurrentLocation.Y + LateralDelta.Y,
                                            CurrentLocation.Z);

    const FVector TraceStart = ProbeOriginXY + CoverSurfaceNormal * 200.0f;
    const FVector TraceEnd   = ProbeOriginXY - CoverSurfaceNormal * 200.0f;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CoverPhysCustomSnap), false, CharacterOwner);
    FHitResult WallHit;
    const bool bHitWall = GetWorld()->LineTraceSingleByChannel(
        WallHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

    FVector TargetXY = ProbeOriginXY;
    if (bHitWall && FVector::DotProduct(WallHit.Normal, CoverSurfaceNormal) > 0.7f)
    {
        // Good wall hit — snap perpendicular distance.
        const FVector SnapPoint = WallHit.ImpactPoint + WallHit.Normal * CoverDistanceFromWall;
        TargetXY = FVector(SnapPoint.X, SnapPoint.Y, CurrentLocation.Z);

        // Keep the stored normal fresh for curved surfaces.
        CoverSurfaceNormal  = WallHit.Normal;
        CoverSurfaceTangent = FVector::CrossProduct(FVector::UpVector, CoverSurfaceNormal).GetSafeNormal();
    }
    else
    {
        // Lost contact with cover (walked off edge or obstruction).
        ExitCoverMode();
        return;
    }

    // -----------------------------------------------------------------------
    // 3. GRAVITY + FLOOR: integrate Z velocity and sweep downward so the
    //    character stays grounded without reimplementing PhysWalking.
    // -----------------------------------------------------------------------
    const float PrevVelZ = Velocity.Z;
    Velocity.Z += GetGravityZ() * DeltaTime;
    const float AvgVelZ  = 0.5f * (PrevVelZ + Velocity.Z);
    const float ZDelta   = AvgVelZ * DeltaTime;

    // -----------------------------------------------------------------------
    // 4. MOVE: apply the combined XY snap + Z gravity in one swept move.
    // -----------------------------------------------------------------------
    const FVector MoveDelta = (TargetXY - CurrentLocation) + FVector(0.0f, 0.0f, ZDelta);

    FHitResult MoveHit;
    SafeMoveUpdatedComponent(MoveDelta, UpdatedComponent->GetComponentQuat(), true, MoveHit);

    if (MoveHit.bBlockingHit)
    {
        if (MoveHit.Normal.Z > GetWalkableFloorZ())
        {
            // Landed on floor — zero vertical velocity.
            Velocity.Z = 0.0f;
        }

        HandleImpact(MoveHit, DeltaTime, MoveDelta);

        // Let the character slide along walls/corners without exiting cover.
        SlideAlongSurface(MoveDelta, 1.0f - MoveHit.Time, MoveHit.Normal, MoveHit, true);
    }
}
```

> **Include guard**: Add `#include "Kismet/KismetMathLibrary.h"` in the `.cpp` if `EnterCoverMode` needs it. `PhysCustom` only uses engine-internal CMC APIs.

---

## Blueprint Changes — `BP_AutoCoverComponent`

### 5. On Enter Cover

Where the overlap / tag check currently fires:

| Before | After |
|---|---|
| Store `CoverNormal`, `CoverTangent`, etc. in BP variables | Call `Get Character Movement` → Cast to `ULyraCharacterMovementComponent` → **`EnterCoverMode(WallHitResult, DistanceFromWall, MaxSpeed)`** |
| Manually set actor rotation to face away from wall | Handled inside `EnterCoverMode` |

The `Status.Cover` GAS tag set/clear stays as-is in the BP component. `Movement.Mode.Cover` is handled automatically by the CMC ↔ Lyra tag wiring.

### 6. On Exit Cover

Where the component currently exits cover:

| Before | After |
|---|---|
| Clear BP normal/tangent variables, restore movement, etc. | Call `ExitCoverMode()` on the CMC. Keep clearing `Status.Cover` tags as now. |

### 7. `MoveInCover` Custom Event

The `Add Movement Input` call stays. The improvement is the **World Direction no longer needs to come from shoulder-trace normals**:

| Before | After |
|---|---|
| `Select Vector` → `Break Vector` → X/Y from `Trace Hit Normal Right/Left` → World Direction X/Y | Pass any plausible world direction (e.g. the raw camera-forward/right from the input action). `PhysCustom` projects it onto `CoverSurfaceTangent` regardless of what direction you pass. |

The `Select Vector` / `Break Vector` / shoulder-trace-derived normal path for **movement direction** can be removed. The action value scale pin stays exactly as-is.

> The existing graph will even work untouched — `PhysCustom`'s projection fixes the direction automatically. Simplification is optional.

### 8. Lean / Edge Detection Traces

Change the **trace start point** from the shoulder socket (animation-driven) to a deterministic math position:

| Before | After |
|---|---|
| Start = Shoulder socket world location (moves with animation) | Start = `GetActorLocation() + CoverSurfaceTangent * EdgeCheckDistance` (CMC's `CoverSurfaceTangent` is `BlueprintReadOnly`) |

The trace **direction** stays the same (perpendicular to the wall). Only the start anchor changes. This means animation poses can never shift the edge detection probe.

---

## Full Change Summary Table

| File / Asset | Type | What Changes |
|---|---|---|
| `LyraGameplayTags.h` | C++ | +1 tag extern: `Movement_Mode_Cover` |
| `LyraGameplayTags.cpp` | C++ | +1 tag define; add `{0, Movement_Mode_Cover}` to `CustomMovementModeTagMap` |
| `LyraCharacterMovementComponent.h` | C++ | +5 state fields, +3 functions (`EnterCoverMode`, `ExitCoverMode`, `IsInCoverMode`), +`PhysCustom` override, update `GetMaxSpeed` comment |
| `LyraCharacterMovementComponent.cpp` | C++ | Implement all 5 new functions |
| `BP_AutoCoverComponent` | Blueprint | Call `EnterCoverMode` / `ExitCoverMode`; remove shoulder-trace direction from `MoveInCover`; fix lean trace anchor |

---

## Further Considerations

### Corner Handling

`SlideAlongSurface` in `PhysCustom` step 4 handles the character hitting a perpendicular wall (e.g. a pillar face at the end of a cover wall) gracefully. The character stops lateral movement without triggering `ExitCoverMode`. If you want the character to stop dead at corners rather than slide, replace the `SlideAlongSurface` call with a simple `Velocity = FVector::ZeroVector`.

### Curved / Multi-Face Cover Meshes

Because `PhysCustom` refreshes `CoverSurfaceNormal` and `CoverSurfaceTangent` from the live wall re-trace each step (see step 2), curved surfaces are handled automatically. The stored surface data adapts every physics tick as the character slides along the curve.

### Jump-Into-Cover Snap

On the very first frame after `EnterCoverMode`, the perpendicular snap in `PhysCustom` immediately corrects any slight misalignment from the initial hit result (e.g. the character was 2cm too far because of capsule resolution). This is the "entry clamp" the current BP system lacks.

### Rotation During Cover

`ALyraCharacter` already sets `bOrientRotationToMovement = false` and uses controller-desired rotation. Since `EnterCoverMode` sets the actor rotation once to face away from the wall, and `PhysCustom` never changes rotation, the camera can still rotate freely (for aiming/leaning) without fighting the CMC. No additional changes needed.

### `Movement.Mode.Cover` GAS Tag — Free Bonus

Because `CustomMovementModeTagMap` now maps index `0` to `Movement.Mode.Cover`, Lyra's existing `ALyraCharacter::SetMovementModeTag` code fires this tag on the ASC automatically whenever the mode is entered or exited. You can use it in:
- AnimBP `GameplayTagPropertyMap` to drive a `bIsInCover` boolean (same pattern as ShootDodge's `Status.ShootDodge` → `bIsShootDodging`)
- GAS blocking tags (e.g. block sprinting, dashing while in cover)
- Any ability's `ActivationRequiredTags` or `ActivationBlockedTags`

### Replication

`SetMovementMode` replicates automatically through the CMC. The stored surface fields (`CoverSurfaceNormal` etc.) are `Transient` and local-only — if you enable multiplayer later, you will need to replicate them via a `UPROPERTY(Replicated)` or a `Client RPC` from the server hit validation. For the current offline / single-player project configuration this is not needed.

---

## References

- `Source/LyraGame/Character/LyraCharacterMovementComponent.{h,cpp}` — all C++ changes
- `Source/LyraGame/LyraGameplayTags.{h,cpp}` — tag registration
- `Source/LyraGame/Character/LyraCharacter.cpp` — `OnMovementModeChanged` → `SetMovementModeTag` wiring (read-only reference, no changes)
- `Docs/ShootDodge_ImplementationGuide.md` — pattern reference for GameplayTagPropertyMap → AnimBP booleans
- UE5 docs: [`UCharacterMovementComponent::PhysCustom`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/GameFramework/UCharacterMovementComponent/PhysCustom)
- UE5 docs: [`UMovementComponent::SafeMoveUpdatedComponent`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/GameFramework/UMovementComponent/SafeMoveUpdatedComponent)

