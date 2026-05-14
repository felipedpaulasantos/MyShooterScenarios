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

── Aim constraint (covers ShootDodge → Prone → GettingUp window) ──────
Get Controller → Get Control Rotation → Break
  ├─► Yaw   → SET AimConstraintCenterYaw   (on owning Character BP)
  └─► Pitch → SET AimConstraintCenterPitch (on owning Character BP)
SET bAimConstrained = true  (on owning Character BP)
Get Player Camera Manager
  ├─► SET ViewPitchMin = AimConstraintCenterPitch − AimConstraintRange
  └─► SET ViewPitchMax = AimConstraintCenterPitch + AimConstraintRange
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

### 7. `LocomotionSM` — `ShootDodge` + `Prone` states

- Add `ShootDodge` state; wire enter transition (`bIsShootDodging`) from ground states and exit transition (`NOT bIsShootDodging`) **to `Prone`** (not to Idle).
- Add `Prone` state; wire exit transitions: `bHasMovementInput AND bProneOnBack` → `GettingUp_FromBack`, `bHasMovementInput AND NOT bProneOnBack` → `GettingUp_FromChest`.
- In Character BP **BeginPlay**, cache the mesh's `StandingMeshOffsetZ` (−88). In `GA_ShootDodge` EndAbility, call `Mesh → SetRelativeLocation(0, 0, StandingMeshOffsetZ − ProneMeshDropAmount)` to ground the prone pose. In `GA_GettingUp` EndAbility, call `SetRelativeLocation(0, 0, StandingMeshOffsetZ)` to restore it.
- Add `GettingUp_FromBack` and `GettingUp_FromChest` states; both exit to `Idle/Walk` on `Time Remaining (ratio) < 0.15`.
- Inside `ShootDodge`: `BlendSpace Player` (BS_Diving_Backwards, X = DiveStartYaw, Loop = false) → `Output Animation Pose`.
- Inside `Prone`: `BlendSpace Player` (BS_Prone_Directional, X = DiveYaw, Loop = false) → `Output Animation Pose`.

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

## Prone State (post-dive landing)

Max Payne 3–style: after the dive ends the character falls into a prone pose and stays there until the player pushes movement input.

### New AnimBP variables (add to `ABP_Mannequin_Base`, mark Thread Safe)

| Variable | Type | How it is set | Purpose |
|---|---|---|---|
| `bIsProne` | `bool` | Driven by state machine entry (or optional GAS tag) | Reserved for gameplay gating; not strictly required for animation-only prone |
| `bHasMovementInput` | `bool` | Computed each frame in Thread Safe Update | `true` when the player is actively pushing the stick/WASD — the prone exit gate |
| `bProneOnBack` | `bool` | Set once in `GA_ShootDodge` on activate (velocity dot product check) | Selects `GettingUp_FromBack` vs `GettingUp_FromChest` state |
| `bSuppressFootIK` | `bool` | Set `true` in `GA_ShootDodge` ActivateAbility; `false` in `GA_GettingUp` EndAbility | Bypasses `FullBody_SkeletalControls` IK and Control Rig foot-plant during dive/prone/get-up |

### Thread Safe Update — add this block

```
Property Access: CharacterMovement → CurrentAcceleration → VectorLength
  └─► > 0.1 → SET bHasMovementInput
```

`CurrentAcceleration` is set by UE the instant input is applied, before velocity changes — it is the cleanest "intent to move" signal. It is safe to read via Property Access on the worker thread (struct copy, not a pointer chain).

### State machine wiring

Route the `ShootDodge` exit **to `Prone`** instead of directly to `Idle`:

```
[Any ground state] → ShootDodge    rule: bIsShootDodging
[ShootDodge]       → Prone         rule: NOT bIsShootDodging   ← replaces old ShootDodge→Idle wire
[Prone]            → Idle/Walk     rule: bHasMovementInput
```

**Transition blend settings:**

| Transition | Cross-fade | Blend Logic |
|---|---|---|
| ShootDodge → Prone | `0.10 s` | Standard |
| Prone → Idle/Walk | `0.20 s` | Standard |

### `Prone` state contents

