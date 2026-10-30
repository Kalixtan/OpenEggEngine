#!/usr/bin/env python3
"""Probe PSN ticket login against a local OpenEgg server."""
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
    parser.add_argument("server_url", help="Local server URL")
    parser.add_argument("--engine", type=Path, default=ROOT / "build" / "OpenEgg")
    parser.add_argument("--invalid-ticket", action="store_true", help="Check rejection of a malformed ticket")
    parser.add_argument("--ticket-in-ini", action="store_true", help="Store the ticket as inline INI hex")
    parser.add_argument("--identity-mismatch", action="store_true", help="Reject a ticket for a different Online ID")
    parser.add_argument("--expect-hello-failure", action="store_true", help="Check a failed hello response")
    parser.add_argument("--expect-auth-rejection", action="store_true",
                        help="Check that the server rejects the unsigned fixture ticket")
    parser.add_argument("--expect-session-saved", action="store_true",
                        help="Check ticket login persistence and reuse on the next launch")
    parser.add_argument("--expect-session-rejected", action="store_true",
                        help="Check that a rejected saved session is removed from the INI")
    parser.add_argument("--insecure-transport", action="store_true",
                        help="Reject ticket login over remote plaintext HTTP")
    parser.add_argument("--eula", action="store_true", help="Check EULA retrieval from the local server")
    parser.add_argument("--community-disabled", action="store_true", help="Disable desktop community permissions")
    args = parser.parse_args()
    if args.insecure_transport:
        if not args.server_url.startswith("http://openegg-probe.invalid"):
            parser.error("insecure transport probe must use the reserved .invalid hostname")
    elif not args.server_url.startswith("http://127.0.0.1:"):
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
        ticket_data = b"invalid" if args.invalid_ticket else ticket()
        config = "[network]\nserver_url = " + args.server_url.rstrip("/") + "\n"
        expected_id = "different_id" if args.identity_mismatch else "openegg_probe"
        config += "psn_online_id = " + expected_id + "\n"
        if args.expect_session_rejected:
            config += "server_session_id = expired-session\nserver_user_id = remembered-user-id\n"
        elif args.ticket_in_ini:
            config += "psn_ticket_hex = " + ticket_data.hex() + "\n"
        else:
            (temp / "ticket.bin").write_bytes(ticket_data)
            config += "ticket_file = ticket.bin\n"
        config += "community_allowed = false\n" if args.community_disabled else ""
        (temp / "OpenEgg.ini").write_text(config)
        harness = temp / "harness.txt"
        harness.write_text("wait 240\nquit\n")
        env = os.environ.copy()
        env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt",
                   GE3_HARNESS=str(harness), SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
        probe = ("RememberedSessionProbe.gm" if args.expect_session_saved or args.expect_session_rejected else
                 "ServerEulaProbe.gm" if args.eula else "TicketLoginProbe.gm")
        command = [str(temp / engine.name), str(ROOT / "tests/networking"), probe, "Main"]
        result = subprocess.run(command, cwd=ROOT, env=env,
                                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        combined_output = result.stdout
        second_result = None
        if args.expect_session_saved:
            saved_config = (temp / "OpenEgg.ini").read_text()
            for setting, expected in (("server_session_id", "remembered-session"),
                                      ("server_user_id", "remembered-user-id")):
                if f"{setting} = {expected}" not in saved_config:
                    raise SystemExit(f"ticket probe failed; {setting} was not saved")
            if os.name != "nt" and (temp / "OpenEgg.ini").stat().st_mode & 0o777 != 0o600:
                raise SystemExit("ticket probe failed; saved session file is not private")
            config_lines = [line for line in saved_config.splitlines()
                            if not line.lstrip().startswith(("ticket_file =", "psn_ticket_hex ="))]
            (temp / "OpenEgg.ini").write_text("\n".join(config_lines) + "\n")
            (temp / "ticket.bin").unlink(missing_ok=True)
            second_result = subprocess.run(command, cwd=ROOT, env=env,
                                           text=True, stdout=subprocess.PIPE,
                                           stderr=subprocess.STDOUT, timeout=30)
            combined_output += second_result.stdout
        elif args.expect_session_rejected:
            result_config = (temp / "OpenEgg.ini").read_text()
            if "server_session_id =" in result_config or "server_user_id =" in result_config:
                raise SystemExit("ticket probe failed; rejected session remained in OpenEgg.ini")
        print(combined_output, end="")
        markers = ("ticket coordinator offline PASS", "ticket coordinator cancel PASS",
                   "desktop community restricted PASS" if args.community_disabled else "desktop community permissions PASS")
        if not args.identity_mismatch and not args.invalid_ticket and not args.insecure_transport and not args.expect_auth_rejection:
            markers += ("ticket coordinator connecting PASS",)
        if args.expect_session_saved:
            markers = ("network: ticket login succeeded",
                       "network: replacement-server session saved to OpenEgg.ini",
                       "network: remembered server session validated",
                       "remembered identity ready PASS")
        elif args.expect_session_rejected:
            markers = ("network: remembered server session validation failed (HTTP 403)",
                       "remembered identity rejected PASS")
        elif args.eula:
            markers = ("network: ticket login succeeded", "network: identity hello succeeded",
                       "network: EULA received", "server EULA PASS", "network: friends synchronized", "server friends PASS",
                       "network: initial community data requests complete", "server coordinator online PASS",
                       "server friend mapping PASS", "replacement auth distinct from PSN PASS")
        elif args.identity_mismatch:
            markers += ("ticket platform ID unavailable PASS", "network: ticket Online ID does not match psn_online_id",
                        "ticket coordinator error PASS")
        elif args.expect_auth_rejection:
            markers += ("ticket coordinator error PASS",)
        elif args.invalid_ticket:
            markers += ("network: ticket Online ID does not match psn_online_id",
                        "ticket coordinator error PASS", "ticket platform ID unavailable PASS")
        elif args.insecure_transport:
            markers += ("network: ticket login requires HTTPS for remote servers",
                        "ticket coordinator error PASS")
            if "network: ticket login succeeded" in result.stdout:
                raise SystemExit("ticket transport probe failed; insecure login reached server")
        elif args.expect_hello_failure:
            markers += ("network: ticket login succeeded", "network: identity hello failed (HTTP 503)",
                        "ticket coordinator error PASS", "ticket request rejected PASS")
        else:
            markers += ("ticket platform ID PASS", "network: ticket login succeeded", "network: identity hello succeeded", "ticket identity PASS",
                        "ticket authenticated request PASS", "ticket coordinator logout PASS")
        missing = [marker for marker in markers if marker not in combined_output]
        failed_community = args.eula and ("network: featured request failed" in combined_output or
                                          "network: news request failed" in combined_output)
        if result.returncode or (second_result and second_result.returncode) or missing or failed_community or (args.invalid_ticket and "ticket identity PASS" in combined_output):
            raise SystemExit(f"ticket probe failed; missing markers: {missing}")
        if args.expect_session_saved and "network: ticket login succeeded" in second_result.stdout:
            raise SystemExit("ticket probe failed; second launch repeated ticket login")
        if args.expect_auth_rejection and ("network: ticket login succeeded" in combined_output or
                                           "ticket identity PASS" in combined_output):
            raise SystemExit("ticket rejection probe failed; unauthorized session was accepted")
        if args.expect_auth_rejection and not any(marker in combined_output for marker in (
                "network: replacement server rejected this PSN ticket",
                "network: ticket login failed (HTTP 403)")):
            raise SystemExit("ticket rejection probe failed; expected authorization rejection was not reported")


if __name__ == "__main__":
    main()
