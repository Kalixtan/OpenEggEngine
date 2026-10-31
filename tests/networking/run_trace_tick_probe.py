#!/usr/bin/env python3
"""Check the GameMonkeyScript.tracTick compatibility stub."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="openegg-trace-tick-") as directory:
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
    shutil.copy2(ROOT / "tests/networking/TraceTickProbe.gm", data)
    harness = root / "harness.txt"
    harness.write_text("wait 30\nquit\n")
    env = os.environ.copy()
    env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
               SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
    result = subprocess.run(
        [str(root / "OpenEgg"), str(data), "TraceTickProbe.gm", "Main"],
        env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
    print(result.stdout, end="")
    if result.returncode or "GameMonkeyScript.tracTick PASS" not in result.stdout:
        raise SystemExit("traceTick compatibility probe failed")
