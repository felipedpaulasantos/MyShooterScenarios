# MyShooterScenarios ("Surrogate")

Unreal Engine **5.7** project based on **Lyra Starter Game**, using Lyra’s **GameFeature** architecture (`Plugins/GameFeatures/*`).

- High-level game pitch: `Docs/GamePitch.md`
- Working in this repo (boot flow, where code lives, common workflows): `AGENTS.md`

## Requirements
- Unreal Engine **5.7** installed (via Epic Launcher or source build).
- Windows + Visual Studio toolchain suitable for UE 5.7 (MSVC + Windows SDK).

## Open the project
1. Double-click `MY_SHOOTER.uproject` (or open it from the Unreal Project Browser).
2. If prompted to rebuild modules, choose **Yes**.

> Tip: The `.uproject` is the canonical entrypoint; the `.sln` is generated/maintained by UnrealBuildTool.

## Build notes
- Regenerate IDE files: right-click `MY_SHOOTER.uproject` → **Generate Visual Studio project files**.
- Clean common build artifacts:
  - `clean-unreal.ps1`

## Running / maps
This project is config-driven (see `Config/*.ini`). Notable defaults:
- Default game map: `Config/DefaultEngine.ini` → `GameDefaultMap=/Game/UI/NiceSettingsMenu/Maps/MAP_MainMenu.MAP_MainMenu`
- Editor startup map: `Config/DefaultEngine.ini` → `EditorStartupMap=/MyShooterFeaturePlugin/Maps/MAP_Playground.MAP_Playground`

## Repo structure (quick map)
- Runtime code: `Source/LyraGame/`
- Editor code: `Source/LyraEditor/`
- GameFeature plugins (gameplay slices): `Plugins/GameFeatures/*`
- Project config: `Config/*.ini`
- Utility scripts: `CustomScripts/`

## Logs
- Live editor log: `Saved/Logs/MY_SHOOTER.log`
- Crash logs: `Saved/Crashes/*/MY_SHOOTER.log`

Convenience scripts:
- `CustomScripts/tail_my_shooter_live_log.ps1`
- `CustomScripts/tail_my_shooter_logs.ps1`
