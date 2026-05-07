# MYST Shoot Dodge — Implementation Guide

> **Covers:** Gameplay Ability setup · GAS tag flow · AnimBP integration · BlendSpace configuration · State machine wiring · Armed/unarmed cases  
> **Assets involved:** `GA_ShootDodge` (BP) · `GE_ShootDodge_Active` (BP) · `BS_Diving_Backwards` · `ABP_Mannequin_Base` · `ABP_ItemAnimLayer_Base`

---

## System Overview

The Shoot Dodge is a Max Payne–style dive mechanic: the player activates `GA_ShootDodge`, the character launches into a directional backwards dive, and the upper body continues to aim and shoot freely throughout. Animation direction is controlled by a BlendSpace driven by the yaw angle at the moment the dive started.

```
GA_ShootDodge (Blueprint Gameplay Ability)
  └─► Apply GE_ShootDodge_Active
        └─► Grants tag  Status.ShootDodge
                  │
                  │  FGameplayTagBlueprintPropertyMap (ULyraAnimInstance)
                  ▼
        bIsShootDodging = true  (AnimBP bool variable)
                  │
                  │  LocomotionSM transition rule
                  ▼
        ShootDodge state  ──► BS_Diving_Backwards (DiveStartYaw)  ──► Output Pose
                                                                           │
                                                                (FullBody_Aiming linked layer
                                                                 applies upper-body weapon aim
                                                                 on top — automatic when armed)
```

---

## Gameplay Tags

| Tag | Registered in | Purpose |
|---|---|---|
| `Status.ShootDodge` | `Config/DefaultGameplayTags.ini` | Active while the dive is in progress; drives `bIsShootDodging` in the AnimBP via `GameplayTagPropertyMap` |
| `Ability.Type.Action.Dive` | `Config/DefaultGameplayTags.ini` | Ability classification tag; set on `GA_ShootDodge` |
| `InputTag.Ability.Dive` | `Config/DefaultGameplayTags.ini` | Input binding tag for the dive ability |

---

## Asset Inventory

| Asset | Type | Location | Role |
|---|---|---|---|
| `GA_ShootDodge` | Blueprint Gameplay Ability | `MyShooterFeaturePlugin/Content/Abilities/` | Activates the dive; applies/removes `GE_ShootDodge_Active`; sets `DiveStartYaw` on the AnimBP |
| `GE_ShootDodge_Active` | Blueprint Gameplay Effect | `MyShooterFeaturePlugin/Content/Abilities/` | Grants `Status.ShootDodge` for the dive duration |
| `BS_Diving_Backwards` | Blend Space | `MyShooterFeaturePlugin/Content/Animations/` | Full-body dive BlendSpace; Yaw axis = dive direction offset |
| `ABP_Mannequin_Base` | Animation Blueprint | `ShooterCore/Content/Characters/` | Main character AnimBP; hosts the `LocomotionSM` and `GameplayTagPropertyMap` binding |
| `ABP_ItemAnimLayer_Base` | Animation Blueprint | `ShooterCore/Content/Characters/` | Weapon linked layer; `FullBody_Aiming` applies upper-body aim offset (armed case) |

---

## GAS Flow

### `GE_ShootDodge_Active`

