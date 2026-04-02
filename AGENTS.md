# AGENTS.md (MyShooterScenarios)

## Project snapshot
- Unreal Engine **5.7** project (`MY_SHOOTER.uproject`) based on **Lyra Starter Game**.
- Main code modules: `Source/LyraGame` (runtime) and `Source/LyraEditor` (editor).
- Uses Lyra’s **GameFeature** architecture heavily (`Plugins/GameFeatures/*`).

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

## Key patterns/conventions in this repo
- **Config is authoritative** for core class selection and boot flow (`DefaultEngine.ini`, `DefaultGame.ini`). When behavior “mysteriously” differs, check config before code.
- **GameplayAbilities + GameplayCues** are central:
  - `Config/DefaultGame.ini` sets `AbilitySystemGlobalsClassName=/Script/LyraGame.LyraAbilitySystemGlobals` and `GlobalGameplayCueManagerClass=/Script/LyraGame.LyraGameplayCueManager`.
  - `LyraGameFeaturePolicy` observes GameFeature load/unload and adds cue notify paths from `UGameFeatureAction_AddGameplayCuePath` actions (see `LyraGameFeaturePolicy.cpp`).
- **Online services are enabled but default to Null in config**:
  - `Config/DefaultEngine.ini` → `[OnlineServices] DefaultServices=Null`
  - `MY_SHOOTER.uproject` enables EOS/Steam/OSS adapter plugins; don’t assume a real backend is active in local runs.
- **Networking uses Iris** (UE5 replication system):
  - `Config/DefaultEngine.ini` sets `bEnableIris=true` and includes multiple Iris configs under `/Script/Engine.Engine`.

## Build / regenerate project files (Windows)
- Canonical entry: open `MY_SHOOTER.uproject` in Unreal Editor.
- If you need to regenerate IDE files, use Unreal’s “Generate Visual Studio project files” on the `.uproject`.
- Solution file exists: `MY_SHOOTER.sln` (but `.uproject` drives UBT/UPROJECT settings).

## Working with content + maps
- Default packaged front-end content is in `/Game/System/FrontEnd/...` and staged via `MapsToCook` in `Config/DefaultGame.ini`.
- Editor starts in a GameFeature-provided map:
  - `EditorStartupMap=/MyShooterFeaturePlugin/Maps/MAP_Playground.MAP_Playground` (from `DefaultEngine.ini`).

## Useful directories to search first
- Core runtime code: `Source/LyraGame/`
- GameFeature plugins (gameplay slices): `Plugins/GameFeatures/`
- Project-wide configuration: `Config/*.ini`
- Automation/maintenance scripts: `CustomScripts/` and `clean-unreal.ps1`

## When changing/adding a GameFeature
- Update the feature’s `.uplugin` metadata/state (e.g. `BuiltInInitialFeatureState`) and keep `ExplicitlyLoaded` behavior in mind.
- If the feature introduces GameplayCues, add a `GameFeatureAction_AddGameplayCuePath` so cues are dynamically discovered when the plugin is registered (the policy observes this).

## References
- Lyra overview (high level): `Source/LyraGame/README.md`
- GameFeature policy implementation (project-specific wiring): `Source/LyraGame/GameFeatures/LyraGameFeaturePolicy.{h,cpp}`
- Config-driven boot settings: `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`

