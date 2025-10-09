# Unreal Engine Project Cleanup Guide

## Safe Cleanup Steps for Unreal Engine Projects

### Step 1: Use Unreal Engine's Built-in Tools (RECOMMENDED)

#### A. Reference Viewer (Find Unused Assets)
1. Open the project in Unreal Engine 5.4
2. In Content Browser, right-click on a folder (e.g., Content)
3. Select "Reference Viewer"
4. Assets with no incoming references are potentially unused

#### B. Size Map (Find Large Assets)
1. In Content Browser, click the View Options (bottom-right)
2. Enable "Show Plugin Content" and "Show Engine Content"
3. Right-click any folder > "Size Map"
4. This shows a visual representation of asset sizes
5. Identify large, unused assets

#### C. Asset Audit (Official Way)
1. Window > Developer Tools > Asset Audit
2. This shows all assets and their references
3. You can filter by:
   - Assets with no references
   - Assets by size
   - Assets by type

### Step 2: Clean Folders That Are Safe to Delete

These folders can be COMPLETELY deleted (they regenerate):

```powershell
# DO NOT DELETE YET - Review first!
Remove-Item "F:\UnrealProjects\MyShooterScenarios\Binaries" -Recurse -Force
Remove-Item "F:\UnrealProjects\MyShooterScenarios\Intermediate" -Recurse -Force
Remove-Item "F:\UnrealProjects\MyShooterScenarios\Saved" -Recurse -Force
Remove-Item "F:\UnrealProjects\MyShooterScenarios\DerivedDataCache" -Recurse -Force -ErrorAction SilentlyContinue
```

### Step 3: Identify Unused Content Packs

Based on your Content folder structure, you have many asset packs:
- ParagonCharacters (multiple)
- Environment packs
- VFX packs
- Audio packs
- Animation packs

**To check if they're used:**
1. In Unreal Editor, Content Browser
2. Right-click on a pack folder (e.g., "Content/ParagonCountess")
3. Select "Fix Up Redirectors in Folder"
4. Then right-click > "Reference Viewer"
5. If nothing references it, it's safe to delete

### Step 4: Find Duplicate or Unused Source Assets

Large source files that might be unused:
- `.fbx` / `.FBX` files (3D models)
- `.psd` files (Photoshop source files)
- `.blend` / `.max` / `.ma` files (3D software files)

These are often imported once and the source file can be removed.

### Step 5: Use the Cleanup Script Below

## Automated Cleanup Options

### Option A: Clean Generated/Temporary Folders (SAFE)
```powershell
# Run this from project root
.\CleanupProject.ps1 -Mode Safe
```

### Option B: Deep Clean + Find Unused Assets (REQUIRES REVIEW)
```powershell
# Run this from project root
.\CleanupProject.ps1 -Mode DeepScan
```

### Option C: Remove Specific Asset Packs
```powershell
# Example: Remove a specific content pack you know you don't use
Remove-Item "F:\UnrealProjects\MyShooterScenarios\Content\ParagonCountess" -Recurse -Force
```

## Important Warnings

⚠️ **Before deleting ANY Content folder:**
1. Make a backup of the entire project
2. Verify in Unreal Editor that assets aren't used
3. Test the project after deletion

⚠️ **Never delete:**
- Content/Core (Lyra core systems)
- Content/Characters (if you use them)
- Content/Plugins (if plugins are active)
- Source folder

⚠️ **Safe to delete after verification:**
- Asset packs you imported but never used
- Source files (.fbx, .psd) after import (keep originals elsewhere)
- Demo/Example content that came with asset packs

## Recommended Workflow

1. **Backup First!**
   ```powershell
   Copy-Item -Path "F:\UnrealProjects\MyShooterScenarios" -Destination "F:\UnrealProjects\MyShooterScenarios_BACKUP" -Recurse
   ```

2. **Clean Safe Folders**
   - Run the cleanup script for Binaries/Intermediate/Saved

3. **Open in Unreal Engine**
   - Use Asset Audit tool
   - Use Size Map tool
   - Review each content pack

4. **Delete Unused Packs**
   - One at a time
   - Test after each deletion

5. **Resave and Recompile**
   - Fix any broken references
   - Recompile the project

6. **Commit to Git**
   - Only commit the cleaned version

## Estimated Space Savings

Typical cleanup can save:
- Binaries/Intermediate/Saved: 5-20 GB (regenerates)
- Unused asset packs: 1-10 GB each
- Source files (.fbx, .psd): 500 MB - 5 GB
- DerivedDataCache: 2-10 GB (regenerates)

After cleanup, your Git repository size could potentially be 50-70% smaller!

