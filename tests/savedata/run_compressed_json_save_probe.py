#!/usr/bin/env python3
"""Check compressed GameMonkey JSON save requests."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="openegg-compressed-save-") as directory:
    root = Path(directory)
    engine = ROOT / "build/OpenEgg"
    shutil.copy2(engine, root / "OpenEgg")
    for archive in engine.parent.glob("*.psarc"):
        (root / archive.name).symlink_to(archive)
    game_data = engine.parent / "game_data"
    if game_data.is_dir():
        (root / "game_data").symlink_to(game_data, target_is_directory=True)
    data = root / "data"
    data.mkdir()
    shutil.copy2(ROOT / "tests/savedata/CompressedJsonSaveProbe.gm", data)
    harness = root / "harness.txt"
    harness.write_text("wait 30\nquit\n")
    env = os.environ.copy()
    env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
               SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
    result = subprocess.run(
        [str(root / "OpenEgg"), str(data), "CompressedJsonSaveProbe.gm", "Main"],
        env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
    print(result.stdout, end="")
    if (result.returncode or "compressed JSON save round trip PASS" not in result.stdout or
            "compressed JSON save round trip FAIL" in result.stdout):
        raise SystemExit("compressed JSON save probe failed")
