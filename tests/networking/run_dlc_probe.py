#!/usr/bin/env python3
"""Check retail-style DLC ID parsing with the desktop entitlement fixture."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
ENGINE = ROOT / "build/OpenEgg"

with tempfile.TemporaryDirectory(prefix="openegg-dlc-") as directory:
    root = Path(directory)
    shutil.copy2(ENGINE, root / ENGINE.name)
    for archive in ENGINE.parent.glob("*.psarc"):
        (root / archive.name).symlink_to(archive)
    data = ENGINE.parent / "game_data"
    if data.is_dir():
        (root / "game_data").symlink_to(data, target_is_directory=True)

    harness = root / "harness.txt"
    harness.write_text("wait 30\nquit\n")
    env = os.environ.copy()
    env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt",
               GE3_HARNESS=str(harness), SDL_AUDIODRIVER="dummy",
               LIBGL_ALWAYS_SOFTWARE="1")
    template = (ROOT / "tests/networking/DlcProbe.gm").read_text()
    for enabled in (False, True):
        (root / "OpenEgg.ini").write_text(
            f"[system]\ndebug_all_dlc_available = {'true' if enabled else 'false'}\n")
        scripts = root / f"scripts-{enabled}"
        scripts.mkdir()
        (scripts / "DlcProbe.gm").write_text(
            template.replace("__DEBUG_ENABLED__", "true" if enabled else "false"))
        result = subprocess.run(
            [str(root / ENGINE.name), str(scripts), "DlcProbe.gm", "Main"],
            cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, timeout=30)
        print(result.stdout, end="")
        if result.returncode or "DLC ID parsing PASS" not in result.stdout:
            raise SystemExit("DLC probe failed")
