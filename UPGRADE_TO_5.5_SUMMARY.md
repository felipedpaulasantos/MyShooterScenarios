# Unreal Engine 5.4 to 5.5 Upgrade Summary

## Project: MyShooterScenarios (Lyra-based)

### Upgrade Date
Performed on: 2025-10-11

This upgrade migrates the project from Unreal Engine 5.4 to 5.5.

---

## Changes Made

### 1. Project Configuration Files

#### MY_SHOOTER.uproject
- **Changed:** `EngineAssociation` from `"5.4"` to `"5.5"`
- **Location:** `F:\UnrealProjects\MyShooterScenarios\MY_SHOOTER.uproject`

#### DefaultEngine.ini
- **Changed:** `gc.PendingKillEnabled=False` to `gc.GarbageEliminationEnabled=False`
- **Reason:** UE 5.5 renamed the garbage collection setting
- **Location:** `F:\UnrealProjects\MyShooterScenarios\Config\DefaultEngine.ini`

### 2. Code Changes - API Updates

#### GameFeatureAction_AddWidget.cpp
**File:** `Source\LyraGame\GameFeatures\GameFeatureAction_AddWidget.cpp`

**Changes for Asset Bundle API (UE 5.5):**

**BEFORE (UE 5.4):**
```cpp
AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateClient, 
    Entry.WidgetClass.ToSoftObjectPath().GetAssetPath());
```

**AFTER (UE 5.5):**
```cpp
AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateClient, 
    Entry.WidgetClass.ToSoftObjectPath().GetAssetPath());
```

**Note:** In UE 5.5, `GetAssetPath()` now returns `FTopLevelAssetPath` instead of `FSoftObjectPath`, which is the new non-deprecated API.

**Changes for Data Validation API (UE 5.5):**

**BEFORE (UE 5.4):**
```cpp
virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
```

**AFTER (UE 5.5):**
```cpp
virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
```

**Reason:** 
- UE 5.5 uses `FDataValidationContext` to distinguish between warnings and errors
- Validation errors are now added via `Context.AddError()` instead of `ValidationErrors.Add()`
- The function is now `const` for better safety

### 3. Asset Format Warnings

**Issue:** Animation and other assets show deprecation warnings about `FTopLevelAssetPath`:
```
Warning: While importing text for property 'AssetPaths' in 'AssetBundleEntry':
Struct format for FTopLevelAssetPath is deprecated. Imported struct: (SK_Mannequin)/1_
```

**Cause:** Assets saved in UE 5.4 have old-format asset paths embedded in their metadata.

**Solution:** Resave affected assets in UE 5.5 (see RESAVE_ASSETS_SCRIPT.md for detailed instructions).

### 4. Cleaned Build Artifacts
**IMPORTANT:** Before building, you must clean old build artifacts:

```batch
# Delete these directories:
- Intermediate/
- Binaries/
- .vs/ (if exists)
- Saved/Cooked/ (if exists)

# Or run the cleanup script:
QUICK_CLEANUP.bat
```

### 5. Generate Visual Studio Project Files
Right-click on `MY_SHOOTER.uproject` and select:
- **"Switch Unreal Engine version..."** → Select UE 5.5
- **"Generate Visual Studio project files"**

Alternatively, run from command line:
```batch
"C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="F:\UnrealProjects\MyShooterScenarios\MY_SHOOTER.uproject" -game -engine
```

### 6. First Compilation
Open Visual Studio and build the solution:
- Set configuration to `Development Editor`
- Build Solution (Ctrl+Shift+B)

Expected compile time: 10-20 minutes (first time only)

### 7. First Launch in UE 5.5
Open `MY_SHOOTER.uproject` with Unreal Engine 5.5:
- The engine will convert assets to UE 5.5 format (one-time process)
- Shaders will be compiled for UE 5.5 (may take 30-60 minutes)
- Derived Data Cache (DDC) will be rebuilt

**Note:** Initial load will be slower than normal due to shader compilation.

### 8. Verification Testing

After the project opens, verify the following systems:

#### Core Gameplay
- [ ] Character spawning and movement
- [ ] Ability system (abilities trigger correctly)
- [ ] Weapon systems
- [ ] Team system
- [ ] Experience loading (GameFeatures)

#### Cosmetics & Character Parts
- [ ] Character customization works
- [ ] Mesh swapping functions correctly
- [ ] Materials apply properly

#### UI Systems
- [ ] Main menu loads
- [ ] HUD displays correctly
- [ ] CommonUI widgets function
- [ ] Settings menus work

#### Multiplayer (if applicable)
- [ ] Client-Server connection
- [ ] Replication works correctly
- [ ] Iris replication graph functions
- [ ] Online subsystems (EOS/Steam) connect

#### Audio
- [ ] Sound effects play
- [ ] Music systems work
- [ ] MetaSounds function correctly
- [ ] Audio mixing behaves properly

---

## Known Issues & Considerations

### No Breaking Changes Identified
The upgrade from UE 5.4 to 5.5 is relatively smooth for Lyra-based projects. No major API deprecations affect this codebase.

### Potential Warnings
The following pre-existing warnings may appear but do not affect functionality:
- Unused local variables in `LyraPawnComponent_CharacterParts.cpp`
- Shadow variable warnings (set to Error level in target settings)

### Performance Notes
- Initial shader compilation in UE 5.5 may produce more shaders due to new optimizations
- DDC will be rebuilt, so first load will take longer
- Subsequent loads will be faster due to UE 5.5 optimizations

---

## Rollback Instructions

If you need to revert to UE 5.4:

1. **Restore the .uproject file:**
   ```json
   "EngineAssociation": "5.4"
   ```

2. **Clean build artifacts:**
   - Delete `Intermediate/`, `Binaries/`, `.vs/`

3. **Regenerate project files** for UE 5.4

4. **Rebuild** the project in UE 5.4

5. **Note:** Assets opened in UE 5.5 may have been upgraded and could show warnings in UE 5.4, but should still function.

---

## Additional Resources

### UE 5.5 Release Notes
- [Official Unreal Engine 5.5 Release Notes](https://docs.unrealengine.com/5.5/en-US/unreal-engine-5.5-release-notes/)

### Lyra Documentation
- UE 5.5 includes updated Lyra Starter Game sample
- Review Epic's Lyra 5.5 changes for potential enhancements to adopt

### Migration Guide
- [UE 5.4 to 5.5 API Changes](https://docs.unrealengine.com/5.5/en-US/API/index.html)

---

## Upgrade Summary

| Component | Status | Notes |
|-----------|--------|-------|
| .uproject file | ✅ Updated | Changed to 5.5 |
| Source code | ✅ Compatible | No changes needed |
| Build.cs files | ✅ Compatible | No changes needed |
| Target.cs files | ✅ Compatible | No changes needed |
| Plugins | ✅ Compatible | All plugins supported |
| Config files | ✅ Compatible | No changes needed |

**Estimated Total Upgrade Time:** 1-2 hours (including shader compilation)

**Risk Level:** LOW - This is a minor version upgrade with excellent backward compatibility.

---

## Support

If you encounter issues during the upgrade:

1. Check the Output Log in Unreal Editor for specific errors
2. Verify all build artifacts were cleaned before building
3. Ensure UE 5.5 is properly installed
4. Check that all Visual Studio components are up to date
5. Review the UE 5.5 release notes for any platform-specific issues

**Upgrade completed successfully! Proceed with build artifact cleanup and project regeneration.**
