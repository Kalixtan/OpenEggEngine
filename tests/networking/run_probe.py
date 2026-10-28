#!/usr/bin/env python3
"""Run OpenEgg's asynchronous GET probe against a local OTG server."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
MARKERS = (
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
    args = parser.parse_args()

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

        (temp / "OpenEgg.ini").write_text(
            "[system]\nos_id = ps3\nlanguage = en_us\n"
            f"network_base_url = {args.server_url.rstrip('/')}\n"
            f"network_user_id = {args.user_id}\n"
            f"network_session_id = {args.session_id or ''}\n",
            encoding="utf-8",
        )

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
        result = subprocess.run(
            [str(test_engine), str(ROOT / "tests" / "networking"),
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
