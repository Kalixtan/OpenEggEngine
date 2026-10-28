#!/usr/bin/env python3
"""Start the retail community process with the user's patched archives."""
import os
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--offline", action="store_true", help="Check the offline fallback")
parser.add_argument("--no-patch", action="store_true", help="Check fallback without the patch archive")
parser.add_argument("--server-url", help="Run a tab request against a local server with the fixture account")
args = parser.parse_args()
if args.server_url:
    endpoint = urlsplit(args.server_url)
    if (endpoint.scheme != "http" or endpoint.hostname != "127.0.0.1" or
            endpoint.username is not None or endpoint.password is not None or
            not endpoint.port or endpoint.path not in ("", "/") or endpoint.query or endpoint.fragment or
            args.offline or args.no_patch):
        parser.error("server probe requires a local server and the patch archive")
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
    url = args.server_url or ("" if args.offline else "http://127.0.0.1:1")
    config = "[network]\nserver_url = " + url + "\n"
    if args.server_url:
        from run_account_login_probe import EMAIL, PASSWORD
        config += f"account_email = {EMAIL}\naccount_password = {PASSWORD}\n"
    (temp / "OpenEgg.ini").write_text(config)
    harness = temp / "harness.txt"
    harness.write_text(f"wait {600 if args.server_url else 30}\nquit\n")
    env = os.environ.copy()
    env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
               SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
    result = subprocess.run([str(temp / engine.name), str(ROOT / "tests/networking"),
                             "CommunityBrowseProbe.gm" if args.server_url else "CommunityStartProbe.gm", "Main"], env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
    print(result.stdout, end="")
    markers = ["community tab request PASS"] if args.server_url else ["community fallback PASS"] if args.offline or args.no_patch else [
        "community screen constructor PASS", "community screen init PASS"]
    if result.returncode or any(marker not in result.stdout for marker in markers) or " FAIL" in result.stdout or "[gm]" in result.stdout:
        raise SystemExit("community start probe failed")
