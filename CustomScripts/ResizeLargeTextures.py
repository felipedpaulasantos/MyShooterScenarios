#!/usr/bin/env python
# ResizeLargeTextures.py
# Percorre todas as Texture2D em /Game e limita o tamanho máximo (LOD) para TARGET_MAX_TEXTURE_SIZE.
# NÃO modifica binário diretamente; usa propriedade max_texture_size para forçar geração de DDC menor.
# Se quiser reimportar fisicamente (downscale real), ative REIMPORT_LARGE=True (reimporta da mesma fonte; para diminuir o arquivo de origem, substitua a fonte antes).

import unreal, os

TARGET_MAX = int(os.environ.get("TARGET_MAX_TEXTURE_SIZE", "4096"))
REIMPORT_LARGE = os.environ.get("REIMPORT_LARGE", "false").lower() in ("1","true","yes")

log_lines = []
asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
texture_assets = asset_registry.get_assets_by_path("/Game", recursive=True)

changed_count = 0
skipped_count = 0
reimported_count = 0
estimated_saved_bytes = 0

for asset_data in texture_assets:
    if asset_data.asset_class != "Texture2D":
        continue
    obj_path = str(asset_data.object_path)
    tex = unreal.EditorAssetLibrary.load_asset(obj_path)
    if not tex:
        log_lines.append(f"SKIP (load fail): {obj_path}")
        continue

    # Obtém tamanho da fonte importada
    try:
        src = tex.get_editor_property("source")
        w = int(src.get_size_x())
        h = int(src.get_size_y())
    except Exception:
        w = int(getattr(tex, "blueprint_get_size_x", lambda: 0)()) if hasattr(tex, "blueprint_get_size_x") else 0
        h = int(getattr(tex, "blueprint_get_size_y", lambda: 0)()) if hasattr(tex, "blueprint_get_size_y") else 0

    if w <= 0 or h <= 0:
        skipped_count += 1
        log_lines.append(f"SKIP (no size): {obj_path}")
        continue

    if w <= TARGET_MAX and h <= TARGET_MAX:
        skipped_count += 1
        continue

    old_pixels = w * h

    # Escala proporcional para que a maior dimensão seja TARGET_MAX
    scale = TARGET_MAX / float(max(w, h))
    new_w = max(1, int(round(w * scale)))
    new_h = max(1, int(round(h * scale)))

    prev_max = None
    try:
        prev_max = tex.get_editor_property("max_texture_size")
    except Exception:
        pass

    # Define clamp
    try:
        tex.set_editor_property("max_texture_size", TARGET_MAX)
        # Gera mips conforme necessário
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FromTextureGroup)
    except Exception as e:
        log_lines.append(f"ERRO set props {obj_path}: {e}")
        continue

    unreal.EditorAssetLibrary.save_loaded_asset(tex)
    changed_count += 1

    new_pixels = new_w * new_h
    saved = max(0, old_pixels - new_pixels) * 4  # estimativa RGBA8
    estimated_saved_bytes += saved

    info = f"AJUSTE: {obj_path} {w}x{h} -> ~{new_w}x{new_h} (MaxTextureSize={TARGET_MAX}, prev={prev_max})"
    if REIMPORT_LARGE:
        try:
            unreal.AssetToolsHelpers.get_asset_tools().reimport_asset(tex)
            unreal.EditorAssetLibrary.save_loaded_asset(tex)
            reimported_count += 1
            info += " | Reimportada"
        except Exception as e:
            info += f" | Falha reimport: {e}"
    log_lines.append(info)

summary = f"Alteradas: {changed_count}, Reimportadas: {reimported_count}, Skipped: {skipped_count}, Estimativa economia: {estimated_saved_bytes/1024/1024:.2f} MB"
log_lines.append(summary)

unreal.log("==== ResizeLargeTextures resumo ====")
for l in log_lines:
    unreal.log(l)
unreal.log("====================================")

print(summary)
