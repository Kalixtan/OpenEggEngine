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
parser.add_argument("--expect-populated", action="store_true", help="Require the synthetic Probe level in greatest hits")
parser.add_argument("--expect-thumbnail", action="store_true", help="Require the fixture panel's thumbnail")
parser.add_argument("--expect-level-data", action="store_true", help="Download the fixture panel's level data")
parser.add_argument("--expect-queue", action="store_true", help="Add, read and remove the fixture level from the queue")
args = parser.parse_args()
if (args.expect_populated or args.expect_thumbnail or args.expect_level_data or args.expect_queue) and not args.server_url:
    parser.error("populated checks require --server-url")
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
    script_dir = ROOT / "tests/networking"
    script_name = "CommunityBrowseProbe.gm" if args.server_url else "CommunityStartProbe.gm"
    if args.expect_thumbnail or args.expect_level_data or args.expect_queue:
        script_dir = temp / "probe"
        script_dir.mkdir()
        source = (ROOT / "tests/networking" / script_name).read_text()
        setup = "global COMMUNITY_REQUIRE_THUMBNAIL = 1;\n"
        if args.expect_level_data or args.expect_queue:
            setup += "global COMMUNITY_REQUIRE_LEVEL = 1;\n"
        if args.expect_queue:
            setup += "global COMMUNITY_REQUIRE_QUEUE = 1;\n"
        (script_dir / script_name).write_text(setup + source)
    result = subprocess.run([str(temp / engine.name), str(script_dir), script_name, "Main"], env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
    print(result.stdout, end="")
    markers = [f"community tab request PASS  {index}  panels " for index in (2, 1, 6, 7, 0)] if args.server_url else ["community fallback PASS"] if args.offline or args.no_patch else [
        "community screen constructor PASS", "community screen init PASS"]
    if args.expect_populated or args.expect_thumbnail or args.expect_level_data or args.expect_queue:
        markers.append("community fixture level PASS  2")
    if args.expect_thumbnail or args.expect_level_data or args.expect_queue:
        markers.append("community thumbnail PASS")
    if args.expect_level_data or args.expect_queue:
        markers.append("community level download PASS")
    if args.expect_queue:
        markers.extend(["community queue put PASS", "community queue read PASS", "community queue delete PASS",
                        "community queue cleared PASS"])
    if (result.returncode or any(marker not in result.stdout for marker in markers) or
            (args.server_url and result.stdout.count("community tab request PASS") != 5) or
            " FAIL" in result.stdout or "[gm]" in result.stdout):
        raise SystemExit("community start probe failed")
