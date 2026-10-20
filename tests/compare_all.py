#!/usr/bin/env python3
"""Run every paired retail/OpenEgg probe section headlessly."""

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    commands = (
        ("sequencer", [sys.executable, str(ROOT / "tests/compare_probe.py")]),
        ("physics", [sys.executable, str(ROOT / "tests/physics/compare.py")]),
    )
    failed = []
    for section, command in commands:
        print(f"Running {section} probes", flush=True)
        if subprocess.run(command, cwd=ROOT, check=False).returncode:
            failed.append(section)
    if failed:
        print(f"Failed sections: {', '.join(failed)}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
