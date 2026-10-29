#!/usr/bin/env python3
"""Start the retail menu and connect its community screen to a local server."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from urllib.parse import urlsplit

from run_account_login_probe import EMAIL, PASSWORD

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("server_url", help="Local server with the disposable fixture account")
args = parser.parse_args()
endpoint = urlsplit(args.server_url)
if (endpoint.scheme != "http" or endpoint.hostname != "127.0.0.1" or not endpoint.port or
        endpoint.username is not None or endpoint.password is not None or
        endpoint.path not in ("", "/") or endpoint.query or endpoint.fragment):
    parser.error("fixture server must be a local 127.0.0.1 URL")

with tempfile.TemporaryDirectory(prefix="openegg-full-menu-") as name:
    temp = Path(name)
    engine = ROOT / "build/OpenEgg"
    shutil.copy2(engine, temp / engine.name)
    for archive in engine.parent.glob("*.psarc"):
        (temp / archive.name).symlink_to(archive)
    data = engine.parent / "game_data"
    if data.is_dir():
        (temp / "game_data").symlink_to(data, target_is_directory=True)
    (temp / "OpenEgg.ini").write_text(
        f"[network]\nserver_url = {args.server_url}\naccount_email = {EMAIL}\naccount_password = {PASSWORD}\n")
    harness = temp / "harness.txt"
    harness.write_text("wait 900\nquit\n")
    env = os.environ.copy()
    env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
               SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
    script_dir = ROOT / "tests/networking"
    try:
        result = subprocess.run([str(temp / engine.name), str(script_dir),
                                 "FullMenuProbe.gm", "Main"], env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=100)
    except subprocess.TimeoutExpired as error:
        if error.stdout:
            print(error.stdout.decode(errors="replace") if isinstance(error.stdout, bytes) else error.stdout, end="")
        raise SystemExit("full menu probe timed out")
    print(result.stdout, end="")
    markers = ("full menu initialized PASS", "full menu online PASS", "full menu community attached PASS")
    if result.returncode or any(marker not in result.stdout for marker in markers) or "[gm]" in result.stdout:
        raise SystemExit("full menu probe failed")
