#!/usr/bin/env python3
"""Ensure manual test sessions cannot be sent to remote servers."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    with tempfile.TemporaryDirectory(prefix="openegg-manual-transport-") as name:
        temp = Path(name)
        engine = ROOT / "build/OpenEgg"
        shutil.copy2(engine, temp / engine.name)
        for archive in engine.parent.glob("*.psarc"):
            (temp / archive.name).symlink_to(archive)
        game_data = engine.parent / "game_data"
        if game_data.is_dir():
            (temp / "game_data").symlink_to(game_data, target_is_directory=True)
        (temp / "OpenEgg.ini").write_text(
            "[network]\nserver_url = http://openegg-probe.invalid\n"
            "session_id = local-test-session\nuser_id = local-test-user\n")
        harness = temp / "harness.txt"
        harness.write_text("wait 30\nquit\n")
        env = os.environ.copy()
        env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt",
                   GE3_HARNESS=str(harness), SDL_AUDIODRIVER="dummy",
                   LIBGL_ALWAYS_SOFTWARE="1", PATH="/usr/bin:/bin:" + os.environ.get("PATH", ""))
        result = subprocess.run(
            [str(temp / engine.name), str(ROOT / "tests/networking"),
             "ManualSessionTransportProbe.gm", "Main"],
            cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, timeout=30)
        print(result.stdout, end="")
        expected = ("network: manual sessions are restricted to loopback servers",
                    "remote manual session blocked PASS")
        missing = [marker for marker in expected if marker not in result.stdout]
        if result.returncode or missing or "manual session validation failed" in result.stdout:
            raise SystemExit(f"manual-session transport probe failed: {missing}")


if __name__ == "__main__":
    main()
