#!/usr/bin/env python3
"""Start the retail community process with the user's patched archives."""
import os
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--offline", action="store_true", help="Check the offline fallback")
parser.add_argument("--no-patch", action="store_true", help="Check fallback without the patch archive")
args = parser.parse_args()
engine = ROOT / "build/OpenEgg"
archives = list(engine.parent.glob("*.psarc"))
if not any(path.name == "patch_data001.psarc" for path in archives):
    raise SystemExit("patch_data001.psarc is required for this probe")

with tempfile.TemporaryDirectory(prefix="openegg-community-") as name:
    temp = Path(name)
    shutil.copy2(engine, temp / engine.name)
    for archive in archives:
        if args.no_patch and archive.name.startswith("patch_data"):
            continue
        (temp / archive.name).symlink_to(archive)
    (temp / "OpenEgg.ini").write_text("[network]\nserver_url = " + ("" if args.offline else "http://127.0.0.1:1") + "\n")
    harness = temp / "harness.txt"
    harness.write_text("wait 30\nquit\n")
    env = os.environ.copy()
    env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
               SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
    result = subprocess.run([str(temp / engine.name), str(ROOT / "tests/networking"),
                             "CommunityStartProbe.gm", "Main"], env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
    print(result.stdout, end="")
    markers = ["community fallback PASS"] if args.offline or args.no_patch else [
        "community screen constructor PASS", "community screen init PASS"]
    if result.returncode or any(marker not in result.stdout for marker in markers) or " FAIL" in result.stdout or "[gm]" in result.stdout:
        raise SystemExit("community start probe failed")
