#!/usr/bin/env python3
"""Capture retail audio once, then compare OpenEgg's PCM against it."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

import numpy as np
from scipy.ndimage import maximum_filter1d
from scipy.signal import correlate
import soundfile as sf

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
SOUND = Path("Platformer/sounds/fx/button-high.wav")
SUITES = {
    "basic": ("waveform.gm", ("normal", "half_volume", "pan_left", "pan_right")),
    "controls": ("waveform_controls.gm", ("speed_half", "speed_double",
                                  "fade_40", "loop_one", "pause_resume")),
}
RATE = 48000
RESULTS = ROOT / "tests/_results/audio/waveforms"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_raw(path):
    data = np.fromfile(path, dtype="<f4")
    if len(data) % 2 or len(data) < RATE:
        raise RuntimeError(f"empty or incomplete stereo capture: {path}")
    return data.reshape(-1, 2)


def save_cases(raw, target, cases, fixed_spacing=False):
    # Merge gaps within each short hit, then find isolated events.
    peak = maximum_filter1d(np.max(np.abs(raw), axis=1), size=240)
    active = np.flatnonzero(peak > 0.008)
    if len(active) == 0:
        raise RuntimeError("capture is silent")
    if fixed_spacing:
        starts = active[0] + np.arange(len(cases)) * RATE
    else:
        starts = np.r_[active[0], active[1:][np.diff(active) > RATE // 3]]
    if len(starts) != len(cases):
        raise RuntimeError(f"expected {len(cases)} isolated sounds, found {len(starts)}")
    target.mkdir(parents=True, exist_ok=True)
    for name, start in zip(cases, starts):
        first = max(0, int(start) - 240)
        clip = raw[first:first + RATE // 4]
        if len(clip) != RATE // 4:
            raise RuntimeError(f"{name}: recording ended before the sound finished")
        sf.write(target / f"{name}.wav", clip, RATE, subtype="FLOAT")
    return starts / RATE


def capture_retail(args, sound, target, probe, cases):
    main = args.main.resolve()
    eboot = args.eboot.resolve()
    if not main.is_file() or not eboot.is_file():
        raise RuntimeError("--main and --eboot must point to the installed PS3 game")
    if not (main.parent / SOUND).is_file():
        raise RuntimeError(f"retail sample missing beside Main.gm: {SOUND}")
    original = main.read_bytes()
    if b"global Main" not in probe.read_bytes():
        raise RuntimeError("waveform probe has no Main entry point")
    config = Path.home() / ".config/rpcs3/config.yml"
    settings = config.read_text()
    if 'Renderer: "Null"' not in settings and 'Renderer: "Cubeb"' not in settings:
        raise RuntimeError("unknown RPCS3 audio renderer setting")
    settings = settings.replace('Renderer: "Null"', 'Renderer: "Cubeb"')
    target.mkdir(parents=True, exist_ok=True)
    raw_path = target / "retail-full.f32le"
    tty = Path.home() / ".cache/rpcs3/TTY.log"
    sink = f"openegg_wave_{os.getpid()}"
    settings = settings.replace('Audio Device: "@@@default@@@"',
                                f'Audio Device: "{sink}"')
    module = subprocess.check_output(
        ["pactl", "load-module", "module-null-sink", f"sink_name={sink}",
         "rate=48000", "channels=2"], text=True).strip()
    with tempfile.TemporaryDirectory(prefix="openegg-wave-") as temporary:
        config_copy = Path(temporary) / "rpcs3.yml"
        config_copy.write_text(settings)
        env = os.environ.copy()
        env.update({"QT_QPA_PLATFORM": "offscreen", "PULSE_SINK": sink})
        recorder = emulator = None
        try:
            with raw_path.open("wb") as raw_file, (target / "retail-boot.log").open("wb") as log:
                recorder = subprocess.Popen(
                    ["parec", "--raw", f"--device={sink}.monitor",
                     "--format=float32le", "--rate=48000", "--channels=2"],
                    stdout=raw_file, stderr=log)
                time.sleep(0.3)
                if recorder.poll() is not None:
                    raise RuntimeError("Pulse monitor recording did not start")
                main.write_bytes(original + b"\n" + probe.read_bytes())
                tty.unlink(missing_ok=True)
                emulator = subprocess.Popen(
                    [str(args.rpcs3), "--no-gui", "--config", str(config_copy),
                     str(eboot)], env=env, stdout=log, stderr=subprocess.STDOUT)
                deadline = time.monotonic() + args.timeout
                done = False
                while time.monotonic() < deadline:
                    if tty.is_file() and b"OEP__wave_done__1" in tty.read_bytes():
                        done = True
                        time.sleep(1.5)
                        break
                    if emulator.poll() is not None:
                        break
                    time.sleep(0.25)
                if not done:
                    raise RuntimeError(f"retail probe did not finish; see {target / 'retail-boot.log'}")
        finally:
            if emulator and emulator.poll() is None:
                emulator.terminate()
                try:
                    emulator.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    emulator.kill()
                    emulator.wait()
            if recorder and recorder.poll() is None:
                recorder.terminate()
                recorder.wait(timeout=5)
            main.write_bytes(original)
            subprocess.run(["pactl", "unload-module", module], check=True)
    starts = save_cases(load_raw(raw_path), target / "retail", cases,
                        args.suite == "controls")
    (target / "retail.json").write_text(json.dumps({
        "probe_sha256": sha(probe), "sound_sha256": sha(sound),
        "eboot_sha256": sha(eboot), "rate": RATE,
        "cases": list(cases), "onsets_s": starts.tolist(),
    }, indent=2) + "\n")


def capture_openegg(args, sound, target, probe, cases):
    target.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="openegg-wave-data-") as temporary:
        root = Path(temporary)
        sample = root / SOUND
        sample.parent.mkdir(parents=True)
        shutil.copyfile(sound, sample)
        shutil.copyfile(probe, root / probe.name)
        harness = root / "waveform.harness"
        harness.write_text(f"wait {60 * len(cases) + 40}\nquit\n")
        raw_path = target / "openegg-full.f32le"
        env = os.environ.copy()
        env.update({"GE3_HEADLESS": "1", "GE3_AUDIO_CAPTURE_RAW": str(raw_path),
                    "GE3_HARNESS": str(harness), "GE3_STRICT": "survey",
                    "SDL_AUDIODRIVER": "dummy", "LIBGL_ALWAYS_SOFTWARE": "1"})
        result = subprocess.run(
            [str(args.engine.resolve()), str(root), probe.name, "Main"],
            cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            timeout=30, text=True, errors="replace", check=False)
        (target / "openegg.log").write_text(result.stdout)
        if result.returncode or "wave_done" not in result.stdout:
            raise RuntimeError(f"OpenEgg probe failed; see {target / 'openegg.log'}")
    return save_cases(load_raw(raw_path), target / "openegg", cases,
                      args.suite == "controls")


def compare(target, cases, report):
    lines = ["# Audio waveform report", "", "Retail source: installed PS3 game in RPCS3.",
             "Retail WAV files stay under ignored `tests/_results/audio/waveforms/`.", "",
             "Correlation uses aligned samples; RMS ratio is OpenEgg divided by retail.", "",
             "| Case | Shift ms | Correlation | RMS ratio | Left RMS ratio | Right RMS ratio | Result |",
             "| --- | ---: | ---: | ---: | ---: | ---: | --- |"]
    passed = 0
    for name in cases:
        retail, rate = sf.read(target / "retail" / f"{name}.wav", dtype="float32")
        local, local_rate = sf.read(target / "openegg" / f"{name}.wav", dtype="float32")
        if rate != RATE or local_rate != RATE:
            raise RuntimeError("capture sample rate changed")
        a = retail.mean(axis=1)
        b = local.mean(axis=1)
        shift = 0
        if np.max(np.abs(a)) < 1e-5 or np.max(np.abs(b)) < 1e-5:
            correlation = 0.0
        else:
            shift = int(np.argmax(correlate(a, b, mode="full", method="fft")) - (len(b) - 1))
            if abs(shift) > RATE // 50:
                correlation = 0.0
            else:
                if shift > 0:
                    a = a[shift:]
                    b = b[:-shift]
                    retail = retail[shift:]
                    local = local[:-shift]
                elif shift < 0:
                    a = a[:shift]
                    b = b[-shift:]
                    retail = retail[:shift]
                    local = local[-shift:]
                correlation = float(np.dot(a, b) / (np.linalg.norm(a) * np.linalg.norm(b) + 1e-12))
        rms = lambda x: float(np.sqrt(np.mean(x * x)))
        def ratio(reference, actual):
            if rms(reference) < 1e-5 and rms(actual) < 1e-5:
                return None
            return rms(actual) / max(rms(reference), 1e-9)
        total = ratio(retail, local)
        left = ratio(retail[:, 0], local[:, 0])
        right = ratio(retail[:, 1], local[:, 1])
        ok = correlation >= 0.99 and total is not None and 0.98 <= total <= 1.02
        ok = ok and all(value is None or 0.98 <= value <= 1.02
                        for value in (left, right))
        passed += ok
        show = lambda value: "silent" if value is None else f"{value:.3f}"
        lines.append(f"| {name} | {shift * 1000 / RATE:.1f} | {correlation:.3f} | {show(total)} | {show(left)} | "
                     f"{show(right)} | {'PASS' if ok else 'FAIL'} |")
    report.write_text("\n".join(lines) + f"\n\nMatched: **{passed}/{len(cases)}**. "
                      "Thresholds: correlation ≥0.99 and RMS within 2%. "
                      "This checks a few short sounds, not full audio parity.\n")
    print(f"waveforms: {passed}/{len(cases)} matched; {report}")
    return 0 if passed == len(cases) else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-data", type=Path, required=True)
    parser.add_argument("--suite", choices=SUITES, default="basic")
    parser.add_argument("--engine", type=Path, default=ROOT / "build/OpenEgg")
    parser.add_argument("--capture-retail", action="store_true")
    parser.add_argument("--main", type=Path)
    parser.add_argument("--eboot", type=Path)
    parser.add_argument("--rpcs3", type=Path, default=Path("/usr/bin/rpcs3"))
    parser.add_argument("--timeout", type=int, default=45)
    args = parser.parse_args()
    probe_name, cases = SUITES[args.suite]
    probe = HERE / probe_name
    target = RESULTS if args.suite == "basic" else RESULTS / args.suite
    report = ROOT / ("AUDIO_WAVEFORM_REPORT.md" if args.suite == "basic"
                     else "AUDIO_CONTROL_WAVEFORM_REPORT.md")
    sound = args.game_data.resolve() / SOUND
    if not sound.is_file():
        parser.error(f"sound missing: {sound}")
    if args.capture_retail:
        if not args.main or not args.eboot:
            parser.error("--capture-retail requires --main and --eboot")
        capture_retail(args, sound, target, probe, cases)
    metadata = target / "retail.json"
    if not metadata.is_file():
        parser.error("no saved retail capture; run with --capture-retail")
    saved = json.loads(metadata.read_text())
    if (saved["probe_sha256"] != sha(probe) or
            saved["sound_sha256"] != sha(sound) or saved["cases"] != list(cases)):
        parser.error("retail capture uses a different probe or sample; recapture it")
    capture_openegg(args, sound, target, probe, cases)
    return compare(target, cases, report)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(error, file=sys.stderr)
        sys.exit(2)
