# PlatformIO extra script for env skr14turbo:
# - Cible manuelle: python -m platformio run -e skr14turbo -t flash_sd
# - Après chaque build: si le lecteur SD est monté (F: par défaut), supprime FIRMWARE.CUR
#   et copie firmware.bin. Désactiver: SKR_AUTO_FLASH=0
#
# Lecteur: variable SKR_SD_DRIVE (ex. G:)

import os
import shutil

Import("env")  # noqa: F821


def _drive_root():
    drive = os.environ.get("SKR_SD_DRIVE", "F:").rstrip("\\/")
    return drive + "\\"


def _do_copy(env):
    root = _drive_root()
    if not os.path.isdir(root):
        return False, "lecteur %s absent" % root.rstrip("\\")

    bin_src = os.path.join(env.subst("$BUILD_DIR"), "firmware.bin")
    if not os.path.isfile(bin_src):
        return False, "pas de %s" % bin_src

    cur = os.path.join(root, "FIRMWARE.CUR")
    # SKR bootloader expects the uppercase filename.
    dst_upper = os.path.join(root, "FIRMWARE.BIN")
    if os.path.isfile(cur):
        os.remove(cur)
        print("SKR SD: supprimé FIRMWARE.CUR")
    shutil.copy2(bin_src, dst_upper)
    print("SKR SD: copié -> %s" % dst_upper)
    return True, None


def _flash_sd_target(target, source, env):
    ok, err = _do_copy(env)
    if not ok:
        print("SKR SD: échec (%s)" % err)
        env.Exit(1)


def _post_build_auto(source, target, env):
    if os.environ.get("SKR_AUTO_FLASH", "1").strip() in ("0", "false", "no"):
        return
    root = _drive_root()
    if not os.path.isdir(root):
        return
    ok, err = _do_copy(env)
    if not ok:
        print("SKR SD: post-build ignoré (%s)" % err)


env.AddCustomTarget(
    "flash_sd",
    "$BUILD_DIR/firmware.bin",
    _flash_sd_target,
    title="SKR SD",
    description="Supprime FIRMWARE.CUR et copie firmware.bin (lecteur SKR_SD_DRIVE ou F:)",
)

# Après linkage / firmware.bin produit
env.AddPostAction("$BUILD_DIR/firmware.bin", _post_build_auto)
