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
    data = temp / "game_data"
    data.mkdir()
    (data / "near_send_fixture.bin").write_bytes(b"NearSendGift fixture\n")
    harness = temp / "harness.txt"
    env = os.environ.copy()
    env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
               SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")

    def run(script):
        harness.write_text("wait 30\nquit\n")
        return subprocess.run([str(temp / engine.name), str(ROOT / "tests/networking"),
                               script, "Main"], env=env, text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)

    valid = run("ActivityPostProbe.gm")
    print(valid.stdout, end="")
    if (valid.returncode or "activity post wrapper PASS" not in valid.stdout or
            "browser wrapper PASS" not in valid.stdout or "near gift table PASS" not in valid.stdout or
            "near send gift return PASS" not in valid.stdout or
            "near/fb no-result stubs PASS" not in valid.stdout or " FAIL" in valid.stdout or
            "[gm]" in valid.stdout):
        raise SystemExit("platform wrapper probe failed")

    for script, entered, returned in (
        ("ActivityPostNullProbe.gm", "activity null required entered", "activity null required returned"),
        ("ActivityPostOptionalProbe.gm", "activity invalid optional entered", "activity invalid optional returned"),
    ):
        result = run(script)
        print(result.stdout, end="")
        if (result.returncode or entered not in result.stdout or returned in result.stdout or
                "[gm]" not in result.stdout):
            raise SystemExit(f"{script} did not raise a GameMonkey exception")
