#!/usr/bin/env python3
"""Check OpenEgg's safe result for a retail BaseKeyValue hang case."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PROBE = ROOT / "tests/networking/BaseKeyValueRetailProbe.gm"
BASELINE = ROOT / "tests/networking/base-key-value-retail.tsv"
PLAN = ROOT / "tests/networking/base-key-value-retail-plan.json"
ENGINE = ROOT / "build/OpenEgg"

sys.path.insert(0, str(ROOT / "tests"))
from compare_probe import check_baseline_source, read_readings  # noqa: E402
from probe_expectations import expected_readings  # noqa: E402


def main():
    if not BASELINE.is_file() or not ENGINE.is_file():
        raise SystemExit("retail baseline or OpenEgg build is missing")
    check_baseline_source(BASELINE, PROBE)
    expected = set(expected_readings(PROBE, PLAN))
    hang = set(json.loads(PLAN.read_text())["retailHang"])
    if not hang <= expected:
        raise SystemExit("retail hang case is not declared by the probe")

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
        [str(ENGINE), str(PROBE.parent), PROBE.name, "Main"],
        cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, timeout=30, check=False)
    print(result.stdout, end="")
    with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8") as output:
        output.write(result.stdout)
        output.flush()
        local, local_duplicates = read_readings(Path(output.name))
    retail, retail_duplicates = read_readings(BASELINE)

    if result.returncode:
        raise SystemExit(f"OpenEgg exited {result.returncode}")
    if local_duplicates or retail_duplicates:
        raise SystemExit("duplicate BaseKeyValue readings")
    if set(local) != expected:
        raise SystemExit(f"OpenEgg readings differ: missing={sorted(expected - set(local))}")
    if set(retail) != expected - hang:
        raise SystemExit(f"retail readings differ: expected hang set={sorted(hang)}")
    if local.get("basekey.utf8overflow.key") != "198":
        raise SystemExit("OpenEgg did not return the safe 198-byte prefix")
    if retail.get("basekey.before") != "call":
        raise SystemExit("retail did not reach the call marker before hanging")
    print("RETAIL HANG basekey.utf8overflow.key; OpenEgg returns length 198")


if __name__ == "__main__":
    main()
