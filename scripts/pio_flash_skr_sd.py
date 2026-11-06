# PlatformIO extra script for env skr14turbo:
# - Cible manuelle: python -m platformio run -e skr14turbo -t flash_sd
# - Après chaque build: si la carte SD de la SKR est trouvée, supprime FIRMWARE.CUR
#   et copie firmware.bin en FIRMWARE.BIN. Désactiver: SKR_AUTO_FLASH=0
#
# Choix du lecteur, dans cet ordre:
#   1. variable SKR_SD_DRIVE (ex. G:) si elle est définie;
#   2. sinon, le lecteur qui contient FIRMWARE.CUR (laissé par la SKR après chaque flash)
#      ou FIRMWARE.BIN (copié mais pas encore flashé);
#   3. sinon, I: (lecteur SD par défaut).

import os
import shutil
import string

Import("env")  # noqa: F821

DEFAULT_DRIVE = "I:"
SKR_MARKER = "FIRMWARE.CUR"
SKR_FILES = (SKR_MARKER, "FIRMWARE.BIN")


def _root(drive):
    return drive.rstrip("\\/") + "\\"


def _find_skr_card():
    """Return the first drive (C: excluded) holding FIRMWARE.CUR or FIRMWARE.BIN, or None."""
    for letter in string.ascii_uppercase:
        if letter in ("A", "B", "C"):
            continue
        root = _root(letter + ":")
        try:
            if any(os.path.isfile(os.path.join(root, name)) for name in SKR_FILES):
                return letter + ":"
        except OSError:
            # Empty card reader slots and unreachable network drives raise here
            continue
    return None


def _drive_root():
    forced = os.environ.get("SKR_SD_DRIVE", "").strip()
    if forced:
        return _root(forced)
    if os.name == "nt":
        found = _find_skr_card()
        if found:
            return _root(found)
    return _root(DEFAULT_DRIVE)


def _do_copy(env, root):
    if not os.path.isdir(root):
        return False, "lecteur %s absent" % root.rstrip("\\")

    bin_src = os.path.join(env.subst("$BUILD_DIR"), "firmware.bin")
    if not os.path.isfile(bin_src):
        return False, "pas de %s" % bin_src

    cur = os.path.join(root, SKR_MARKER)
    # SKR bootloader expects the uppercase filename.
    dst_upper = os.path.join(root, "FIRMWARE.BIN")
    if os.path.isfile(cur):
        os.remove(cur)
        print("SKR SD: supprimé %s" % SKR_MARKER)
    shutil.copy2(bin_src, dst_upper)
    print("SKR SD: copié -> %s" % dst_upper)
    return True, None


def _flash_sd_target(target, source, env):
    ok, err = _do_copy(env, _drive_root())
    if not ok:
        print("SKR SD: échec (%s)" % err)
        env.Exit(1)


def _post_build_auto(source, target, env):
    if os.environ.get("SKR_AUTO_FLASH", "1").strip().lower() in ("0", "false", "no"):
        return
    root = _drive_root()
    if not os.path.isdir(root):
        print("SKR SD: carte non trouvée (%s absent), copie ignorée" % root.rstrip("\\"))
        return
    ok, err = _do_copy(env, root)
    if not ok:
        print("SKR SD: post-build ignoré (%s)" % err)


env.AddCustomTarget(
    "flash_sd",
    "$BUILD_DIR/firmware.bin",
    _flash_sd_target,
    title="SKR SD",
    description="Supprime FIRMWARE.CUR et copie firmware.bin (SKR_SD_DRIVE, sinon carte détectée, sinon I:)",
)

# Après linkage / firmware.bin produit
env.AddPostAction("$BUILD_DIR/firmware.bin", _post_build_auto)
