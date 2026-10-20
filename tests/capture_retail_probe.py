#!/usr/bin/env python3
"""Capture a paired script probe from an installed PS3 game in RPCS3."""

import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import sys

from compare_probe import BASELINE, PROBE, ROOT, read_readings
from probe_expectations import expected_readings


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--main", type=Path, required=True,
                        help="installed extracted game_data/Main.gm")
    parser.add_argument("--eboot", type=Path, required=True,
                        help="installed EBOOT.BIN")
    parser.add_argument("--tty", type=Path,
                        default=Path.home() / ".cache/rpcs3/TTY.log")
    parser.add_argument("--rpcs3", type=Path, default=Path("/usr/bin/rpcs3"))
    parser.add_argument("--probe", type=Path, default=PROBE)
    parser.add_argument("--support-file", type=Path, action="append", default=[],
                        help="extra probe script copied beside retail Main.gm for this run")
    parser.add_argument("--expected", type=Path,
                        help="frame-sample plan for probes with generated names")
    parser.add_argument("--baseline", type=Path, default=BASELINE)
    parser.add_argument("--timeout", type=int, default=38)
    args = parser.parse_args()

    main_path = args.main.resolve()
    original = main_path.read_bytes()
    probe = args.probe.resolve().read_bytes()
    expected = set(expected_readings(args.probe.resolve(), args.expected))
    if not expected:
        parser.error("probe has no declared readings")
    if b"global Main" not in probe:
        parser.error("probe needs its own Main entry point")
    if not args.eboot.is_file():
        parser.error(f"missing EBOOT: {args.eboot}")

    support = []
    used = set()
    for source in args.support_file:
        target = main_path.parent / source.name
        if target == main_path or target in used:
            parser.error(f"duplicate or invalid support file: {source}")
        used.add(target)
        support.append((target, source.read_bytes(),
                        target.read_bytes() if target.exists() else None))

    try:
        section = args.probe.resolve().relative_to(ROOT / "tests").parts[0]
    except ValueError:
        section = "other"
    log_dir = ROOT / "tests/_results" / section
    log_dir.mkdir(parents=True, exist_ok=True)
    boot_log = log_dir / f"{args.probe.stem}-retail-boot.log"
    tty_log = log_dir / f"{args.probe.stem}-retail-tty.log"
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    try:
        for target, data, _ in support:
            target.write_bytes(data)
        main_path.write_bytes(original + b"\n" + probe)
        args.tty.unlink(missing_ok=True)
        with boot_log.open("wb") as output:
            try:
                subprocess.run([str(args.rpcs3), "--no-gui", str(args.eboot.resolve())],
                               env=env, stdout=output, stderr=subprocess.STDOUT,
                               timeout=args.timeout, check=False)
            except subprocess.TimeoutExpired:
                pass  # The game keeps running after the probe has finished.
    finally:
        main_path.write_bytes(original)
        for target, _, previous in support:
            if previous is None:
                target.unlink(missing_ok=True)
            else:
                target.write_bytes(previous)

    if not args.tty.is_file():
        print(f"no RPCS3 TTY log; see {boot_log}", file=sys.stderr)
        return 1
    tty_log.write_bytes(args.tty.read_bytes())
    if "VM: Access violation" in boot_log.read_text(errors="replace"):
        print(f"retail fault; see {boot_log}", file=sys.stderr)
        return 1
    tty_text = tty_log.read_text(errors="replace")
    readings, duplicates = read_readings(tty_log)
    if set(readings) != expected or duplicates:
        missing = len(expected - set(readings))
        extra = len(set(readings) - expected)
        print(f"retail capture incomplete: {missing} missing, {extra} extra, "
              f"{len(duplicates)} duplicates; see {tty_log}", file=sys.stderr)
        return 1
    source_hash = hashlib.sha256(probe).hexdigest()
    support_hashes = "".join(f"# support-sha256 {target.name} {hashlib.sha256(data).hexdigest()}\n"
                             for target, data, _ in support)
    retail_warnings = "# retail-warning fmod-invalid-seek\n" if (
        "FMOD error An invalid seek position was passed to this function." in tty_text
    ) else ""
    args.baseline.write_text(f"# probe-sha256 {source_hash}\n" + support_hashes +
                             retail_warnings +
                             "".join(f"{key}\t{value}\n"
                                     for key, value in sorted(readings.items())))
    print(f"captured {len(readings)} retail readings to {args.baseline}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
