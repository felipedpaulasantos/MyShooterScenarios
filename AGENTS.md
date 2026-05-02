# AGENTS.md (MyShooterScenarios)

## Project snapshot
- Unreal Engine **5.7** project (`MY_SHOOTER.uproject`) based on **Lyra Starter Game**.
- Main code modules: `Source/LyraGame` (runtime) and `Source/LyraEditor` (editor).
- Uses Lyra's **GameFeature** architecture heavily (`Plugins/GameFeatures/*`).

This file is the repo playbook (where systems live, how the project boots, and common workflows). For a quick "how do I open/run this project?" entrypoint, see `README.md`.

## Authoritative sources (check these first)
When behavior differs from expectation, verify the configuration and plugin metadata before changing code:
- `Config/DefaultEngine.ini` (engine classes, maps)
- `Config/DefaultGame.ini` (Lyra systems, GameFeature policy)
- `MY_SHOOTER.uproject` (engine/plugins enabled for the project)
- Feature plugin `.uplugin` files (e.g. `Plugins/GameFeatures/MyShooterFeaturePlugin/MyShooterFeaturePlugin.uplugin`)

## How the game boots (where to look)
- Defaults are config-driven:
  - Engine classes: `Config/DefaultEngine.ini` → `GameEngine=/Script/LyraGame.LyraGameEngine`, `AssetManagerClassName=/Script/LyraGame.LyraAssetManager`.
  - Maps & entry blueprints: `Config/DefaultEngine.ini` →
    - `GlobalDefaultGameMode=/Game/B_LyraGameMode.B_LyraGameMode_C`
    - `GameInstanceClass=/Game/B_LyraGameInstance.B_LyraGameInstance_C`
    - `GameDefaultMap=/Game/UI/NiceSettingsMenu/Maps/MAP_MainMenu.MAP_MainMenu`
    - `EditorStartupMap=/MyShooterFeaturePlugin/Maps/MAP_Playground.MAP_Playground`
- GameFeature policy wires feature loading & runtime cue discovery:
  - `Source/LyraGame/GameFeatures/LyraGameFeaturePolicy.{h,cpp}` (registered in `Config/DefaultGame.ini` via `GameFeaturesManagerClassName=/Script/LyraGame.LyraGameFeaturePolicy`).

## "Where should new gameplay code/content go?"
- Prefer **GameFeature plugins** for shippable, toggleable gameplay slices:
  - Example: `Plugins/GameFeatures/MyShooterFeaturePlugin` (enabled by default, `BuiltInInitialFeatureState": "Active"`, depends on `ShooterCore`).
  - Other feature plugins: `ShooterCore`, `ShooterMaps`, `TopDownArena`, `ShooterTests`.
- Keep core/shared systems in `Source/LyraGame` (input/UI/ability system foundations).

### Practical rule of thumb
- **New weapon / ability / enemy / map slice** that can be enabled/disabled → add it to a **GameFeature** plugin.
- **Cross-cutting foundation** used by multiple features (UI framework, GAS glue, save system) → keep in `Source/LyraGame`.
- **Editor-only tooling** → `Source/LyraEditor`.

## Key patterns/conventions in this repo
- **Config is authoritative** for core class selection and boot flow (`DefaultEngine.ini`, `DefaultGame.ini`). When behavior “mysteriously” differs, check config before code.
- **GameplayAbilities + GameplayCues** are central:
  - `Config/DefaultGame.ini` sets `AbilitySystemGlobalsClassName=/Script/LyraGame.LyraAbilitySystemGlobals` and `GlobalGameplayCueManagerClass=/Script/LyraGame.LyraGameplayCueManager`.
  - `LyraGameFeaturePolicy` observes GameFeature load/unload and adds cue notify paths from `UGameFeatureAction_AddGameplayCuePath` actions (see `LyraGameFeaturePolicy.cpp`).
- **Offline single-player by default**:
  - `Config/DefaultEngine.ini` → `[OnlineServices] DefaultServices=Null`
  - `MY_SHOOTER.uproject` has multiplayer/online backend plugins disabled (EOS/Steam/OSS adapter, SteamSockets) to keep the build lean.
  - To re-enable online play/services later: flip those plugins back on in `MY_SHOOTER.uproject` and change `[OnlineServices] DefaultServices=...` (plus any subsystem-specific config).