```
[Blend Space Player]
  BlendSpace = BS_Prone_Directional
  Yaw        ← GET DiveYaw   (reuse existing variable — direction relative to dive start)
  Loop       = false          ← holds last frame while prone
       │
       ▼
[Output Animation Pose]
```

Reusing `DiveYaw` aligns the prone direction with the preceding dive — no extra variables needed.

### Suppressing foot IK during prone (fixes animation distortion)

**Root cause:** The `FullBody_SkeletalControls` linked layer (and any Control Rig foot-plant node in the main AnimGraph) runs **after** `LocomotionSM`. It sees the prone feet positions, ray-casts them to the floor, and tries to plant them — mangling the pose. The `DisableLegIK` curve approach only works if the curve name exactly matches what the specific IK node reads; if not, it does nothing or activates something unrelated.

**Reliable fix: add a `bSuppressFootIK` variable that bypasses the IK layer entirely.**

#### Step 1 — New AnimBP variable

Add to `ABP_Mannequin_Base`, mark **Thread Safe**:

| Variable | Type | Default | Purpose |
|---|---|---|---|
| `bSuppressFootIK` | `bool` | `false` | When `true`, passes the pose through `FullBody_SkeletalControls` and any Control Rig foot-plant node untouched |

#### Step 2 — Drive it from the abilities

**`GA_ShootDodge` → `ActivateAbility`** (alongside existing `DiveStartYaw` set):
```
GetOwningCharacter → GetMesh → GetAnimInstance → Cast to ABP_Mannequin_Base
  └─► SET bSuppressFootIK = true
```

**`GA_GettingUp` → `EndAbility`** (alongside existing aim constraint restore):
```
GetOwningCharacter → GetMesh → GetAnimInstance → Cast to ABP_Mannequin_Base
  └─► SET bSuppressFootIK = false
```

This covers the full `ShootDodge → Prone → GettingUp` window and restores IK cleanly when locomotion resumes.

#### Step 3 — Bypass in `ABP_ItemAnimLayer_Base → FullBody_SkeletalControls`

Open `ABP_ItemAnimLayer_Base`, find the **`FullBody_SkeletalControls`** function graph. Wrap all existing nodes with a branch at the very top:

```
[Input Pose]
     │
Property Access: Outer ABP → bSuppressFootIK
  TRUE  → [Input Pose] ─────────────────────────────────────────► [Output Pose]  (full bypass)
  FALSE → [Input Pose] → [existing Two Bone IK / Control Rig] → [Output Pose]  (normal IK)
```

The `Property Access` node can reach `bSuppressFootIK` on the outer `ABP_Mannequin_Base` from inside the linked layer — right-click → **Property Access** → navigate to the variable.

#### Step 4 — Bypass the Control Rig foot-plant node (if present in the main AnimGraph)

Open `ABP_Mannequin_Base` → main **AnimGraph**. If there is a **Control Rig** node (typically `CR_Mannequin_FootPlant` or similar) after the linked layer output:

- Select the Control Rig node → find its **Alpha** input pin.
- Wire: `NOT bSuppressFootIK` → cast to float (0.0 or 1.0) → **Alpha**

Alpha = 0.0 passes the pose through unchanged. This is zero-cost when IK is suppressed.

#### Finding the IK node in your specific setup

If you're unsure whether foot IK lives in the linked layer or the main AnimGraph (or both):

1. In PIE, open the **Anim Blueprint Debugger** (select character → **Window → Anim Blueprint Debugger**).
2. Trigger prone so the distortion appears.
3. Look for active nodes involving bones like `ik_foot_l`, `ik_foot_r`, `foot_l`, `foot_r` — those are the ones to bypass.

> **Why not `DisableLegIK` curve:** Lyra 5.x foot IK is typically driven by `Enable_FootIK_R` / `Enable_FootIK_L` curves (1.0 = IK **on**, 0.0 = IK **off**) or by a Control Rig alpha pin — not `DisableLegIK`. Setting `DisableLegIK = 1.0` likely had no effect. The variable-driven bypass above is guaranteed to work regardless of curve naming.

---

### Prone mesh grounding (fixes floating above ground)

