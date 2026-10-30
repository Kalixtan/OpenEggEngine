#!/usr/bin/env python3
"""Compare retail and OpenEgg Watermelon_UrlEncode readings."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PROBE = ROOT / "tests/networking/UrlEncodeRetailProbe.gm"
BASELINE = ROOT / "tests/networking/url-encode-retail.tsv"
ENGINE = ROOT / "build/OpenEgg"

sys.path.insert(0, str(ROOT / "tests"))
from compare_probe import check_baseline_source, read_readings  # noqa: E402
from probe_expectations import expected_readings  # noqa: E402


def main():
    if not BASELINE.is_file():
        raise SystemExit(f"retail baseline missing: {BASELINE}")
    if not ENGINE.is_file():
        raise SystemExit(f"engine missing: {ENGINE}")
    check_baseline_source(BASELINE, PROBE)

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
        local, duplicates = read_readings(Path(output.name))
    retail, _ = read_readings(BASELINE)
    expected = set(expected_readings(PROBE))
    failures = []
    if result.returncode:
        failures.append(f"engine exited {result.returncode}")
    if duplicates:
        failures.append(f"duplicate local readings: {sorted(duplicates)}")
    if set(local) != expected:
        failures.append(f"local reading set differs: missing={sorted(expected - set(local))}")
    if set(retail) != expected:
        failures.append(f"retail reading set differs: missing={sorted(expected - set(retail))}")
    for name in sorted(expected & set(local) & set(retail)):
        if local[name] != retail[name]:
            failures.append(f"{name}: retail={retail[name]} OpenEgg={local[name]}")
        else:
            print(f"PASS {name} {local[name]}")
    if failures:
        raise SystemExit("URL encode parity failed:\n- " + "\n- ".join(failures))
    print(f"URL encode parity passed: {len(expected)}/{len(expected)}")


if __name__ == "__main__":
    main()
