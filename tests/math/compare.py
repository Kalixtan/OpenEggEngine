#!/usr/bin/env python3
"""Compare vector and math probes with the saved retail capture."""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests"))
from compare_probe import check_baseline_source, read_readings
from probe_expectations import expected_readings
from run import run_script

HERE = Path(__file__).resolve().parent
PROBE = HERE / "probe.gm"
PLAN = HERE / "plan.json"
BASELINE = HERE / "retail.tsv"


def main():
    expected = expected_readings(PROBE, PLAN)
    check_baseline_source(BASELINE, PROBE)
    retail, _ = read_readings(BASELINE)
    _, error, log, _ = run_script(ROOT / "build/OpenEgg", HERE, PROBE.name,
                                  HERE / "harness", 20,
                                  ROOT / "tests/_results/math", strict="survey")
    local, duplicates = read_readings(log)
    failures = []
    for name in expected:
        if retail.get(name) != local.get(name):
            failures.append((name, retail.get(name), local.get(name)))
    if error:
        print(f"OpenEgg run error: {error}")
    for name, expected_value, actual in failures:
        print(f"FAIL {name}: retail={expected_value!r} OpenEgg={actual!r}")
    if duplicates:
        print(f"duplicate readings: {', '.join(sorted(duplicates))}")
    passed = len(expected) - len(failures)
    print(f"Math probes: {passed}/{len(expected)} matched retail")
    return 1 if failures or duplicates or error else 0


if __name__ == "__main__":
    sys.exit(main())