**Why `SetActorLocation` fails here:** The Character Movement Component (CMC) repositions the actor every frame to keep the capsule bottom on the floor. Any manual Z drop is overridden before the next tick. Shrinking the capsule half-height alone leaves the capsule floating at mid-standing height until the CMC catches up, producing a visible pop.

**Correct fix: shift the Skeletal Mesh component's relative Z**, not the actor. This is purely visual, bypasses the CMC entirely, and requires zero capsule changes.

**New Character Blueprint variable:**

| Variable | Type | Default | Purpose |
|---|---|---|---|
| `ProneMeshDropAmount` | `float` | `44.0` | Units to drop the mesh below its normal standing offset during prone. Tune in PIE. |

Cache the original mesh offset once in **BeginPlay** (or just hardcode `-88.0` if it never changes):
```
Get Mesh → Get Relative Location → Break → Z → SET StandingMeshOffsetZ   (stores −88)
```

**Enter prone** — add at the bottom of `GA_ShootDodge` `EndAbility` (both paths), after removing `GE_ShootDodge_Active`:

```
GetOwningCharacter → Get Mesh
  └─► Set Relative Location (X=0, Y=0, Z = StandingMeshOffsetZ − ProneMeshDropAmount)
                                      e.g.  −88 − 44 = −132
```

**Exit prone** — add at the bottom of `GA_GettingUp` `EndAbility`, alongside the existing aim constraint restore:

```
GetOwningCharacter → Get Mesh
  └─► Set Relative Location (X=0, Y=0, Z = StandingMeshOffsetZ)   ← restore to −88
```

**Tuning `ProneMeshDropAmount` in PIE:**

1. Play in editor, trigger the dive so the character enters Prone.
2. Select the character → **Details** → **Mesh** → **Relative Location Z** — scrub the value live until the character's back is flush with the floor.
3. The difference from `-88` is your `ProneMeshDropAmount`. Common range: `30–55` units.

| Value | Result |
|---|---|
| Too small | Character still visibly floats above floor |
| ~`44` (start here) | Body roughly at floor level for a standard mannequin prone pose |
| Too large | Character sinks into the floor |

> The capsule stays at standing height throughout prone. This is intentional — it keeps navigation and physics queries correct while the character is immobile on the ground. The visual discrepancy between capsule and mesh during prone is imperceptible in gameplay.

### Get-up animation — two directional states

The get-up sequence is selected based on whether the character fell on their **back** or **chest**, determined at dive activation time from the velocity direction.

#### New AnimBP variable

Add to `ABP_Mannequin_Base`, mark **Thread Safe**:

| Variable | Type | Default | Purpose |
|---|---|---|---|
| `bProneOnBack` | `bool` | `true` | Set in `GA_ShootDodge`; selects which get-up sequence to play |

#### Set `bProneOnBack` in `GA_ShootDodge` on `ActivateAbility`

Add immediately after setting `DiveStartYaw`:

```
GetOwningCharacter
  ├─► GetVelocity → Normalize  ─┐
  └─► GetActorForwardVector    ─┴─► DotProduct
                                        │
                              VectorLength(Velocity) > 10?
                                YES → dot < 0? → TRUE  = bProneOnBack = true  (moving backward)
                                               → FALSE = bProneOnBack = false (moving forward)
                                NO  → bProneOnBack = true  (default: standing-still dodge)
  └─► SET AnimBP.bProneOnBack
```

`DotProduct < 0` means the character is moving opposite to their forward vector (backing away) → lands on back. `≥ 0` = moving forward → lands on chest. The velocity-length guard prevents a zero-vector dot product from producing misleading results.

#### State machine wiring

Replace the single `GettingUp` state with two:

```
[Prone] ──(bHasMovementInput AND bProneOnBack)──────► [GettingUp_FromBack]  ──(Time Remaining < 0.15)──► [Idle/Walk]
[Prone] ──(bHasMovementInput AND NOT bProneOnBack)──► [GettingUp_FromChest] ──(Time Remaining < 0.15)──► [Idle/Walk]
```