| Property | Value |
|---|---|
| **Duration Policy** | `Has Duration` (match the ability's dive window, e.g. `0.8 s`) or `Infinite` if ended manually |
| **Granted Tags → Added** | `Status.ShootDodge` |
| **Stacking** | None |

### `GA_ShootDodge` — key Blueprint nodes

**On `ActivateAbility`:**

```
Apply Gameplay Effect to Owner  ──► GE_ShootDodge_Active
  └─► store ActiveEffectHandle

GetOwningCharacter
  └─► GetMesh → GetAnimInstance → Cast to ABP_Mannequin_Base
        └─► SET DiveStartYaw = GetActorRotation.Yaw
```

**On `EndAbility`** (both committed and cancelled paths):

```
Remove Active Gameplay Effect with Handle  ──► ActiveEffectHandle
```

> **Why store the handle:** `GE_ShootDodge_Active` may have a fixed duration, but if the ability ends early (mid-air fall, death, interrupt), removing the GE explicitly keeps the `Status.ShootDodge` tag from lingering.

---

## AnimBP Variables

All variables live on `ABP_Mannequin_Base`. Mark every new variable **Thread Safe** so it can be read inside `Blueprint Thread Safe Update Animation`.

| Variable | Type | How it is set | Purpose |
|---|---|---|---|
| `bIsShootDodging` | `bool` | `GameplayTagPropertyMap` — auto (see below) | Gates the `ShootDodge` state machine transition |
| `DiveStartYaw` | `float` | Set once from `GA_ShootDodge` on ability activate | Yaw of the actor at dive start; BlendSpace reference direction |
| `DiveYaw` | `float` | Computed each frame in Thread Safe Update | `ControlYaw − DiveStartYaw` (normalized); BlendSpace X input |
| `DiveAimYaw` | `float` | Computed each frame in Thread Safe Update | `ControlYaw − ActorYaw` (normalized); Aim Offset X input when needed |
| `DiveAimPitch` | `float` | Computed each frame in Thread Safe Update | `ControlPitch` clamped `−90..90`; Aim Offset Y input when needed |
| `CachedControlRotation` | `Rotator` | Set each frame in `Event Blueprint Update Animation` (game thread) | Thread-safe proxy for `GetControlRotation()` — property access to the controller can return zero on the worker thread |

---

## `GameplayTagPropertyMap` Binding

Lyra's `ULyraAnimInstance` provides `FGameplayTagBlueprintPropertyMap`. When the ASC grants or removes a tag, the mapped AnimBP property is updated automatically — no Blueprint code required.

**How to configure (in `ABP_Mannequin_Base` Class Defaults):**

1. Select the AnimBP in the editor, open **Class Defaults**.
2. Find **Gameplay Tag Property Map** (inherited from `ULyraAnimInstance`).
3. Add one entry:

| Field | Value |
|---|---|
| **Tag** | `Status.ShootDodge` |
| **Property** | `bIsShootDodging` |
| **Property Type** | `Bool` |

`ULyraAnimInstance::InitializeWithAbilitySystem` wires this up at begin-play; the bool updates the moment the tag is added or removed.

---

## AnimBP Update Functions

### `Event Blueprint Update Animation` (game thread)

Used **only** to cache values that cannot be safely read on the worker thread.

```
[Event Blueprint Update Animation]
  Try Get Pawn Owner → Is Valid?
    └─► Get Controller → Get Control Rotation → SET CachedControlRotation
```

### `Blueprint Thread Safe Update Animation` (worker thread)

Compute dive variables here. Use `GET CachedControlRotation` (the cached value from above) — do **not** use a Property Access node for `Control Rotation` directly; the controller chain can return `(0,0,0)` on the worker thread.

```
── DiveYaw (BlendSpace direction relative to dive start) ──────────────
GET CachedControlRotation → Break → Yaw  ─┐
                                            ├─► Subtract → Normalize Angle → SET DiveYaw
GET DiveStartYaw                          ─┘

── DiveAimYaw / DiveAimPitch (used if an explicit Aim Offset is added) ─
GET CachedControlRotation → Break → Yaw  ─┐
                                            ├─► Subtract → Normalize Angle → SET DiveAimYaw
Property Access: Actor Rotation → Yaw    ─┘

GET CachedControlRotation → Break → Pitch → Clamp(−90, 90) → SET DiveAimPitch
```

> **FInterpTo smoothing (optional):** Replace the direct `SET DiveYaw` with `FInterp To(DiveYaw, TargetYaw, GetDeltaSeconds, 6.0)` for a smoother BlendSpace traversal. A speed of `6.0` is responsive; lower values add cinematic lag.

---

## BlendSpace Configuration (`BS_Diving_Backwards`)

| Axis | Role | Recommended range |
|---|---|---|
| **Horizontal (Yaw)** | Dive direction relative to the character's facing at dive start | `−180 → 180` |
| **Vertical (Pitch)** | Vertical arc / aim elevation (optional) | `−90 → 90` |

**Interpolation:** Set both axes' **Interpolation Type** to `Averaged` and **Interpolation Time** to `0.0`. Drive smoothing from the `DiveYaw` variable in the AnimBP (`FInterpTo`) rather than from the asset, so the speed is controllable without reopening the BlendSpace.

**Animation samples:** Place full-body "diving backwards" poses at the cardinal yaw points (`−90`, `0`, `+90` at minimum; add `±45` for smoother blending). The center sample (`0`) is what plays when the camera faces the same direction as the dive.

---

## `LocomotionSM` — ShootDodge State

Open `ABP_Mannequin_Base` → **AnimGraph** → double-click **`LocomotionSM`**.

### Adding the state

1. Right-click on empty canvas → **Add State** → name it `ShootDodge`.

### Transition wires

**Hover** over the **edge** of an existing ground state (e.g. `Idle`) until a small white arrow appears, then **left-click-drag** to `ShootDodge`. Repeat in reverse (ShootDodge → Idle).

| Transition | Rule node |
|---|---|
| Any ground state → `ShootDodge` | `GET bIsShootDodging` → Result |
| `ShootDodge` → Idle/Walk | `NOT bIsShootDodging` → Result |

**Transition blend settings (recommended):**

| | Enter | Exit |
|---|---|---|
| Cross-fade duration | `0.15 s` | `0.10 s` |
| Blend Logic | Standard Blend | Standard Blend |

### State contents (final working setup)

Double-click the `ShootDodge` state. The graph is intentionally minimal:

```
[Blend Space Player]
  BlendSpace = BS_Diving_Backwards
  Yaw        ← GET DiveStartYaw
  Pitch      ← 0.0  (or GET DivePitch if vertical arcing is needed)
       │
       ▼
[Output Animation Pose]
```

> **Why no Layered Blend Per Bone here:** `bUseControllerRotationYaw = true` is left enabled, so the character mesh naturally rotates to face the camera. The `FullBody_Aiming` linked layer (from the equipped weapon) applies upper-body aim on top of whatever `LocomotionSM` outputs. The ShootDodge state only needs to supply the correct full-body dive pose; the existing aiming pipeline handles the rest.

---

## Main AnimGraph — Node Order (read-only)

The main AnimGraph in `ABP_Mannequin_Base` already chains the linked layers in the correct order. No changes are needed here:

```
LocomotionSM (ShootDodge state outputs dive pose)
     ↓
ALI_ItemAnimLayers – FullBody_Aiming       ← weapon upper-body aim
     ↓
ALI_ItemAnimLayers – FullBodyAdditives     ← recoil, additive FX
     ↓
ALI_ItemAnimLayers – FullBody_SkeletalControls  ← IK
     ↓
ALI_ItemAnimLayers – LeftHandPose_OverrideState ← left-hand grip
     ↓
Output Pose
```

Because the dive state lives inside `LocomotionSM`, it feeds the base pose into this chain. The upper-body aim applies on top for free.

---

## Unarmed vs. Armed Cases

### Unarmed (no weapon equipped)

`ABP_Mannequin_Base::FullBody_Aiming` has **no implementation** (Input Pose is not connected to Output Pose in the base ABP). The pose from `LocomotionSM` passes through all linked layer slots unchanged. The BlendSpace provides the complete visible result.

No additional setup needed for this case.

### Armed (weapon equipped)

The weapon's `ABP_ItemAnimLayer_Base::FullBody_Aiming` replaces the empty base implementation:

```
Input Pose → AimOffset (AimYaw, AimPitch) → Output Pose
```

This applies the weapon's upper-body aim offset on top of the dive pose — which is the desired Max Payne behaviour (legs diving, arms tracking the target). **No changes to the item layer are needed for standard use.**

**If the weapon layer fights with the dive pose** (e.g. overrides the full body instead of a layered blend), add a pass-through branch at the top of `ABP_ItemAnimLayer_Base::FullBody_Aiming`:

```
GET bIsShootDodging
  TRUE  → Input Pose ─────────────────────────► Output Pose   (pass-through)
  FALSE → Input Pose → AimOffset (Yaw, Pitch) → Output Pose   (normal weapon aim)
```

---

## Rotation Behaviour During the Dive

`bUseControllerRotationYaw` is left **`true`** throughout the dive. The character mesh continues to rotate to face the camera direction, which drives natural upper-body aim tracking through the existing `FullBody_Aiming` pipeline.

The `DiveStartYaw` variable captures the actor's yaw **at the moment of activation**. `DiveYaw = NormalizeAngle(ControlYaw − DiveStartYaw)` then represents how far the camera has moved from the dive's starting direction, which is fed to the BlendSpace to select the correct dive pose.

> **Alternative (locked-body style):** If you want the body fully frozen in the dive direction (stricter Max Payne flight arcade feel), set `bUseControllerRotationYaw = false` and `bOrientRotationToMovement = false` in the ability, then drive upper-body aiming via an explicit `Layered Blend Per Bone` (split at `spine_01`) + `Aim Offset` inside the `ShootDodge` state using `DiveAimYaw`/`DiveAimPitch`. Restore both flags in `EndAbility`. This approach requires more setup and was explored but not used as the default.

---

## Setup Checklist

### 1. Gameplay Tag

Confirm `Status.ShootDodge` is present in `Config/DefaultGameplayTags.ini`:

```ini
+GameplayTagList=(Tag="Status.ShootDodge",DevComment="Applied by GE_ShootDodge_Active for the duration of GA_ShootDodge. Drives bIsShootDodging in the AnimBP via GameplayTagPropertyMap.")
```

### 2. `GE_ShootDodge_Active`

- Duration Policy: `Has Duration` (set to dive window length) or `Infinite`.
- Granted Tags → Added: `Status.ShootDodge`.

### 3. `GA_ShootDodge`

- On `ActivateAbility`: apply `GE_ShootDodge_Active`, store the handle, set `DiveStartYaw` on the AnimBP.
- On `EndAbility` (both paths): remove the GE by handle.

### 4. `ABP_Mannequin_Base` — variables

Add and mark **Thread Safe**:

| Variable | Type |
|---|---|
| `bIsShootDodging` | `bool` |
| `DiveStartYaw` | `float` |
| `DiveYaw` | `float` |
| `DiveAimYaw` | `float` |
| `DiveAimPitch` | `float` |
| `CachedControlRotation` | `Rotator` |

### 5. `ABP_Mannequin_Base` — `GameplayTagPropertyMap`

In **Class Defaults**: add entry `Status.ShootDodge` → `bIsShootDodging` (Bool).

### 6. `ABP_Mannequin_Base` — update functions

- **`Event Blueprint Update Animation`** (game thread): cache `GetControlRotation()` → `CachedControlRotation`.
- **`Blueprint Thread Safe Update Animation`** (worker thread): compute `DiveYaw`, `DiveAimYaw`, `DiveAimPitch` from `CachedControlRotation`.

### 7. `LocomotionSM` — `ShootDodge` state

- Add state, wire enter/exit transitions on `bIsShootDodging`.
- Inside state: `BlendSpace Player` (BS_Diving_Backwards, X = DiveStartYaw) → `Output Animation Pose`.

### 8. `BS_Diving_Backwards`

- Horizontal axis: Yaw `−180..180`. Vertical axis: Pitch `−90..90` (optional).
- Interpolation Time: `0.0` on both axes (smoothing handled by `FInterpTo` in the AnimBP).

---

## Debugging

### Variables not updating (AimYaw / DiveYaw stuck at 0)

**Symptom:** `DiveYaw` is always `0` when looking around.  
**Cause:** Property Access to `Control Rotation` on the worker thread returns `(0,0,0)` if the controller chain is not properly cached.  
**Fix:** Use `CachedControlRotation` (set in `Event Blueprint Update Animation` on the game thread) instead of any Property Access node for `Control Rotation` in the thread-safe function.

### `DiveStartYaw` is always 0

**Symptom:** BlendSpace always samples the center regardless of facing direction.  
**Cause:** The `Cast to ABP_Mannequin_Base` in `GA_ShootDodge` is failing silently; `DiveStartYaw` is never written and stays at its default `0.0`.  
**Fix:** Add a `Print String` immediately after the cast node — if nothing prints, confirm the mesh's **Anim Class** in the Character Blueprint matches the class you are casting to.

### Whole body rotates with camera during dive

**Symptom:** The character mesh spins to face the camera, overriding the dive animation.  
**Cause:** `bUseControllerRotationYaw = true` (expected) combined with a high rotation rate. The mesh is legitimately following the camera — this is intentional in the final setup.  
**If undesired:** Lower `CharacterMovement → Rotation Rate → Yaw` (e.g. `180°/s`) to add a Max Payne–style lag between camera and body.

### State transition never fires (never enters `ShootDodge` state)

**Checklist:**
1. Confirm `GE_ShootDodge_Active` successfully grants `Status.ShootDodge` — use **Gameplay Debugger** → `AbilitySystem` tab (`` ` `` key in PIE), check **Active Tags** on the player.
2. Confirm the `GameplayTagPropertyMap` entry exists in `ABP_Mannequin_Base` Class Defaults.
3. Open the AnimBP debugger (PIE → select the character → **Window → Anim Blueprint Debugger**), watch `bIsShootDodging` flip when the ability activates.

### Upper body does not aim during the dive (unarmed)

This is expected. `ABP_Mannequin_Base::FullBody_Aiming` is empty by design; when unarmed there is no item layer override. The BlendSpace provides the full-body pose. Equip a weapon to see the aim layer activate.

---

## Architecture Notes

### Why `DiveStartYaw` and not a live `ControlYaw − ActorYaw`

With `bUseControllerRotationYaw = true`, the actor yaw tracks the camera — the live difference between them is always near `0`. `DiveStartYaw` snapshots the actor's direction at the moment of activation, giving a stable reference that drifts meaningfully from the camera yaw as the player aims around during flight.

### Why `GameplayTagPropertyMap` instead of a Blueprint event

`FGameplayTagBlueprintPropertyMap` is Lyra's idiomatic way to bridge GAS and the AnimBP. It is latency-free (updated the same frame the ASC tag changes), requires no polling, and keeps GAS concerns out of the animation update path.

### Why the BlendSpace lives in `LocomotionSM` and not in a separate montage slot

Placing it in `LocomotionSM` means the existing linked layer chain (`FullBody_Aiming`, `FullBodyAdditives`, `FullBody_SkeletalControls`, `LeftHandPose_OverrideState`) applies after it automatically — upper-body weapon aim, IK, and left-hand grip work for free. A montage on a slot would require manually overriding or suppressing those layers.

---

## Extending the System

### Add a directional dodge variant (forward, sideways)

Create additional BlendSpaces (e.g. `BS_Diving_Forward`, `BS_Diving_Left`) and swap which asset the `Blend Space Player` node references using a **Set Blend Space** node driven by the direction of player input at activation time. Cache the velocity direction normalized relative to the actor forward vector in `GA_ShootDodge` and map it to the asset selector.

### Add slow-motion during the dive

In `GA_ShootDodge`, on `ActivateAbility`, call `UGameplayMessageSubsystem::BroadcastMessage` with tag `Music.Context.GameplayAction` and a message requesting the slow-motion ability (`Ability.Type.Action.SlowDownTime`). Both tags already exist in `DefaultGameplayTags.ini`.

### Add a GameplayCue for dive VFX / audio

1. Create a `GC_Character_ShootDodge` GameplayCue Notify asset in `MyShooterFeaturePlugin/Content/GameplayCues/`.
2. Add `GameplayCue.Character.ShootDodge` to `DefaultGameplayTags.ini`.
3. In `GA_ShootDodge`, call **Execute Gameplay Cue** on activation.
4. Add a `UGameFeatureAction_AddGameplayCuePath` entry to `MyShooterFeaturePlugin`'s Game Feature Data so Lyra discovers the cue when the plugin activates (see `LyraGameFeaturePolicy`).

