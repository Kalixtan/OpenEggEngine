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
    "Watermelon index requests QUEUED",
    "typed index response PASS",
    "wildcard index response PASS",
    "index authorization response PASS",
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("server_url", help="Local server root, e.g. http://127.0.0.1:8080")
    parser.add_argument("--engine", type=Path, default=ROOT / "build" / "OpenEgg")
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
            f"network_base_url = {args.server_url.rstrip('/')}\n",
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
        result = subprocess.run(
            [str(test_engine), str(ROOT / "tests" / "networking"),
             "OpenEggNetworkProbe.gm", "Main"],
            cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, timeout=15, check=False,
        )
        print(result.stdout, end="")
        missing = [marker for marker in MARKERS if marker not in result.stdout]
        if result.returncode or missing:
            raise SystemExit(f"network probe failed; missing markers: {missing}")


if __name__ == "__main__":
    main()
