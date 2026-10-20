#!/usr/bin/env python3
"""Compare audio script readings from retail and OpenEgg."""

import argparse
from collections import defaultdict
import csv
from datetime import datetime, timezone
import os
from pathlib import Path
import shutil
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from compare_probe import check_baseline_source, read_readings
from probe_expectations import expected_readings
from run import ROOT, percent, run_script

HERE = Path(__file__).resolve().parent
SOUND = Path("Platformer/sounds/fx/button-high.wav")
REPORT = ROOT / "AUDIO_TEST_REPORT.md"


def inventory():
    with (ROOT / "docs/api/function_notes.tsv").open() as source:
        return sorted(row["name"] for row in csv.DictReader(source, delimiter="\t")
                      if row["name"].startswith("Audio."))


def compare_section(engine, probe, baseline, root, log_dir, names):
    check_baseline_source(baseline, probe)
    declared = expected_readings(probe)
    if len(declared) != len(set(declared)):
        raise ValueError(f"duplicate reading names in {probe}")
    _, error, log_path, _ = run_script(
        engine, root, probe.name, HERE / "smoke.harness", 15, log_dir,
        strict="survey")
    retail, retail_dupes = read_readings(baseline)
    local, local_dupes = read_readings(log_path)
    problems = [f"{probe.name}: {error}"] if error else []
    for label, values, duplicates in (("retail", retail, retail_dupes),
                                      ("OpenEgg", local, local_dupes)):
        problems += [f"{probe.name} {label}: missing {name}"
                     for name in declared if name not in values]
        problems += [f"{probe.name} {label}: unexpected {name}"
                     for name in values if name not in declared]
        problems += [f"{probe.name} {label}: duplicate {name}"
                     for name in sorted(duplicates)]
    rows = []
    for name in declared:
        function = ".".join(name.split(".")[:2])
        if function not in names:
            raise ValueError(f"no API inventory entry for {name}")
        rows.append((function, name, retail.get(name), local.get(name),
                     name in retail and name in local and retail[name] == local[name]))
    return rows, problems


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, default=ROOT / "build/OpenEgg")
    parser.add_argument("--game-data", type=Path,
                        default=os.environ.get("OPENEGG_AUDIO_GAME_DATA"),
                        help="your extracted retail game_data folder; enables voice checks")
    parser.add_argument("--report", type=Path, default=REPORT)
    args = parser.parse_args()

    names = inventory()
    log_dir = ROOT / "tests/_results/audio"
    log_dir.mkdir(parents=True, exist_ok=True)
    rows, problems = compare_section(args.engine.resolve(), HERE / "state.gm",
                                     HERE / "state-retail.tsv", HERE, log_dir, names)
    asset_status = "skipped (pass --game-data to test voices)"
    if args.game_data:
        source = args.game_data.expanduser().resolve() / SOUND
        if not source.is_file():
            parser.error(f"sound file missing: {source}")
        with tempfile.TemporaryDirectory(prefix="openegg-audio-") as temporary:
            root = Path(temporary)
            target = root / SOUND
            target.parent.mkdir(parents=True)
            shutil.copyfile(source, target)
            shutil.copyfile(HERE / "voice.gm", root / "voice.gm")
            voice_rows, voice_problems = compare_section(
                args.engine.resolve(), HERE / "voice.gm",
                HERE / "voice-retail.tsv", root, log_dir, names)
        rows.extend(voice_rows)
        problems.extend(voice_problems)
        asset_status = "tested with user supplied sound"

    grouped = defaultdict(list)
    for function, _, _, _, passed in rows:
        grouped[function].append(passed)
    passed = sum(row[4] for row in rows)
    tested = sum(bool(grouped[name]) for name in names)
    lines = ["# Audio test report", "",
             f"Generated: {datetime.now(timezone.utc):%Y-%m-%d %H:%M UTC}", "",
             f"Matched: **{passed}/{len(rows)} ({percent(passed, len(rows))})** script readings. "
             "This does not test emitted audio or DSP parity.", "",
             f"Voice probe: {asset_status}. Functions with readings: **{tested}/{len(names)}**.", "",
             "## Per function", "",
             "| Function | Matched | Cases | Status |", "| --- | ---: | ---: | --- |"]
    for function in names:
        results = grouped[function]
        count = sum(results)
        status = "untested" if not results else "PASS" if count == len(results) else "FAIL"
        lines.append(f"| `{function}` | {count} | {len(results)} | {status} |")
    lines += ["", "## Readings", "", "| Function | Case | Retail | OpenEgg | Result |",
              "| --- | --- | --- | --- | --- |"]
    for function, name, retail, local, match in rows:
        lines.append(f"| `{function}` | {name} | `{retail}` | `{local}` | "
                     f"{'PASS' if match else 'FAIL'} |")
    if problems:
        lines += ["", "## Capture problems", ""] + [f"- {problem}" for problem in problems]
    args.report.write_text("\n".join(lines) + "\n")
    print(f"audio: {passed}/{len(rows)} matched, {len(problems)} capture problems")
    print(f"Report: {args.report}")
    return 1 if passed != len(rows) or problems else 0


if __name__ == "__main__":
    sys.exit(main())
