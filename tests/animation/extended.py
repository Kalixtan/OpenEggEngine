#!/usr/bin/env python3
"""Run additional animation resource and entity clock probes headlessly."""

import argparse
from datetime import datetime, timezone
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
READING = re.compile(r"OEP__(Animation\..*?)__(.*?)\.gm")
ASSETS = {
    "checkpoint": Path("Platformer/gameAssets/checkpointTriggeredInactive.fsa"),
    "ps3_records": Path("PS3_Platformer/gameAssets/goalTriggered.fsa"),
}


def read_baseline(path):
    values = {}
    if not path.is_file():
        return values
    for line in path.read_text(errors="replace").splitlines():
        if line and not line.startswith("#") and "\t" in line:
            name, value = line.split("\t", 1)
            values[name] = value
    return values


def baseline_matches(path, probe, support=()):
    if not path.is_file():
        return False
    metadata = {}
    for line in path.read_text(errors="replace").splitlines():
        if line.startswith("# probe-sha256 "):
            metadata["probe"] = line.split()[-1]
        elif line.startswith("# support-sha256 "):
            _, _, name, digest = line.split(maxsplit=3)
            metadata[name] = digest
    if metadata.get("probe") != hashlib.sha256(probe.read_bytes()).hexdigest():
        return False
    return all(metadata.get(path.name) == hashlib.sha256(path.read_bytes()).hexdigest()
               for path in support)


def run_probe(engine, game_data, probe, asset_paths, support=()):
    with tempfile.TemporaryDirectory(prefix="openegg-animation-") as temp:
        data_root = Path(temp)
        for relative in asset_paths:
            source = game_data / relative
            if not source.is_file():
                raise FileNotFoundError(source)
            target = data_root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
        for filename in support:
            shutil.copyfile(ROOT / "tests/animation" / filename,
                            data_root / filename)
        script = f"{probe.stem}.gm"
        shutil.copyfile(probe, data_root / script)
        harness = data_root / "harness"
        shutil.copyfile(ROOT / "tests/animation/harness", harness)
        env = os.environ.copy()
        env.update({"GE3_HEADLESS": "1", "GE3_TRACE": "1", "GE3_STRICT": "halt",
                    "SDL_AUDIODRIVER": "dummy", "LIBGL_ALWAYS_SOFTWARE": "1",
                    "GE3_HARNESS": str(harness)})
        result = subprocess.run(
            [str(engine), str(data_root), script, "Main"], cwd=ROOT, env=env,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            errors="replace", timeout=20, check=False)
    return {name: value for name, value in READING.findall(result.stdout)}, result


