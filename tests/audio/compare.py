#!/usr/bin/env python3
"""Compare audio script readings from retail and OpenEgg."""

import argparse
from collections import defaultdict
import csv
from datetime import datetime, timezone
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from compare_probe import check_baseline_source, read_readings
from probe_expectations import expected_readings
from run import ROOT, percent, run_script

HERE = Path(__file__).resolve().parent
SOUND = Path("Platformer/sounds/fx/button-high.wav")
STREAM_SOUND = Path("Platformer/levels/gamescom_master_8.wav")
REPORT = ROOT / "reports" / "AUDIO.md"
RETAIL_WARNING_PREFIX = "# retail-warning "
VOLATILE_READING = re.compile(
    r'ProbeVolatile\("([A-Za-z0-9_.]+)"')


def retail_setup_warnings(baselines):
    warnings = []
    for baseline in baselines:
        for line in baseline.read_text(errors="replace").splitlines():
            if not line.startswith(RETAIL_WARNING_PREFIX):
                continue
            code = line[len(RETAIL_WARNING_PREFIX):]
            if code == "fmod-invalid-seek" and baseline.stem == "slicker-retail":
                warnings.append(
                    "Retail Slicker setup failed: RPCS3 logged an FMOD invalid-seek "
                    "error. The captured play call returned 0 with voice volume 0; "
                    "these readings do not describe successful Slicker playback."
                )

    voice = HERE / "voice_controls-retail.tsv"
    if voice.exists():
        retail, _ = read_readings(voice)
        if (retail.get("Audio.playStream.preloaded") == "1" and
                retail.get("Audio.playStream.samplePositive") == "0" and
                retail.get("Audio.playStream.sampleLength") == "0"):
            warnings.append(
                "Retail playStream setup failed for the tested WAV: preload returned "
                "1, but playStream returned 0 and no voice length. Playback parity "
                "cannot be judged from this failed setup."
            )
    return warnings


def retail_setup_skips(baseline):
    retail, _ = read_readings(baseline)
    if (baseline.stem == "slicker-retail" and
            "# retail-warning fmod-invalid-seek" in
            baseline.read_text(errors="replace")):
        return {"Audio.playSoundSlicker.sameVoice",
                "Audio.playSoundSlicker.volume"}
    if (baseline.stem == "voice_controls-retail" and
            retail.get("Audio.playStream.preloaded") == "1" and
            retail.get("Audio.playStream.samplePositive") == "0" and
            retail.get("Audio.playStream.sampleLength") == "0"):
        return {"Audio.playStream.samplePositive",
                "Audio.playStream.sampleLength"}
    return set()


def inventory():
    with (ROOT / "docs/api/function_notes.tsv").open() as source:
        return sorted(row["name"] for row in csv.DictReader(source, delimiter="\t")
                      if row["name"].startswith("Audio."))


