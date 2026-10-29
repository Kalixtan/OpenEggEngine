#!/usr/bin/env python3
"""Exercise PS3 activity and browser wrappers with a headless engine."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
engine = ROOT / "build/OpenEgg"
with tempfile.TemporaryDirectory(prefix="openegg-activity-") as name:
    temp = Path(name)
    shutil.copy2(engine, temp / engine.name)
    for archive in engine.parent.glob("*.psarc"):
        (temp / archive.name).symlink_to(archive)
    data = engine.parent / "game_data"
    if data.is_dir():
        (temp / "game_data").symlink_to(data, target_is_directory=True)
    harness = temp / "harness.txt"
    harness.write_text("wait 30\nquit\n")
    env = os.environ.copy()
    env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
               SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
    result = subprocess.run([str(temp / engine.name), str(ROOT / "tests/networking"),
                             "ActivityPostProbe.gm", "Main"], env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
    print(result.stdout, end="")
    if (result.returncode or "activity post wrapper PASS" not in result.stdout or
            "browser wrapper PASS" not in result.stdout or " FAIL" in result.stdout or "[gm]" in result.stdout):
        raise SystemExit("platform wrapper probe failed")