Set **Priority Order** on both Prone exit transitions (right-click arrow → Details → Priority Order: `1` for FromBack, `2` for FromChest) — the rules are mutually exclusive but explicit priority prevents editor ambiguity warnings.

Neither `GettingUp` state has a back-transition — once entered, it always completes.

**Transition blend settings:**

| Transition | Cross-fade | Notes |
|---|---|---|
| Prone → GettingUp_FromBack | `0.10 s` | Quick cut into get-up start |
| Prone → GettingUp_FromChest | `0.10 s` | Quick cut into get-up start |
| GettingUp_FromBack → Idle/Walk | `0.20 s` | Smooth blend into locomotion |
| GettingUp_FromChest → Idle/Walk | `0.20 s` | Smooth blend into locomotion |

#### `GettingUp_FromBack` state contents

```
[Sequence Player]
  Sequence = AM_GetUp_FromBack
  Loop     = false
       │
       ▼
[Output Animation Pose]
```

#### `GettingUp_FromChest` state contents

```
[Sequence Player]
  Sequence = AM_GetUp_FromChest
  Loop     = false
       │
       ▼
[Output Animation Pose]
```

Use **Sequence Player** (not BlendSpace Player) in both — it exposes the time-remaining value the exit rule needs.

#### Exit transition rule (same for both states)

In each `GettingUp_* → Idle/Walk` transition rule graph:
1. Right-click → search **"Time Remaining (ratio)"** → UE auto-binds it to the state's sequence.
2. Wire: `Time Remaining (ratio) < 0.15` → **Result**

`0.15` fires the transition when 15% of the animation remains, giving the `0.20 s` blend a clean overlap window. Adjust to taste:

| Feel | Threshold |
|---|---|
| Very snappy | `0.05` |
| Natural overlap (default) | `0.15` |
| Early pre-blend | `0.25` |

#### Blocking movement and abilities during get-up

The state machine cannot set GAS tags directly. The bridge is: **Animation Notifies → Gameplay Events → `GA_GettingUp` ability**.

**New Gameplay Tags** (add to `Config/DefaultGameplayTags.ini`):

```ini
+GameplayTagList=(Tag="Status.GettingUp",DevComment="Active during get-up animation. Blocks movement and abilities.")
+GameplayTagList=(Tag="Event.Animation.GettingUp.Begin",DevComment="Sent by anim notify at start of get-up sequence.")
+GameplayTagList=(Tag="Event.Animation.GettingUp.End",DevComment="Sent by anim notify at end of get-up sequence.")
```

**Animation Notifies on both `AM_GetUp_FromBack` and `AM_GetUp_FromChest`:**

Add two point `AnimNotify` events in the Notifies track:

| Position | Notify | Action |
|---|---|---|
| Frame 0 | Blueprint Anim Notify | `Send Gameplay Event to Actor` → `Event.Animation.GettingUp.Begin` |
| Last frame − 2 | Blueprint Anim Notify | `Send Gameplay Event to Actor` → `Event.Animation.GettingUp.End` |

Inside each notify Blueprint:
```
Received_Notify(MeshComp, Animation)
  └─► MeshComp → Get Owner → Cast to LyraCharacter
        └─► Get Ability System Component
              └─► Send Gameplay Event to Actor
                    Actor    = owning actor
                    EventTag = Event.Animation.GettingUp.Begin  (or .End)
                    Payload  = default
```

**`GE_GettingUp_Active`** (new Blueprint Gameplay Effect):

| Property | Value |
|---|---|
| Duration Policy | `Infinite` (removed manually) |
| Granted Tags → Added | `Status.GettingUp` |
| Stacking | None |

**`GA_GettingUp`** (new Blueprint Gameplay Ability):

- **Ability Trigger:** Tag = `Event.Animation.GettingUp.Begin`, Source = `GameplayEvent`
- **Activation Owned Tags:** `Status.GettingUp`

On `ActivateAbility`:
```
Apply Gameplay Effect to Self  ──► GE_GettingUp_Active
  └─► store GettingUpEffectHandle

Wait Gameplay Event  (Tag = Event.Animation.GettingUp.End, OnlyTriggerOnce = true)
  └─► Remove Active Gameplay Effect with Handle  ──► GettingUpEffectHandle
        └─► End Ability
```

