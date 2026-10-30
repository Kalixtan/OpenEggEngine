#!/usr/bin/env python3
"""Check desktop platform restriction fallbacks."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="openegg-restriction-") as directory:
    root = Path(directory)
    shutil.copy2(ROOT / "build/OpenEgg", root / "OpenEgg")
    harness = root / "harness.txt"
    harness.write_text("wait 30\nquit\n")
    env = os.environ.copy()
    env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
               SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
    template = (ROOT / "tests/networking/PlatformRestrictionProbe.gm").read_text()
    for allowed in (True, False):
        for account_restricted in (False, True):
            data = root / f"allowed-{allowed}-account-{account_restricted}"
            data.mkdir()
            script = template.replace("__CHAT_EXPECTED__", "0" if allowed else "1")
            script = script.replace("__ACCOUNT_EXPECTED__", "1" if account_restricted else "0")
            (data / "PlatformRestrictionProbe.gm").write_text(script)
            (root / "OpenEgg.ini").write_text(
                f"[network]\ncommunity_allowed = {'true' if allowed else 'false'}\n"
                f"account_restricted = {'true' if account_restricted else 'false'}\n")
            result = subprocess.run([str(root / "OpenEgg"), str(data), "PlatformRestrictionProbe.gm", "Main"],
                                    env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
            print(result.stdout, end="")
            if (result.returncode or "platform restriction PASS" not in result.stdout or
                    "desktop trophy queue rejection PASS" not in result.stdout or
                    "desktop trophy queue rejection FAIL" in result.stdout):
                raise SystemExit("platform restriction probe failed")
