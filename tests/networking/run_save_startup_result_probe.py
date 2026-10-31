#!/usr/bin/env python3
"""Check the save states used by the startup flow."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
EXPECTED = {"new": 1, "no-toc": 2, "no-patch": 5, "complete": 3}
with tempfile.TemporaryDirectory(prefix="openegg-save-startup-") as directory:
    root = Path(directory)
    engine = ROOT / "build/OpenEgg"
    shutil.copy2(engine, root / "OpenEgg")
    for archive in engine.parent.glob("*.psarc"):
        (root / archive.name).symlink_to(archive)
    game_data = engine.parent / "game_data"
    if game_data.is_dir():
        (root / "game_data").symlink_to(game_data, target_is_directory=True)
    harness = root / "harness.txt"
    harness.write_text("wait 30\nquit\n")
    env = os.environ.copy()
    env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
               SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
    template = (ROOT / "tests/networking/SaveStartupResultProbe.gm").read_text()

    for name, expected in EXPECTED.items():
        save_dir = root / "savedata"
        shutil.rmtree(save_dir, ignore_errors=True)
        save_dir.mkdir()
        if name != "new":
            (save_dir / "save.json").write_text("{}")
        if name in ("no-patch", "complete"):
            toc_dir = save_dir / "myLevels"
            toc_dir.mkdir()
            (toc_dir / "toc.json").write_text("{}")
        if name == "complete":
            (save_dir / "patchfile.json").write_text("{}")

        data = root / f"data-{name}"
        data.mkdir()
        script = template.replace("__EXPECTED__", str(expected))
        (data / "SaveStartupResultProbe.gm").write_text(script)
        result = subprocess.run(
            [str(root / "OpenEgg"), str(data), "SaveStartupResultProbe.gm", "Main"],
            env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        print(result.stdout, end="")
        if result.returncode or "save startup result PASS" not in result.stdout or \
                "save startup result FAIL" in result.stdout:
            raise SystemExit(f"save startup result probe failed: {name}")