Grant `GA_GettingUp` in the same ability set as `GA_ShootDodge`.

Before the `End Ability` node in `GA_GettingUp`, also **restore aim constraints**:

```
SET bAimConstrained = false  (on owning Character BP)
Get Player Camera Manager
  ├─► SET ViewPitchMin = -89.9   (UE default)
  └─► SET ViewPitchMax =  89.9   (UE default)
```

**Block movement input** — in the Character Blueprint, listen to the ASC tag count:

```
BeginPlay → Get ASC → Add Gameplay Tag Count Changed Delegate (Status.GettingUp)
  Count > 0  → Get Player Controller → Set Ignore Move Input (true)
  Count == 0 → Get Player Controller → Set Ignore Move Input (false)
```

`SetIgnoreMoveInput` uses an internal stack counter — the `true`/`false` pair stacks safely with other systems. Do **not** use `Reset Ignore Move Input`; that clears the whole stack regardless of who pushed it.

The character will still slide to a stop under friction, gravity continues to apply, and the camera remains freely aimable — only directional steering input is suppressed.

**Aim rotation constraint** — new Character Blueprint variables:

| Variable | Type | Default | Purpose |
|---|---|---|---|
| `bAimConstrained` | `bool` | `false` | Enables per-tick yaw clamp |
| `AimConstraintCenterYaw` | `float` | `0.0` | Control yaw captured at dive start |
| `AimConstraintCenterPitch` | `float` | `0.0` | Control pitch captured at dive start |
| `AimConstraintRange` | `float` | `45.0` | Half-angle limit in degrees |

`bAimConstrained` is set to `true` in `GA_ShootDodge` on `ActivateAbility` and `false` in `GA_GettingUp` on `EndAbility`, so it covers the full `ShootDodge → Prone → GettingUp` window automatically.

Pitch is constrained via `PlayerCameraManager.ViewPitchMin/Max` (set in `GA_ShootDodge`, restored in `GA_GettingUp`) — engine-enforced, no tick needed.

Yaw is constrained by a lightweight `Event Tick` branch in the Character Blueprint:

```
[Event Tick]
  Is bAimConstrained?
    FALSE → skip (zero cost)
    TRUE  →
      Get Controller Rotation → Break → Yaw (CurrentYaw)
      Delta = NormalizeAngle(CurrentYaw − AimConstraintCenterYaw)
      Is |Delta| > AimConstraintRange?
        FALSE → no correction
        TRUE  →
          ClampedYaw = AimConstraintCenterYaw + (Sign(Delta) × AimConstraintRange)
          Get Controller → Set Control Rotation (Pitch=current, Yaw=ClampedYaw, Roll=0)
```

> `PlayerCameraManager.ViewYawMin/Max` also exists but clamps to absolute world-space values and doesn't handle the 360° seam cleanly when the reference angle is near ±180°. The per-tick correction avoids this entirely.

**Block abilities** — on every ability that must be suppressed during get-up (e.g. `GA_ShootDodge`, jump, fire), add `Status.GettingUp` to its **Activation Blocked Tags** in Class Defaults.

### Optional: `Status.Prone` Gameplay Tag

If prone needs gameplay consequences (block jumping, reduce speed, gate abilities):

1. Add to `Config/DefaultGameplayTags.ini`:
   ```ini
   +GameplayTagList=(Tag="Status.Prone",DevComment="Applied when character is in the prone state after ShootDodge landing.")
   ```
2. At the end of `GA_ShootDodge` (after removing `GE_ShootDodge_Active`), apply `GE_Prone_Active` (Infinite, grants `Status.Prone`).
3. Remove `GE_Prone_Active` when the player exits prone — via an Animation Notify at the end of the get-up animation, or via a separate `GA_ExitProne` that waits on `bHasMovementInput` using a `WaitGameplayEvent` or a tick-based check.
4. Add `Status.Prone` → `bIsProne` to the `GameplayTagPropertyMap` in `ABP_Mannequin_Base` Class Defaults.

For animation-only prone with no gameplay gating, skip this entirely — `bHasMovementInput` alone is sufficient.

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

