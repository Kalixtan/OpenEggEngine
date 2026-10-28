#!/usr/bin/env python3
"""Test an unsigned local ticket against a server with authentication disabled."""
import argparse
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]


def ticket():
    def field(kind, data):
        return struct.pack(">HH", kind, len(data)) + data
    def u32(value):
        return struct.pack(">I", value)
    def u64(value):
        return struct.pack(">Q", value)
    now = int(time.time() * 1000)
    body = b"".join((field(8, b"OpenEggTest"), field(1, u32(0x100)),
                     field(7, u64(now)), field(7, u64(now + 3600000)),
                     field(2, u64(1)), field(4, b"openegg_probe"),
                     field(8, b"gb"), field(4, b"np"),
                     field(8, b"UP9000-NPUA70251_00-SOUNDSHAPES00000"),
                     u32(0), field(0, b""), field(0, b""), field(0, b"")))
    footer = field(8, b"test") + field(8, bytes.fromhex("3006020101020101"))
    return (struct.pack(">BBIH", 0x20, 1, 0, len(body) + len(footer) + 8)
            + b"\x30\x00" + struct.pack(">H", len(body)) + body
            + b"\x30\x02" + struct.pack(">H", len(footer)) + footer)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("server_url", help="Local server with RequireAuthentication=false")
    parser.add_argument("--engine", type=Path, default=ROOT / "build" / "OpenEgg")
    parser.add_argument("--invalid-ticket", action="store_true", help="Check rejection of a malformed ticket")
    parser.add_argument("--expect-hello-failure", action="store_true", help="Check a failed hello response")
    parser.add_argument("--eula", action="store_true", help="Check EULA retrieval from the local server")
    parser.add_argument("--community-disabled", action="store_true", help="Disable desktop community permissions")
    args = parser.parse_args()
    if not args.server_url.startswith("http://127.0.0.1:"):
        parser.error("fixture must use a local 127.0.0.1 server")
    with tempfile.TemporaryDirectory(prefix="openegg-ticket-") as name:
        temp = Path(name)
        engine = args.engine.resolve()
        shutil.copy2(engine, temp / engine.name)
        for asset in engine.parent.glob("*.psarc"):
            (temp / asset.name).symlink_to(asset)
        data = engine.parent / "game_data"
        if data.is_dir():
            (temp / "game_data").symlink_to(data, target_is_directory=True)
        (temp / "ticket.bin").write_bytes(b"invalid" if args.invalid_ticket else ticket())
        (temp / "OpenEgg.ini").write_text("[network]\nserver_url = " + args.server_url.rstrip("/") + "\nticket_file = ticket.bin\n" +
                                          ("community_allowed = false\n" if args.community_disabled else ""))
        harness = temp / "harness.txt"
        harness.write_text("wait 240\nquit\n")
        env = os.environ.copy()
        env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt",
                   GE3_HARNESS=str(harness), SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
        probe = "ServerEulaProbe.gm" if args.eula else "TicketLoginProbe.gm"
        result = subprocess.run([str(temp / engine.name), str(ROOT / "tests/networking"),
                                 probe, "Main"], cwd=ROOT, env=env,
                                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        print(result.stdout, end="")
        markers = ("ticket coordinator offline PASS", "ticket coordinator connecting PASS", "ticket coordinator cancel PASS",
                   "desktop community restricted PASS" if args.community_disabled else "desktop community permissions PASS")
        if args.eula:
            markers = ("network: ticket login succeeded", "network: identity hello succeeded",
                       "network: EULA received", "server EULA PASS", "network: friends synchronized", "server friends PASS",
                       "network: initial community data requests complete", "server coordinator online PASS")
        elif args.invalid_ticket:
            markers += ("network: ticket login failed (HTTP 400)", "ticket coordinator error PASS", "ticket platform ID unavailable PASS")
        elif args.expect_hello_failure:
            markers += ("network: ticket login succeeded", "network: identity hello failed (HTTP 503)",
                        "ticket coordinator error PASS", "ticket request rejected PASS")
        else:
            markers += ("ticket platform ID PASS", "network: ticket login succeeded", "network: identity hello succeeded", "ticket identity PASS",
                        "ticket authenticated request PASS", "ticket coordinator logout PASS")
        missing = [marker for marker in markers if marker not in result.stdout]
        failed_community = args.eula and ("network: featured request failed" in result.stdout or
                                          "network: news request failed" in result.stdout)
        if result.returncode or missing or failed_community or (args.invalid_ticket and "ticket identity PASS" in result.stdout):
            raise SystemExit(f"ticket probe failed; missing markers: {missing}")


if __name__ == "__main__":
    main()
