#!/usr/bin/env python3
"""Check the local beta-agreement file binding headlessly."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
ENGINE = ROOT / "build/OpenEgg"

with tempfile.TemporaryDirectory(prefix="openegg-beta-") as directory:
    root = Path(directory)
    engine = root / "OpenEgg"
    shutil.copy2(ENGINE, engine)
    harness = root / "harness.txt"
    harness.write_text("wait 30\nquit\n")
    env = os.environ.copy()
    env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt",
               GE3_HARNESS=str(harness), SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
    template = (ROOT / "tests/networking/BetaAgreementProbe.gm").read_text()
    for case in ("missing", "present"):
        data = root / case
        data.mkdir()
        (data / "BetaAgreementProbe.gm").write_text(template.replace("__CASE__", case))
        if case == "present":
            (data / "betaparticipationagreement.txt").write_bytes(b"Synthetic agreement\0ignored")
        result = subprocess.run([str(engine), str(data), "BetaAgreementProbe.gm", "Main"], env=env,
                                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        print(result.stdout, end="")
        if result.returncode or f"beta agreement {case} PASS" not in result.stdout:
            raise SystemExit(f"beta agreement {case} probe failed")
