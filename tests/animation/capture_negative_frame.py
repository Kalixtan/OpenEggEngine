#!/usr/bin/env python3
"""Capture the retail negative-frame case, which is expected to fault."""

import argparse
import hashlib
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
PROBE = ROOT / "tests/animation/negative_frame.gm"
BASELINE = ROOT / "tests/animation/negative-frame-retail.tsv"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--main", type=Path, required=True)
    parser.add_argument("--eboot", type=Path, required=True)
    parser.add_argument("--tty", type=Path,
                        default=Path.home() / ".cache/rpcs3/TTY.log")
    parser.add_argument("--rpcs3", type=Path, default=Path("/usr/bin/rpcs3"))
    parser.add_argument("--timeout", type=int, default=38)
    parser.add_argument("--baseline", type=Path, default=BASELINE)
    args = parser.parse_args()

    args.baseline.unlink(missing_ok=True)
    command = [sys.executable, str(ROOT / "tests/capture_retail_probe.py"),
               "--main", str(args.main), "--eboot", str(args.eboot),
               "--tty", str(args.tty), "--rpcs3", str(args.rpcs3),
               "--probe", str(PROBE), "--baseline", str(args.baseline),
               "--timeout", str(args.timeout)]
    capture = subprocess.run(command, cwd=ROOT, text=True,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                             check=False)
    if capture.returncode == 0:
        print("Retail returned from the negative frame call; captured full readings.")
        print(capture.stdout, end="")
        return 0

    logs = ROOT / "tests/_results/animation"
    boot = (logs / "negative_frame-retail-boot.log").read_text(errors="replace")
    tty = (logs / "negative_frame-retail-tty.log").read_text(errors="replace")
    started = "OEP__Animation.negativeFrame.started__1.gm" in tty
    returned = "OEP__Animation.negativeFrame.returned__1.gm" in tty
    fault = "VM: Access violation reading location 0x38600004" in boot
    if not (started and not returned and fault):
        print(capture.stdout, end="")
        print("Retail did not show the recorded negative-index fault.", file=sys.stderr)
        return 1

    digest = hashlib.sha256(PROBE.read_bytes()).hexdigest()
    args.baseline.parent.mkdir(parents=True, exist_ok=True)
    args.baseline.write_text(
        f"# probe-sha256 {digest}\n"
        "# retail-fault ppu=0x000b6848 read=0x38600004\n"
        "Animation.negativeFrame.started\t1\n")
    print("Retail fault confirmed after the probe marker; crash record updated.")
    print(capture.stdout, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
