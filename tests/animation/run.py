#!/usr/bin/env python3
"""Run animation resource/playback probes using user-supplied extracted assets."""

import argparse
from datetime import datetime, timezone
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
ASSET = Path("Platformer/gameAssets/checkpointTriggeredInactive.fsa")
READING = re.compile(r"OEP__(Animation\..*?)__(.*?)\.gm")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-data", type=Path, required=True,
                        help="extracted game_data directory; no assets are copied into the repo")
    parser.add_argument("--engine", type=Path, default=ROOT / "build/OpenEgg")
    parser.add_argument("--report", type=Path, default=ROOT / "tests/_results/animation.md")
    args = parser.parse_args()

    source = args.game_data.resolve() / ASSET
    engine = args.engine.resolve()
    if not source.is_file():
        parser.error(f"animation asset not found: {source}")
    if not engine.is_file():
        parser.error(f"engine not found: {engine}")

    with tempfile.TemporaryDirectory(prefix="openegg-animation-") as temp:
        data_root = Path(temp)
        target = data_root / ASSET
        target.parent.mkdir(parents=True)
        shutil.copyfile(source, target)
        entry_script = "AnimationProbeMain.gm"
        shutil.copyfile(ROOT / "tests/animation/probe.gm", data_root / entry_script)
        harness = data_root / "harness"
        shutil.copyfile(ROOT / "tests/animation/harness", harness)
        env = os.environ.copy()
        env.update({"GE3_HEADLESS": "1", "GE3_TRACE": "1",
                    "GE3_STRICT": "halt", "SDL_AUDIODRIVER": "dummy",
                    "LIBGL_ALWAYS_SOFTWARE": "1", "GE3_HARNESS": str(harness)})
        result = subprocess.run(
            [str(engine), str(data_root), entry_script, "Main"], cwd=ROOT,
            env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, errors="replace", timeout=15, check=False)

    readings = READING.findall(result.stdout)
    expected = [
        "Animation.getNumImages.initial", "Animation.getNumFrames.initial",
        "Animation.getSourcePixelWidth.initial", "Animation.getSourcePixelHeight.initial",
        "Animation.getCurrentFrame.initial", "Animation.getPlayTime.initial",
        "Animation.isPlaying.initial", "Animation.isPaused.initial", "Animation.isDone.initial",
        "Animation.getPlayTime.changed", "Animation.getPlayPosition.seek",
        "Animation.getCurrentFrame.seek", "Animation.getCurrentFrame.set",
        "Animation.isPlaying.play", "Animation.isPaused.play",
        "Animation.isPlaying.pause", "Animation.isPaused.pause",
        "Animation.isPlaying.playOnce", "Animation.isDone.end", "Animation.isPlaying.end",
        "Animation.isReversed.true", "Animation.getCurrentFrame.reversed",
        "Animation.isReversed.false", "Animation.getNumFrames.customSequence",
        "Animation.getCurrentFrame.sequenceFirst", "Animation.getCurrentFrame.sequenceSecond",
        "Animation.getNumImages.sharedResource", "Animation.getNumFrames.sharedSequence",
        "Animation.getNumImages.unloaded", "Animation.getNumImages.sharedStillLoaded",
        "Animation.getNumImages.afterUnloadAll", "Animation.getNumImages.missingResource",
    ]
    observed = {name: value for name, value in readings}
    baseline_path = ROOT / "tests/animation/retail-baseline.tsv"
    retail = {}
    if baseline_path.is_file():
        for line in baseline_path.read_text(errors="replace").splitlines():
            if not line or line.startswith("#") or "\t" not in line:
                continue
            name, value = line.split("\t", 1)
            retail[name] = value
    missing = [name for name in expected if name not in observed]
    local_checks = {
        "resource loaded": int(observed.get("Animation.getNumImages.initial", "0")) > 0,
        "frame count exposed": observed.get("Animation.getNumFrames.initial") == observed.get("Animation.getNumImages.initial"),
        "dimensions positive": float(observed.get("Animation.getSourcePixelWidth.initial", "0")) > 0 and
                               float(observed.get("Animation.getSourcePixelHeight.initial", "0")) > 0,
        "play time round trip": abs(float(observed.get("Animation.getPlayTime.changed", "nan")) - 120) < 0.01,
        "seek round trip": abs(float(observed.get("Animation.getPlayPosition.seek", "nan")) - 60) < 0.01,
        "play/pause states": observed.get("Animation.isPlaying.play") == "1" and
                             observed.get("Animation.isPaused.play") == "0" and
                             observed.get("Animation.isPlaying.pause") == "0" and
                             observed.get("Animation.isPaused.pause") == "1",
        "reverse state": observed.get("Animation.isReversed.true") == "1" and
                         observed.get("Animation.isReversed.false") == "0",
        "unload keeps object callable": observed.get("Animation.getNumImages.unloaded") == "0",
        "custom sequence frame selection": observed.get("Animation.getNumFrames.customSequence") == "2" and
            observed.get("Animation.getCurrentFrame.sequenceFirst") == "0" and
            observed.get("Animation.getCurrentFrame.sequenceSecond") == "34",
        "shared resource sequence state": observed.get("Animation.getNumImages.sharedResource") == "35" and
            observed.get("Animation.getNumFrames.sharedSequence") == "2",
        "cache clear preserves live handles": observed.get("Animation.getNumImages.sharedStillLoaded") == "35" and
            observed.get("Animation.getNumImages.afterUnloadAll") == "35",
        "missing resource stays empty": observed.get("Animation.getNumImages.missingResource") == "0",
    }
    checks_passed = sum(local_checks.values())
    comparable = [(name, value, retail[name]) for name, value in readings if name in retail]
    mismatches = []
    for name, value, reference in comparable:
        try:
            equal = abs(float(value) - float(reference)) <= 1e-5
        except ValueError:
            equal = value == reference
        if not equal:
            mismatches.append(name)
    report = [
        "# Animation probe", "",
        f"Generated: {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')}",
        (f"Retail comparison: **{len(comparable) - len(mismatches)}/{len(comparable)} matched**."
         if comparable else "Retail comparison: awaiting a successful retail capture."),
        f"Asset: `{ASSET}` (copied from the supplied extracted game data for this run).",
        f"Local checks: **{checks_passed}/{len(local_checks)} passed**.", "",
        "## Checks", "", "| Check | Result |", "| --- | --- |",
    ]
    report.extend(f"| {name} | {'PASS' if passed else 'FAIL'} |" for name, passed in local_checks.items())
    report.extend(["", "## Readings", "", "| Probe | OpenEgg | Retail | Match |", "| --- | --- | --- | --- |"])
    for name, value in readings:
        reference = retail.get(name)
        if reference is None:
            match = "pending"
            reference = "pending"
        else:
            try:
                match = "yes" if abs(float(value) - float(reference)) <= 1e-5 else "no"
            except ValueError:
                match = "yes" if value == reference else "no"
        report.append(f"| `{name}` | `{value}` | `{reference}` | {match} |")
    if mismatches:
        report.extend(["", "Retail mismatches: " + ", ".join(f"`{name}`" for name in mismatches)])
    if missing:
        report.extend(["", "Missing readings: " + ", ".join(f"`{name}`" for name in missing)])
    if result.returncode:
        report.extend(["", f"Engine exit code: `{result.returncode}`"])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text("\n".join(report) + "\n")
    print(f"Animation probe: {checks_passed}/{len(local_checks)} local checks passed; "
          f"{len(readings)}/{len(expected)} readings. Report: {args.report}")
    if result.returncode or missing or checks_passed != len(local_checks):
        print(result.stdout)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