def equal(value, expected):
    try:
        return abs(float(value) - float(expected)) < 1e-5
    except (TypeError, ValueError):
        return value == expected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-data", type=Path, required=True,
                        help="extracted retail game_data directory")
    parser.add_argument("--engine", type=Path, default=ROOT / "build/OpenEgg")
    parser.add_argument("--report", type=Path,
                        default=ROOT / "reports/ANIMATION_EXTENDED.md")
    args = parser.parse_args()
    game_data = args.game_data.resolve()
    engine = args.engine.resolve()
    for relative in ASSETS.values():
        if not (game_data / relative).is_file():
            parser.error(f"animation asset not found: {game_data / relative}")
    if not engine.is_file():
        parser.error(f"engine not found: {engine}")

    records, records_run = run_probe(
        engine, game_data, ROOT / "tests/animation/record_boundaries.gm",
        [ASSETS["ps3_records"]])
    negative, negative_run = run_probe(
        engine, game_data, ROOT / "tests/animation/negative_frame.gm",
        [ASSETS["checkpoint"]])
    entity, entity_run = run_probe(
        engine, game_data, ROOT / "tests/animation/entity_clock.gm",
        [ASSETS["checkpoint"]], ["AnimationProbeEntity.gm"])

    records_expected = {
        "Animation.records.images": "19", "Animation.records.frames": "19",
        "Animation.records.first": "0", "Animation.records.middle": "9",
        "Animation.records.last": "18", "Animation.records.clamped": "18",
    }
    entity_expected = {
        "Animation.entity.enabled.initial": "1",
        "Animation.entity.enabled.disabled": "0",
        "Animation.entity.disabled.clockStopped": "1",
        "Animation.entity.enabled.restored": "1",
        "Animation.entity.enabled.clockResumed": "1",
    }
    negative_started = negative.get("Animation.negativeFrame.started") == "1"
    negative_returned = negative.get("Animation.negativeFrame.returned") == "1"
    warning = ("WARNING: Animation.setCurrentFrame(-1) would fault retail "
               "(observed PPU access violation at 0x000b6848 reading 0x38600004)")
    negative_safe = (negative_started and not negative_returned and
                     negative_run.returncode == 0 and warning in negative_run.stdout and
                     "Segmentation fault" not in negative_run.stdout and
                     "Access violation" not in negative_run.stdout)
    records_probe = ROOT / "tests/animation/record_boundaries.gm"
    entity_probe = ROOT / "tests/animation/entity_clock.gm"
    negative_probe = ROOT / "tests/animation/negative_frame.gm"
    records_baseline = ROOT / "tests/animation/records-retail.tsv"
    entity_baseline = ROOT / "tests/animation/entity-clock-retail.tsv"
    negative_baseline = ROOT / "tests/animation/negative-frame-retail.tsv"
    crash_record = negative_baseline.read_text(errors="replace")
    checks = {
        **{name: equal(records.get(name), expected)
           for name, expected in records_expected.items()},
        **{name: equal(entity.get(name), expected)
           for name, expected in entity_expected.items()},
        "negative index does not crash OpenEgg": negative_safe,
        "retail negative-index fault is recorded":
            "# retail-fault ppu=0x000b6848 read=0x38600004" in crash_record,
        "retail probe captures are current":
            baseline_matches(records_baseline, records_probe) and
            baseline_matches(entity_baseline, entity_probe,
                             [ROOT / "tests/animation/AnimationProbeEntity.gm"]) and
            baseline_matches(negative_baseline, negative_probe),
        "retail-fault warning is limited to negative index":
            warning not in records_run.stdout and warning not in entity_run.stdout,
    }

    baselines = {
        "records": read_baseline(ROOT / "tests/animation/records-retail.tsv"),
        "entity": read_baseline(ROOT / "tests/animation/entity-clock-retail.tsv"),
        "negative": read_baseline(ROOT / "tests/animation/negative-frame-retail.tsv"),
    }
    all_readings = {**records, **entity, **negative}
    retail = {**baselines["records"], **baselines["entity"], **baselines["negative"]}
    compared = [(name, value, retail[name]) for name, value in all_readings.items()
                if name in retail]
    matched = sum(equal(value, expected) for _, value, expected in compared)

    lines = [
        "# Animation edge probes", "",
        f"Generated: {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')}",
        f"Local checks: **{sum(checks.values())}/{len(checks)} passed**.",
        (f"Captured retail readings: **{matched}/{len(compared)} matched**."
         if compared else "Captured retail readings: pending."), "",
        "## Checks", "", "| Check | Result |", "| --- | --- |",
    ]
    lines.extend(f"| {name} | {'PASS' if passed else 'FAIL'} |"
                 for name, passed in checks.items())
    lines.extend(["", "## Readings", "", "| Probe | OpenEgg | Retail | Match |",
                  "| --- | --- | --- | --- |"])
    for name, value in sorted(all_readings.items()):
        reference = retail.get(name, "pending")
        match = "pending" if reference == "pending" else (
            "yes" if equal(value, reference) else "no")
        lines.append(f"| `{name}` | `{value}` | `{reference}` | {match} |")
    lines.extend(["", "Negative frame call returned to script: "
                  f"**{'yes' if negative_returned else 'no'}**.",
                  "Retail fault confirmed: PPU `0x000b6848` read unmapped `0x38600004`.",
                  "The invalid-index case stays isolated so its retail fault cannot stop other probes.", ""])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text("\n".join(lines))
    print(f"Animation edge probes: {sum(checks.values())}/{len(checks)} local checks; "
          f"{matched}/{len(compared)} retail readings matched. Report: {args.report}")
    for label, result in (("records", records_run), ("negative frame", negative_run),
                          ("entity clock", entity_run)):
        if result.returncode:
            print(f"{label} probe exited {result.returncode}:\n{result.stdout}")
    return 0 if all(checks.values()) and matched == len(compared) else 1


if __name__ == "__main__":
    raise SystemExit(main())