def compare_section(engine, probe, baseline, root, log_dir, names, support=()):
    check_baseline_source(baseline, probe, support)
    declared = expected_readings(probe)
    if len(declared) != len(set(declared)):
        raise ValueError(f"duplicate reading names in {probe}")
    _, error, log_path, _ = run_script(
        engine, root, probe.name, HERE / "smoke.harness", 15, log_dir,
        strict="survey")
    retail, retail_dupes = read_readings(baseline)
    local, local_dupes = read_readings(log_path)
    setup_skips = retail_setup_skips(baseline)
    volatile = set(VOLATILE_READING.findall(probe.read_text()))
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
                     name in retail and name in local and retail[name] == local[name],
                     name in setup_skips, name in volatile))
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
    baselines = [HERE / f"{stem}-retail.tsv" for stem in
                 ("state", "manager", "effects", "script_playback",
                  "resources", "voice", "voice_controls", "slicker", "voice_lifetime",
                  "voice_gate")]
    warnings = retail_setup_warnings(baselines)
    log_dir = ROOT / "tests/_results/audio"
    log_dir.mkdir(parents=True, exist_ok=True)
    rows, problems = [], []
    for stem in ("state", "manager", "effects", "script_playback"):
        section_rows, section_problems = compare_section(
            args.engine.resolve(), HERE / f"{stem}.gm",
            HERE / f"{stem}-retail.tsv", HERE, log_dir, names)
        rows.extend(section_rows)
        problems.extend(section_problems)
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
            stream_source = args.game_data.expanduser().resolve() / STREAM_SOUND
            if not stream_source.is_file():
                parser.error(f"stream sample missing: {stream_source}")
            stream_target = root / STREAM_SOUND
            stream_target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(stream_source, stream_target)
            for stem in ("resources", "voice", "voice_controls", "slicker", "voice_lifetime",
                         "voice_gate"):
                shutil.copyfile(HERE / f"{stem}.gm", root / f"{stem}.gm")
                support = (HERE / "sampleList.txt",) if stem == "resources" else ()
                for support_file in support:
                    shutil.copyfile(support_file, root / support_file.name)
                section_rows, section_problems = compare_section(
                    args.engine.resolve(), HERE / f"{stem}.gm",
                    HERE / f"{stem}-retail.tsv", root, log_dir, names, support)
                rows.extend(section_rows)
                problems.extend(section_problems)
        asset_status = "tested with user supplied sound"

    grouped = defaultdict(list)
    for row in rows:
        function, _, _, _, passed_reading, setup_skip, clock = row
        grouped[function].append((passed_reading, setup_skip, clock))
    skipped = sum(row[5] for row in rows)
    clock_dependent = sum(row[6] for row in rows)
    compared = len(rows) - skipped - clock_dependent
    passed = sum(row[4] for row in rows if not row[5] and not row[6])
    tested = sum(bool(grouped[name]) for name in names)
    lines = ["# Audio test report", "",
             f"Generated: {datetime.now(timezone.utc):%Y-%m-%d %H:%M UTC}", "",
             f"Matched: **{passed}/{compared} ({percent(passed, compared)})** comparable script readings; "
             f"{skipped} skipped after retail setup failures; "
             f"{clock_dependent} clock-dependent readings excluded. "
             "Effect-chain calls without a getter only check completion. "
             "This does not test emitted audio or DSP parity.", "",
             f"Asset probes: {asset_status}. Functions with readings: **{tested}/{len(names)}**.", "",
             "## Retail setup warnings", ""]
    lines.extend(f"- **Warning:** {warning}" for warning in warnings)
    if not warnings:
        lines.append("- None detected in the captured retail readings.")
    lines += [
             "",
             "## Per function", "",
             "| Function | Matched | Compared | Setup skip | Clock | Status |",
             "| --- | ---: | ---: | ---: | ---: | --- |"]
    for function in names:
        results = grouped[function]
        count = sum(passed_reading for passed_reading, skip, clock in results
                    if not skip and not clock)
        cases = sum(not skip and not clock for _, skip, clock in results)
        setup_ignored = sum(skip for _, skip, _ in results)
        clock_ignored = sum(clock for _, _, clock in results)
        status = ("untested" if not results else "CLOCK" if not cases and clock_ignored else
                  "SKIP" if not cases else "PASS" if count == cases else "FAIL")
        lines.append(f"| `{function}` | {count} | {cases} | {setup_ignored} | "
                     f"{clock_ignored} | {status} |")
    lines += ["", "## Readings", "", "| Function | Case | Retail | OpenEgg | Result |",
              "| --- | --- | --- | --- | --- |"]
    for function, name, retail, local, match, skip, clock in rows:
        result = "SETUP SKIP" if skip else "CLOCK" if clock else "PASS" if match else "FAIL"
        lines.append(f"| `{function}` | {name} | `{retail}` | `{local}` | "
                     f"{result} |")
    if problems:
        lines += ["", "## Capture problems", ""] + [f"- {problem}" for problem in problems]
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text("\n".join(lines) + "\n")
    print(f"audio: {passed}/{compared} comparable readings matched; "
          f"{skipped} setup skips, {clock_dependent} clock-dependent, "
          f"{len(problems)} capture problems")
    for warning in warnings:
        print(f"WARNING: {warning}")
    print(f"Report: {args.report}")
    return 1 if passed != compared or problems else 0


if __name__ == "__main__":
    sys.exit(main())
