# AGENTS.md (MyShooterScenarios)

## Project snapshot
- Unreal Engine **5.7** project (`MY_SHOOTER.uproject`) based on **Lyra Starter Game**.
- Main code modules: `Source/LyraGame` (runtime) and `Source/LyraEditor` (editor).
- Uses Lyra’s **GameFeature** architecture heavily (`Plugins/GameFeatures/*`).

This file is the repo playbook (where systems live, how the project boots, and common workflows). For a quick “how do I open/run this project?” entrypoint, see `README.md`.

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

## “Where should new gameplay code/content go?”
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
- Lyra overview (high level): `Source/LyraGame/README.md`
- GameFeature policy implementation (project-specific wiring): `Source/LyraGame/GameFeatures/LyraGameFeaturePolicy.{h,cpp}`
- Config-driven boot settings: `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`

