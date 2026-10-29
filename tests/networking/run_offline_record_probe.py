#!/usr/bin/env python3
"""Check retail offline-score callbacks against a disposable server fixture."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("server_url")
parser.add_argument("user_id")
parser.add_argument("session_id")
parser.add_argument("level_id")
args = parser.parse_args()
endpoint = urlsplit(args.server_url)
if (endpoint.scheme != "http" or endpoint.hostname != "127.0.0.1" or not endpoint.port or
        endpoint.username is not None or endpoint.password is not None or
        endpoint.path not in ("", "/") or endpoint.query or endpoint.fragment or
        not re.fullmatch(r"[A-Za-z0-9]{8}", args.level_id) or args.level_id == "zzzzzzzz"):
    parser.error("a local server and a disposable fixture level are required")

with tempfile.TemporaryDirectory(prefix="openegg-offline-record-") as name:
    temp = Path(name)
    engine = ROOT / "build/OpenEgg"
    shutil.copy2(engine, temp / engine.name)
    for archive in engine.parent.glob("*.psarc"):
        (temp / archive.name).symlink_to(archive)
    data = engine.parent / "game_data"
    if data.is_dir():
        (temp / "game_data").symlink_to(data, target_is_directory=True)
    (temp / "OpenEgg.ini").write_text(
        f"[network]\nserver_url = {args.server_url}\nuser_id = {args.user_id}\nsession_id = {args.session_id}\n")
    probe = temp / "probe"
    probe.mkdir()
    source = (ROOT / "tests/networking/OfflineRecordProbe.gm").read_text()
    (probe / "OfflineRecordProbe.gm").write_text(source.replace("__LEVEL_ID__", args.level_id))
    harness = temp / "harness.txt"
    harness.write_text("wait 600\nquit\n")
    env = os.environ.copy()
    env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
               SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
    result = subprocess.run([str(temp / engine.name), str(probe), "OfflineRecordProbe.gm", "Main"],
                            env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
    print(result.stdout, end="")
    if (result.returncode or "offline score callbacks PASS" not in result.stdout or
            "offline score leaderboard PASS" not in result.stdout or " FAIL" in result.stdout or "[gm]" in result.stdout):
        raise SystemExit("offline record probe failed")
