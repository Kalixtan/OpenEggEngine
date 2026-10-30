#!/usr/bin/env python3
"""Check INI session writes preserve unreadable files and create missing ones."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    if os.name != "posix":
        print("SKIP: this probe checks POSIX file permissions")
        return

    compiler = shutil.which("g++", path="/usr/bin:/bin")
    if not compiler:
        raise SystemExit("g++ is required for this probe")

    host_env = os.environ.copy()
    host_env["PATH"] = "/usr/bin:/bin"
    with tempfile.TemporaryDirectory(prefix="openegg-ini-write-") as name:
        temp = Path(name)
        probe = temp / "ini-write-probe"
        subprocess.run([
            compiler, "-std=c++17", "-I", str(ROOT / "src/core"),
            str(ROOT / "tests/networking/IniWriteProbe.cpp"),
            str(ROOT / "src/core/system_config.cpp"), "-o", str(probe),
        ], check=True, env=host_env)

        ini = temp / "OpenEgg.ini"
        original = b"[network]\nserver_url = https://example.invalid\npsn_online_id = ProbeID\n"
        ini.write_bytes(original)
        ini.chmod(0)
        try:
            subprocess.run([str(probe), str(ini), "unreadable"], check=True)
        finally:
            ini.chmod(0o600)
        if ini.read_bytes() != original:
            raise SystemExit("unreadable existing INI was changed")
        print("unreadable existing INI is preserved PASS")

        missing = temp / "missing.ini"
        subprocess.run([str(probe), str(missing), "missing"], check=True)
        print("missing INI accepts saved session PASS")


if __name__ == "__main__":
    main()
