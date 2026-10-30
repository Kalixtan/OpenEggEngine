#!/usr/bin/env python3
"""Smoke-test retail locale font setup using bundled game resources."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
ENGINE = ROOT / "build/OpenEgg"

with tempfile.TemporaryDirectory(prefix="openegg-network-font-") as directory:
    root = Path(directory)
    shutil.copy2(ENGINE, root / ENGINE.name)
    for archive in ENGINE.parent.glob("*.psarc"):
        (root / archive.name).symlink_to(archive)
    data = ENGINE.parent / "game_data"
    if data.is_dir():
        (root / "game_data").symlink_to(data, target_is_directory=True)
    (root / "OpenEgg.ini").write_text("[system]\nlanguage = en_us\n[network]\nproduct_region = SCEA\n")
    harness = root / "harness.txt"
    harness.write_text("wait 30\nquit\n")
    env = os.environ.copy()
    env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt",
               GE3_HARNESS=str(harness), SDL_AUDIODRIVER="dummy",
               LIBGL_ALWAYS_SOFTWARE="1")
    result = subprocess.run(
        [str(root / ENGINE.name), str(ROOT / "tests/networking"), "NetworkFontProbe.gm", "Main"],
        cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, timeout=30)
    print(result.stdout, end="")
    markers = (
        "network locale font default PASS",
        "network locale font Japanese PASS",
        "network locale font Russian PASS",
        "network locale font Korean PASS",
        "network locale font Traditional Chinese PASS",
    )
    if result.returncode or any(marker not in result.stdout for marker in markers):
        raise SystemExit("network locale font probe failed")