- **Networking uses Iris** (UE5 replication system):
  - `Config/DefaultEngine.ini` sets `bEnableIris=true` and includes multiple Iris configs under `/Script/Engine.Engine`.
  - ReplicationGraph usage is currently disabled via config (`Config/DefaultGame.ini` → `[/Script/LyraGame.LyraReplicationGraphSettings] bDisableReplicationGraph=True`).
    - Note: the `ReplicationGraph` plugin itself is **enabled** in `MY_SHOOTER.uproject`. Treat this as “available, but turned off by settings” unless you intentionally re-enable it.

## Build / regenerate project files (Windows)
- Canonical entry: open `MY_SHOOTER.uproject` in Unreal Editor.
- If you need to regenerate IDE files, use Unreal’s “Generate Visual Studio project files” on the `.uproject`.
- Solution file exists: `MY_SHOOTER.sln` (but `.uproject` drives UBT/UPROJECT settings).

## Log triage (PowerShell)
- Crash logs can be huge; prefer tailing/searching instead of opening full files.
- Tail recent crash logs (Saved/Crashes/*/MY_SHOOTER.log):
  - `powershell -NoProfile -ExecutionPolicy Bypass -File CustomScripts/tail_my_shooter_logs.ps1 -TailLines 200 -MaxCrashes 3`
- Tail the *live* editor log (Saved/Logs/MY_SHOOTER.log) while the editor is running:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File CustomScripts/tail_my_shooter_live_log.ps1 -TailLines 200`

## Working with content + maps
- Default packaged front-end content is in `/Game/System/FrontEnd/...` and staged via `MapsToCook` in `Config/DefaultGame.ini`.
- Editor starts in a GameFeature-provided map:
  - `EditorStartupMap=/MyShooterFeaturePlugin/Maps/MAP_Playground.MAP_Playground` (from `DefaultEngine.ini`).

## Useful directories to search first
- Core runtime code: `Source/LyraGame/`
- GameFeature plugins (gameplay slices): `Plugins/GameFeatures/`
- Project-wide configuration: `Config/*.ini`
- Automation/maintenance scripts: `CustomScripts/` and `clean-unreal.ps1`

---

## Source/LyraGame subsystem map

All folders under `Source/LyraGame/` and what they own:

| Folder | Owns |
|---|---|
| `AbilitySystem/` | GAS foundation: `ULyraAbilitySystemComponent`, attribute sets, ability base classes, async actions, global ability system |
| `AI/` | AI controller base, perception/sensing helpers |
| `Animation/` | Anim instances, anim notify states, linked anim layers |
| `Audio/` | Audio settings, mix modifiers, Lyra audio component |
| `Camera/` | `ULyraCameraComponent`, camera modes, spring-arm overrides |
| `Character/` | `ALyraCharacter`, `ULyraPawnExtensionComponent`, `ULyraHealthComponent`, `ULyraPawnData` |
| `Cosmetics/` | Character part / cosmetic system (swappable meshes) |
| `Development/` | Cheat manager, dev settings, debug helpers |
| `Equipment/` | `ULyraEquipmentManagerComponent`, equipment definition/instance |
| `Feedback/` | Number pop, screen effects, rumble |
| `GameFeatures/` | `LyraGameFeaturePolicy`, GameFeature action implementations |
| `GameModes/` | `ALyraGameMode`, `ALyraGameState`, `ULyraExperienceManagerComponent`, `ULyraExperienceDefinition` |
| `Hotfix/` | Runtime hotfix manager |
| `Input/` | `ULyraInputComponent`, `ULyraInputConfig`, input modifier classes |
| `Interaction/` | Interaction system (interact ability, `ULyraInteractableComponent`) |
| `Inventory/` | Item definition, inventory manager component |
| `Messages/` | Gameplay message structs broadcast via `GameplayMessageRouter` |
| `Performance/` | Performance settings, machine-quality snapshot |
| `Physics/` | Physical material overrides, trace channels helpers |
| `Player/` | `ALyraPlayerController`, `ALyraPlayerState`, `ALyraLocalPlayer` |
| `Replays/` | Replay subsystem, async actions for replay playback |
| `Settings/` | `ULyraSettingsLocal`, `ULyraSettingsShared`, screen/input settings |
| `System/` | `ULyraAssetManager`, `ULyraGameInstance`, `ULyraGameData`, lifecycle helpers |
| `Teams/` | Team subsystem, team info actor, team display assets |
| `Tests/` | Gauntlet test base classes |
| `UI/` | HUD layout (`ULyraHUDLayout`), indicator system, CommonUI widget base |
| `Weapons/` | `ULyraWeaponStateComponent`, ranged weapon instance, spread helpers |

**Build file:** `Source/LyraGame/LyraGame.Build.cs`
- Add engine/plugin module dependencies to `PublicDependencyModuleNames` or `PrivateDependencyModuleNames`.
- Key modules already present: `GameplayAbilities`, `GameplayTags`, `EnhancedInput`, `CommonUI`, `CommonGame`, `Niagara`, `AIModule`, `ModularGameplay`, `GameFeatures`, `ReplicationGraph`.
- For inter-plugin C++ deps (e.g. calling into `ShooterCoreRuntime` from `LyraGame`), add to `PrivateDependencyModuleNames` and include the plugin's public header path.

---

## Lyra Experience system

Experiences are the primary composition mechanism — they describe a full gameplay mode as a data asset.

- **`ULyraExperienceDefinition`** (`Source/LyraGame/GameModes/`) — the root asset. References:
  - `DefaultPawnData` (`ULyraPawnData`) — which character class and ability set to spawn with.
  - `ActionSets` (`ULyraExperienceActionSet`) — reusable bundles of `UGameFeatureAction`s (grant abilities, add components, inject HUD widgets, etc.).
  - `Actions` — inline `UGameFeatureAction` list (same types as action sets, but specific to this experience).
- **`ULyraExperienceActionSet`** — a reusable list of `UGameFeatureAction`s shared across multiple experiences (e.g. a "core shooter HUD" set used by several maps).
- **How features plug in:** a GameFeature plugin does **not** modify the Experience asset directly. Instead it provides its own Experience (or action set) and the map's `ALyraWorldSettings` selects which Experience to load via `DefaultGameplayExperience`.
- **Where Experiences live in this project:** `Plugins/GameFeatures/MyShooterFeaturePlugin/Content/Experience/`

### Typical new-mode checklist
1. Create a `ULyraExperienceDefinition` data asset in your feature plugin's `Content/Experience/`.
2. Assign `DefaultPawnData` (or inherit from a ShooterCore pawn data).
3. Add `UGameFeatureAction_AddAbilities`, `UGameFeatureAction_AddComponents`, `UGameFeatureAction_AddWidgets` actions / action sets as needed.
4. Set `DefaultGameplayExperience` on the map's `ALyraWorldSettings` (or `B_LyraWorldSettings`) to point to your new asset.

---

## GameFeature plugin inventory

All plugins under `Plugins/GameFeatures/`:

| Plugin | State | Description |
|---|---|---|
| `MyShooterFeaturePlugin` | **Active** (always on) | Primary project plugin. Custom weapons, abilities, maps, and the main `Experience` asset for this project. Depends on `ShooterCore`. C++ module: `MyShooterFeaturePluginRuntime`. |
| `ShooterCore` | Registered (not auto-active) | Lyra's shared shooter systems: weapon framework, damage/health abilities, input configs, base pawn data. Required dependency for most other shooter plugins. C++ module: `ShooterCoreRuntime`. |
| `ShooterMaps` | Registered | Stock Lyra shooter maps (Expanse, Convolution, etc.). Content-only; no C++ module. Depends on `ShooterCore`. |
| `ShooterExplorer` | Registered | Extends `ShooterCore` with adventure/exploration elements. Content-only. Depends on `ShooterCore` + `LyraExampleContent`. |
| `ShooterTests` | Registered | Gauntlet/automation tests for shooter gameplay. C++ module: `ShooterTestsRuntime`. Depends on `ShooterCore`. |
| `TopDownArena` | Registered | Separate top-down arena game mode (Game2). C++ module: `TopDownArenaRuntime`. Uses Niagara. |
| `Meshy-ue-plugin` | Explicitly loaded | **Editor-only** Meshy AI 3-D asset bridge (Win64/Mac). No gameplay content; C++ module: `meshy`. Not a Game Feature — ignore for gameplay work. |

> **"Registered" vs "Active":** `Registered` means the plugin is known to the GameFeature subsystem but its actions are **not** applied until something activates it (e.g. loading an Experience that references it). `Active` means actions are applied at startup.

---

## Asset naming conventions

Follow Lyra/UE community prefixes consistently so AI agents and humans can infer asset type from name:

| Prefix | Asset type |
|---|---|
| `B_` | Blueprint class (Actor, Component, etc.) |
| `W_` | Widget Blueprint (UMG) |
| `MAP_` | Level / Map |
| `GA_` | Gameplay Ability (`UGameplayAbility`) |
| `GE_` | Gameplay Effect (`UGameplayEffect`) |
| `GC_` | Gameplay Cue Notify |
| `GAS_` or `AS_` | Gameplay Ability Set |
| `GT_` | Gameplay Tag table |
| `ID_` | Input Action (`UInputAction`) |
| `IMC_` | Input Mapping Context (`UInputMappingContext`) |
| `EQS_` | Environment Query |
| `BT_` | Behaviour Tree |
| `BB_` | Blackboard |
| `DT_` | Data Table |
| `DA_` | Data Asset (generic) |
| `T_` | Texture |
| `M_` | Material |
| `MI_` | Material Instance |
| `MF_` | Material Function |
| `SM_` | Static Mesh |
| `SK_` | Skeletal Mesh |
| `ABP_` | Animation Blueprint |
| `AM_` | Animation Montage |
| `NS_` | Niagara System |
| `NE_` | Niagara Emitter |
| `S_` | Sound Wave |
| `SC_` | Sound Cue |

---

## Common workflows

### Add a new GameFeature plugin (gameplay slice)
Prefer creating a new plugin under `Plugins/GameFeatures/` when the feature is meant to be toggleable.

Checklist:
1. Create the plugin (Editor: **Plugins** window → **New Plugin** → **Game Feature (Content Only)** or **Game Feature (C++)**).
2. Ensure the plugin’s `.uplugin` metadata matches intent:
   - `BuiltInInitialFeatureState` (e.g. `Active` for always-on in this project)
   - `ExplicitlyLoaded` (affects whether it auto-registers)
3. Add dependencies (e.g. `ShooterCore`) if required.

### Add GameplayCues in a feature
If a feature adds cue notifies, it must also register a cue path so Lyra can discover them dynamically when the feature loads.

Checklist:
- Add a `UGameFeatureAction_AddGameplayCuePath` to the feature’s Game Feature Data.
- Verify the cue notify asset path is inside the registered directory.
- Confirm `LyraGameFeaturePolicy` logs cue-path registration when activating the feature.

### Add maps provided by a feature
- Put maps under the feature plugin’s `Content/Maps/` (or an equivalent folder).
- If the map should be cooked/packaged, ensure it’s included via project settings / `MapsToCook` (project-specific; check `Config/DefaultGame.ini`).

## Debugging quick hits
- Live log: `Saved/Logs/MY_SHOOTER.log`
- Crash logs: `Saved/Crashes/*/MY_SHOOTER.log`
- Prefer tail scripts in `CustomScripts/` to avoid opening huge logs.

## When changing/adding a GameFeature
- Update the feature’s `.uplugin` metadata/state (e.g. `BuiltInInitialFeatureState`) and keep `ExplicitlyLoaded` behavior in mind.
- If the feature introduces GameplayCues, add a `GameFeatureAction_AddGameplayCuePath` so cues are dynamically discovered when the plugin is registered (the policy observes this).

## References
- GameFeature policy implementation (project-specific wiring): `Source/LyraGame/GameFeatures/LyraGameFeaturePolicy.{h,cpp}`
- Experience system classes: `Source/LyraGame/GameModes/LyraExperienceDefinition.{h,cpp}`, `LyraExperienceActionSet.{h,cpp}`, `LyraExperienceManagerComponent.{h,cpp}`
- Module build rules: `Source/LyraGame/LyraGame.Build.cs`
- Config-driven boot settings: `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`

## MYST Inventory / Weapon-Bar system (custom)

All code lives in `Plugins/GameFeatures/MyShooterFeaturePlugin/Source/MyShooterFeaturePluginRuntime/`.

| Phase | What was built | Key files |
|---|---|---|
| 1 | Core item fragments | `Public/Inventory/InventoryFragment_ItemCategory.h`, `InventoryFragment_UsableItem.h`, `InventoryFragment_WeaponSlotType.h` |
| 2 | Inventory attribute set (capacity attributes) | `Public/AbilitySystem/MYSTInventoryAttributeSet.h` |
| 3 | Capacity enforcement + use-item ability | `Public/Inventory/MYSTInventoryCapacityComponent.h`, `Public/AbilitySystem/Abilities/GA_UseItem.h` |
| 4 | Weapon bar (fixed + pickup slots) | `Public/Inventory/MYSTWeaponBarComponent.h` |
| 5 | Blueprint function library | `Public/Inventory/MYSTInventoryFunctionLibrary.h` |

**Message tags** (broadcast via `UGameplayMessageSubsystem`):
- `MYST.WeaponBar.Message.SlotsChanged` → payload `FMYSTWeaponBarSlotsChangedMessage`
- `MYST.WeaponBar.Message.ActiveSlotChanged` → payload `FMYSTWeaponBarActiveSlotChangedMessage`

**Gameplay Tags** registered in `Config/DefaultGameplayTags.ini` and natively in `Public/MYSTGameplayTags.h`.

**Implementation how-to:** `Docs/InventorySystem_ImplementationGuide.md`

### Notes for future changes
- `ULyraEquipmentManagerComponent` and `ULyraEquipmentInstance` in `Source/LyraGame/Equipment/` were patched with `LYRAGAME_API` so the plugin can link against them. Do not remove these.
- `GameplayMessageRuntime` was added to `PrivateDependencyModuleNames` in the plugin's `Build.cs`.

---

## MYST Level Loading system (custom)

Single-player level loading flow that does **not** depend on Lyra's Experience system or online services.

**Key file:** `Public/LevelLoading/MYSTLevelLoadingSubsystem.h` / `Private/LevelLoading/MYSTLevelLoadingSubsystem.cpp`

### How it works

`UMYSTLevelLoadingSubsystem` is a `UGameInstanceSubsystem` that:
1. Implements `ILoadingProcessInterface` and registers itself with `ULoadingScreenManager` (from `CommonLoadingScreen` plugin).
2. Shows the loading screen immediately when `RequestLevelTravel()` is called.
3. Advances through a four-phase state machine, keeping the loading screen up until all gates clear:

| Phase | Gate to advance |
|---|---|
| `WaitingForMap` | `FCoreUObjectDelegates::PostLoadMapWithWorld` fires |
| `WaitingForStreaming` | All `ULevelStreaming` objects are loaded+visible (covers World Partition cells too) |
| `WaitingForPSO` | `FShaderPipelineCache::NumPrecompilesRemaining() == 0` + `MinHoldAfterCompleteSeconds` |
| `Idle` | Loading screen is hidden, gameplay begins |

### Blueprint usage

```
GetGameInstance → Get Subsystem (MYSTLevelLoadingSubsystem)
  → Bind OnLoadingComplete (optional)
  → Bind OnShaderProgressUpdated → drive progress bar widget
  → Call RequestLevelTravel("/Game/Maps/L_MyLevel", true)
```

### Blueprint-assignable events

| Delegate | When |
|---|---|
| `OnLoadingStarted` | Right before `OpenLevel` is called |
| `OnStreamingComplete` | All streaming levels/WP cells are visible |
| `OnShadersComplete` | PSO precompile drained |
| `OnShaderProgressUpdated(float, int32)` | Every tick during WaitingForPSO phase |
| `OnLoadingComplete` | Loading screen about to hide — safe to start gameplay |

### Tunable properties (set on the subsystem CDO or in BP)

| Property | Default | Purpose |
|---|---|---|
| `PhaseTimeoutSeconds` | 30 s | Force-advances stuck phase and logs a warning |
| `MinHoldAfterCompleteSeconds` | 0.25 s | Minimum hold after all gates clear (texture streaming catch-up) |

### Build.cs additions
- `CommonLoadingScreen` → `ILoadingProcessInterface`, `ULoadingScreenManager`
- `RenderCore` → `FShaderPipelineCache`

