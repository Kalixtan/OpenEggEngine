#!/usr/bin/env python3
"""Refresh all sequencer retail captures from an installed PS3 game."""

import argparse
from pathlib import Path
import subprocess
import sys

from compare_probe import BASELINE, PROBE, ROOT


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--main", type=Path, required=True)
    parser.add_argument("--eboot", type=Path, required=True)
    parser.add_argument("--timeout", type=int, default=38)
    args = parser.parse_args()
    folder = ROOT / "tests/sequencer"
    probes = [PROBE, folder / "edge_probe.gm", *sorted((folder / "edge_inputs").glob("*.gm"))]
    failed = []
    for probe in probes:
        baseline = BASELINE if probe == PROBE else folder / "edge-baselines" / f"{probe.stem}.tsv"
        print(f"retail: {probe.name}", flush=True)
        result = subprocess.run([
            sys.executable, str(ROOT / "tests/capture_retail_probe.py"),
            "--main", str(args.main), "--eboot", str(args.eboot),
            "--probe", str(probe), "--baseline", str(baseline),
            "--timeout", str(args.timeout),
        ], cwd=ROOT, check=False)
        if result.returncode:
            failed.append(probe.name)
    if failed:
        print(f"incomplete retail captures: {', '.join(failed)}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
