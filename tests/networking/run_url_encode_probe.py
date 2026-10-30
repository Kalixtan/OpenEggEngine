#!/usr/bin/env python3
"""Check URL escaping and retail output-buffer boundaries headlessly."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
ENGINE = ROOT / "build/OpenEgg"
MARKERS = (
    "url encode 511-byte buffer PASS",
    "url encode overflow prefix PASS",
    "url encode path separator PASS",
    "url encode empty input PASS",
)


def main():
    if not ENGINE.is_file():
        raise SystemExit(f"engine not found: {ENGINE}")

    cache = Path("/tmp/OpenEgg_psarc_cache")
    remove_cache = not cache.exists()
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-url-encode-") as directory:
            root = Path(directory)
            test_engine = root / ENGINE.name
            shutil.copy2(ENGINE, test_engine)
            for archive in ENGINE.parent.glob("*.psarc"):
                (root / archive.name).symlink_to(archive)
            game_data = ENGINE.parent / "game_data"
            if game_data.is_dir():
                (root / "game_data").symlink_to(game_data, target_is_directory=True)
            (root / "OpenEgg.ini").write_text(
                "[system]\nos_id = ps3\nlanguage = en_us\n", encoding="utf-8")

            env = os.environ.copy()
            env.update({
                "GE3_HEADLESS": "1",
                "GE3_TRACE": "1",
                "GE3_STRICT": "halt",
                "GE3_HARNESS": str(ROOT / "tests/networking/harness.txt"),
                "SDL_AUDIODRIVER": "dummy",
                "LIBGL_ALWAYS_SOFTWARE": "1",
                "PATH": "/usr/bin:/bin:" + env.get("PATH", ""),
            })
            result = subprocess.run(
                [str(test_engine), str(ROOT / "tests/networking"), "UrlEncodeProbe.gm", "Main"],
                cwd=ROOT,
                env=env,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=30,
                check=False,
            )
            print(result.stdout, end="")
            missing = [marker for marker in MARKERS if marker not in result.stdout]
            if result.returncode or missing:
                raise SystemExit(f"URL encode probe failed; missing markers: {missing}")
    finally:
        if remove_cache and cache.exists():
            shutil.rmtree(cache)


if __name__ == "__main__":
    main()
