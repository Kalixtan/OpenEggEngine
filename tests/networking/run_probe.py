#!/usr/bin/env python3
"""Run OpenEgg's asynchronous GET probe against a local OTG server."""

import argparse
import os
import re
from pathlib import Path
import shutil
import subprocess
import tempfile
import uuid


ROOT = Path(__file__).resolve().parents[2]
MARKERS = (
    "base key split PASS",
    "query flag parsing PASS",
    "query float format PASS",
    "unloaded EULA values PASS",
    "configured identity PASS",
    "Watermelon index requests QUEUED",
    "typed index response PASS",
    "wildcard index response PASS",
    "index authorization response PASS",
    "album list route PASS",
    "album contents route PASS",
    "following page route PASS",
    "following all response PASS",
    "followers page route PASS",
    "hearts page route PASS",
    "heart get route PASS",
    "heart put route PASS",
    "heart delete route PASS",
    "following get route PASS",
    "following put route PASS",
    "following delete route PASS",
    "self follow short circuit PASS",
    "queued page route PASS",
    "queued get route PASS",
    "queued put route PASS",
    "queued delete route PASS",
    "null callback request QUEUED",
    "purge request QUEUED",
    "purged request callback PASS",
    "activity page route PASS",
    "activity post response PASS",
    "featured unknown-user response PASS",
    "featured post response PASS",
    "rating get response PASS",
    "record post route PASS",
    "favourite all response PASS",
    "favourite put response PASS",
    "favourite delete response PASS",
    "rating put response PASS",
    "leaderboard page route PASS",
    "leaderboard get response PASS",
    "leaderboard all response PASS",
    "leaderboard near route PASS",
    "leaderboard post setup PASS",
    "leaderboard integer post PASS",
    "leaderboard string post PASS",
    "metadata all response PASS",
    "metadata get response PASS",
    "metadata put response PASS",
    "empty metadata put response PASS",
    "ticket POST route PASS",
    "empty POST route PASS",
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("server_url", help="Local server root, e.g. http://127.0.0.1:8080")
    parser.add_argument("--engine", type=Path, default=ROOT / "build" / "OpenEgg")
    parser.add_argument("--session-id", help="Local test session ID for an authenticated probe")
    parser.add_argument("--user-id", default="test-user", help="Identity belonging to the session")
    parser.add_argument("--target-id", help="Fixture user ID for follow and unfollow checks")
    parser.add_argument("--level-id", help="Fixture level ID for heart and unheart checks")
    args = parser.parse_args()
    if args.target_id:
        args.target_id = str(uuid.UUID(args.target_id))
        if not args.session_id:
            parser.error("--target-id requires --session-id")
    if args.level_id and (not args.session_id or not re.fullmatch(r"[A-Za-z0-9]{8}", args.level_id)):
        parser.error("--level-id requires a session and an eight-character level ID")

    engine = args.engine.resolve()
    if not engine.is_file():
        raise SystemExit(f"engine not found: {engine}")
    if not args.server_url.startswith(("http://", "https://")):
        raise SystemExit("server URL must use http:// or https://")

    with tempfile.TemporaryDirectory(prefix="openegg-network-") as temp_name:
        temp = Path(temp_name)
        test_engine = temp / engine.name
        shutil.copy2(engine, test_engine)
        for asset in engine.parent.glob("*.psarc"):
            (temp / asset.name).symlink_to(asset)
        data_dir = engine.parent / "game_data"
        if data_dir.is_dir():
            (temp / "game_data").symlink_to(data_dir, target_is_directory=True)

        config = "[system]\nos_id = ps3\nlanguage = en_us\n"
        if args.session_id:
            config += (f"[network]\nserver_url = {args.server_url.rstrip('/')}\n"
                       f"user_id = {args.user_id}\nsession_id = {args.session_id}\n")
        else:
            config += (f"network_base_url = {args.server_url.rstrip('/')}\n"
                       f"network_user_id = {args.user_id}\n")
        (temp / "OpenEgg.ini").write_text(config, encoding="utf-8")

        env = os.environ.copy()
        env.update({
            "GE3_HEADLESS": "1",
            "GE3_TRACE": "1",
            "GE3_STRICT": "halt",
            "GE3_HARNESS": str(ROOT / "tests" / "networking" / "harness.txt"),
            "SDL_AUDIODRIVER": "dummy",
            "LIBGL_ALWAYS_SOFTWARE": "1",
            # Avoid selecting PSn00bSDK's cross assembler as host assembler.
            "PATH": "/usr/bin:/bin:" + env.get("PATH", ""),
        })
        probe = "AuthenticatedNetworkProbe.gm" if args.session_id else "OpenEggNetworkProbe.gm"
        markers = ("authenticated request QUEUED", "authenticated index response PASS",
                   "authenticated user pages QUEUED", "authenticated following response PASS",
                   "authenticated followers response PASS", "authenticated hearts response PASS") if args.session_id else MARKERS
        script_dir = ROOT / "tests" / "networking"
        if args.session_id:
            script_dir = temp / "probe"
            script_dir.mkdir()
            source = (ROOT / "tests" / "networking" / probe).read_text(encoding="utf-8")
            source = source.replace("__TARGET_ID__", args.target_id or "").replace("__LEVEL_ID__", args.level_id or "")
            (script_dir / probe).write_text(source, encoding="utf-8")
            if args.target_id:
                markers += ("authenticated follow PASS", "authenticated follow read PASS", "authenticated unfollow PASS",
                            "authenticated unfollow readback PASS")
            if args.level_id:
                markers += ("authenticated heart PASS", "authenticated heart read PASS", "authenticated unheart PASS",
                            "authenticated unheart readback PASS", "authenticated unpublish denied PASS",
                            "authenticated snapshot PASS", "authenticated versions response PASS",
                            "authenticated record post PASS", "authenticated record readback PASS")
        result = subprocess.run(
            [str(test_engine), str(script_dir),
             probe, "Main"],
            cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, timeout=15, check=False,
        )
        print(result.stdout, end="")
        missing = [marker for marker in markers if marker not in result.stdout]
        if result.returncode or missing:
            raise SystemExit(f"network probe failed; missing markers: {missing}")


if __name__ == "__main__":
    main()
