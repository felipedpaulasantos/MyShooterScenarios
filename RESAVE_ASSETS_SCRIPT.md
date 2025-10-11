# Script to Resave Assets with UE 5.5 Format

## Issue
Animation assets and other content show this warning:
```
Warning: While importing text for property 'AssetPaths' in 'AssetBundleEntry':Struct format for FTopLevelAssetPath is deprecated. Imported struct: (SK_Mannequin)/1_
```

This happens because assets saved in UE 5.4 have the old asset path format embedded in their metadata.

## Solution: Resave Assets in UE 5.5

### Method 1: Use Editor Utility Widget (Recommended)

Create an Editor Utility Widget with this Python script:

```python
import unreal

# Get the asset registry
asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()

# Get all animation assets
all_assets = asset_registry.get_assets_by_path("/Game", recursive=True)

animation_assets = []
for asset_data in all_assets:
    asset_class = str(asset_data.asset_class_path.asset_name)
    if asset_class in ["AnimSequence", "AnimMontage", "BlendSpace", "AnimBlueprint"]:
        animation_assets.append(str(asset_data.object_path))

print(f"Found {len(animation_assets)} animation assets to resave")

# Load and save each asset
editor_asset_lib = unreal.EditorAssetLibrary()
saved_count = 0

for asset_path in animation_assets:
    if editor_asset_lib.does_asset_exist(asset_path):
        asset = editor_asset_lib.load_asset(asset_path)
        if asset:
            editor_asset_lib.save_asset(asset_path)
            saved_count += 1
            if saved_count % 10 == 0:
                print(f"Saved {saved_count}/{len(animation_assets)} assets...")

print(f"Successfully resaved {saved_count} animation assets")
```

### Method 2: Use Content Browser (Manual)

1. Open your project in UE 5.5
2. In Content Browser, navigate to your animation folders
3. Select all animations (Ctrl+A)
4. Right-click → Asset Actions → **Bulk Edit via Property Matrix**
5. Close the Property Matrix (this loads all assets)
6. Right-click on selected assets → **Save**

### Method 3: Command Line Resave

In the editor, open the Output Log and run:

```
obj savepackage /Game/Characters
obj savepackage /Game/Animations
obj savepackage /Game/ParagonCharacters
```

Replace with your actual content paths.

### Method 4: Fix Up Redirectors (Fastest)

1. In Content Browser, right-click on **Content** folder
2. Select **Fix Up Redirectors in Folder**
3. This will resave affected assets automatically

## What Gets Fixed

When assets are resaved in UE 5.5:
- Old format: `(PackageName)/AssetName` → Deprecated
- New format: `/PackageName.AssetName` → FTopLevelAssetPath

This applies to:
- Asset bundle data in Animation assets
- Asset references in Data Assets
- Experience Definitions
- Game Feature Actions

## Expected Results

After resaving:
- ✅ No more `FTopLevelAssetPath` deprecation warnings
- ✅ Assets use UE 5.5 format
- ✅ Faster loading times
- ✅ Better compatibility with future UE versions

## Note

These warnings are **cosmetic** and don't break functionality, but it's best practice to resave assets after an engine upgrade.

