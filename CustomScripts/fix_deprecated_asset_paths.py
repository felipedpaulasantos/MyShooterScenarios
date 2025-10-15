"""
Script to fix deprecated FTopLevelAssetPath format warnings in UE 5.5
This script will find and resave assets to update them to the new format.

Run this in Unreal Engine Editor via:
- Tools > Execute Python Script
- Or use the Python console in the Output Log
"""

import unreal

def fix_deprecated_asset_paths():
    """
    Finds and resaves assets with deprecated FTopLevelAssetPath formats.
    """
    
    unreal.log("=" * 70)
    unreal.log("Fixing Deprecated FTopLevelAssetPath Format")
    unreal.log("=" * 70)
    
    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    editor_asset_lib = unreal.EditorAssetLibrary()
    
    # Focus on asset types that commonly have AssetBundleEntry data
    asset_classes = [
        "PrimaryDataAsset",
        "DataAsset", 
        "LyraPawnData",
        "LyraExperienceDefinition",
        "LyraExperienceActionSet",
        "SkeletalMesh",
        "AnimSequence",
        "AnimMontage",
        "BlendSpace",
        "BlendSpace1D"
    ]
    
    total_fixed = 0
    
    for asset_class in asset_classes:
        unreal.log(f"\n{'=' * 70}")
        unreal.log(f"Processing {asset_class} assets...")
        unreal.log('=' * 70)
        
        # Search for assets of this type
        filter = unreal.ARFilter(
            class_names=[asset_class],
            recursive_paths=["/Game"],
            recursive_classes=True
        )
        
        assets = asset_registry.get_assets(filter)
        
        if not assets:
            unreal.log(f"  No {asset_class} assets found.")
            continue
            
        unreal.log(f"  Found {len(assets)} {asset_class} assets")
        
        fixed_count = 0
        
        for i, asset_data in enumerate(assets):
            asset_path = str(asset_data.object_path)
            
            try:
                # Show progress every 10 assets
                if (i + 1) % 10 == 0:
                    unreal.log(f"  Progress: {i + 1}/{len(assets)}...")
                
                # Load the asset
                asset = unreal.load_asset(asset_path)
                
                if asset:
                    # Check if asset is modified or needs resaving
                    package = asset.get_outer()
                    
                    if package:
                        # Mark package dirty to force resave with new format
                        package.mark_package_dirty()
                        
                        # Save the asset
                        saved = editor_asset_lib.save_loaded_asset(asset, False)
                        
                        if saved:
                            fixed_count += 1
                            
            except Exception as e:
                unreal.log_warning(f"  Failed to process {asset_path}: {str(e)}")
                continue
        
        unreal.log(f"  ✓ Resaved {fixed_count} {asset_class} assets")
        total_fixed += fixed_count
    
    unreal.log("\n" + "=" * 70)
    unreal.log("Fix Complete!")
    unreal.log("=" * 70)
    unreal.log(f"Total assets resaved: {total_fixed}")
    unreal.log("\nThe deprecated FTopLevelAssetPath warnings should be gone.")
    unreal.log("Restart the editor to verify the warnings are cleared.")

if __name__ == "__main__":
    fix_deprecated_asset_paths()

