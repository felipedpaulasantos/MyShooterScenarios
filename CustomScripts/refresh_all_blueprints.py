# Refresh and compile all Blueprints in the project (including GameFeature plugin content)
# Usage (inside Unreal Editor):
# - Open Output Log and run: py "F:/UnrealProjects/MyShooterScenarios/CustomScripts/refresh_all_blueprints.py"
# Or use the Python Editor Script Plugin to execute this file.

import unreal

EDITOR_LIB = unreal.EditorAssetLibrary
BLUEPRINT_LIB = unreal.BlueprintEditorLibrary

# Candidate roots to scan. '/Game' is the project. GameFeature plugins mount under their own root like '/MyShooterFeaturePlugin'.
# We'll discover all top-level roots dynamically and include '/Game' explicitly.

def get_all_roots():
    roots = set(['/Game'])
    for mp in unreal.get_editor_subsystem(unreal.AssetRegistrySubsystem).get_all_cached_paths():
        # Only include top-level collection roots that look like content mounts
        if mp and mp.startswith('/') and mp.count('/') == 1 and mp != '/Engine':
            roots.add(mp)
    return sorted(roots)


def iter_assets(paths):
    for root in paths:
        for path in EDITOR_LIB.list_assets(root, recursive=True, include_folder=False):
            yield path


def is_blueprint_obj(obj):
    return isinstance(obj, unreal.Blueprint) or isinstance(obj, unreal.AnimBlueprint) or isinstance(obj, unreal.WidgetBlueprint)


def refresh_compile_save(path):
    obj = EDITOR_LIB.load_asset(path)
    if not obj:
        return False

    # For BP-derived types, get the underlying Blueprint asset if needed
    bp = None
    if isinstance(obj, unreal.Blueprint):
        bp = obj
    elif isinstance(obj, unreal.BlueprintGeneratedClass):
        bp = obj.get_blueprint_obj()
    elif isinstance(obj, (unreal.AnimBlueprint, unreal.WidgetBlueprint)):
        bp = obj

    if bp is None:
        return False

    try:
        BLUEPRINT_LIB.refresh_all_nodes(bp)
        BLUEPRINT_LIB.compile_blueprint(bp)
        # Save if it became dirty
        EDITOR_LIB.save_asset(path, only_if_is_dirty=True)
        return True
    except Exception as e:
        unreal.log_warning(f"Failed to refresh/compile/save {path}: {e}")
        return False


def main():
    roots = get_all_roots()
    unreal.log(f"Refreshing and compiling Blueprints under: {roots}")
    total = 0
    changed = 0
    for path in iter_assets(roots):
        asset_data = EDITOR_LIB.find_asset_data(path)
        if not asset_data:
            continue
        asset_class = asset_data.get_class()
        # Cheap filter: class name contains 'Blueprint' usually
        if 'Blueprint' not in asset_class.get_name():
            continue
        total += 1
        if refresh_compile_save(path):
            changed += 1
    unreal.log(f"Blueprint refresh/compile complete. Processed: {total}, Succeeded: {changed}.")


if __name__ == '__main__':
    main()
