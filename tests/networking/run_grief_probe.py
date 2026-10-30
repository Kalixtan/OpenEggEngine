#!/usr/bin/env python3
"""Probe report request construction using disposable local test data."""
import argparse
from email import policy
from email.parser import BytesParser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading

from run_ticket_probe import ticket

ROOT = Path(__file__).resolve().parents[2]
SESSION = "00000000-0000-0000-0000-000000000001"
USER = "00000000-0000-0000-0000-000000000002"
ITEM = "/~level:levelABC/~upload:42----BouNdaRy"
received = threading.Event()
report_countries = []


class Handler(BaseHTTPRequestHandler):
    def reply(self, status, body=b""):
        self.send_response(status)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        if self.path == "/identity/login/token/psn":
            self.reply(201, ('{"session":{"id":"' + SESSION + '","person":{"id":"' + USER +
                              '","display_name":"openegg_probe","openegg_psn_online_id":"openegg_probe"}},'
                              '"openegg_authorized":true,"openegg_remembered_session":true}').encode())
            return
        content_type = self.headers.get("Content-Type", "")
        if (self.path != "/otg/~grief:*.post" or self.headers.get("X-OTG-Identity-SessionId") != SESSION or
                content_type != "multipart/form-data; boundary=--fOsBKwDjKn"):
            self.reply(400)
            return
        message = BytesParser(policy=policy.default).parsebytes(("Content-Type: " + content_type + "\r\nMIME-Version: 1.0\r\n\r\n").encode() + body)
        parts = list(message.iter_parts())
        names = [part.get_param("name", header="content-disposition") for part in parts]
        values = [part.get_payload(decode=True) for part in parts]
        if (names == ["reportLanguageId", "reporterCountryCode", "item", "reportReasonId", "platformId", "consoleId"] and
                values == [b"1", b"gb", ITEM.encode(), b"1", b"1", b""] and
                all(part.get_filename() is None for part in parts)):
            report_countries.append(values[1])
            received.set()
            self.reply(200, b"{}")
        else:
            self.reply(400)

    def do_GET(self):
        self.reply(200 if self.path.startswith("/otg/~identity:*.hello") else 404)

    def log_message(self, *args):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server-url", help="Disposable local replacement server")
    parser.add_argument("--level-id", help="Level belonging to a different fixture account")
    args = parser.parse_args()
    server = None
    if args.server_url:
        if not args.server_url.startswith("http://127.0.0.1:") or not args.level_id or not args.level_id.isalnum():
            parser.error("local server URL and disposable fixture level ID required")
        url, item = args.server_url.rstrip("/"), f"/~level:{args.level_id}/~upload:42"
    else:
        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        server.daemon_threads = True
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        url, item = f"http://127.0.0.1:{server.server_port}", ITEM
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-grief-") as name:
            temp = Path(name)
            engine = ROOT / "build/OpenEgg"
            shutil.copy2(engine, temp / engine.name)
            for asset in engine.parent.glob("*.psarc"):
                (temp / asset.name).symlink_to(asset)
            data = engine.parent / "game_data"
            if data.is_dir():
                (temp / "game_data").symlink_to(data, target_is_directory=True)
            (temp / "ticket.bin").write_bytes(ticket())
            (temp / "OpenEgg.ini").write_text(f"[network]\nserver_url = {url}\npsn_online_id = openegg_probe\nticket_file = ticket.bin\n")
            script_dir = temp / "probe"
            script_dir.mkdir()
            source = (ROOT / "tests/networking/GriefProbe.gm").read_text().replace("__ITEM__", item)
            (script_dir / "GriefProbe.gm").write_text(source)
            harness = temp / "harness.txt"
            harness.write_text("wait 240\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                       SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
            command = [str(temp / engine.name), str(script_dir), "GriefProbe.gm", "Main"]
            result = subprocess.run(command, cwd=ROOT, env=env, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
            print(result.stdout, end="")
            markers = ("grief offline PASS", "grief boundary exhaustion PASS", "grief queued PASS", "grief response PASS")
            if result.returncode or (server and not received.is_set()) or any(marker not in result.stdout for marker in markers):
                raise SystemExit("grief probe failed")
            if server:
                ini = temp / "OpenEgg.ini"
                config = ini.read_text()
                if "psn_country = gb" not in config or f"server_session_id = {SESSION}" not in config:
                    raise SystemExit("grief probe failed; ticket country or saved session was not stored")
                (temp / "ticket.bin").unlink()
                ini.write_text("\n".join(line for line in config.splitlines()
                                          if not line.lstrip().startswith(("ticket_file =", "psn_ticket_hex ="))) + "\n")
                received.clear()
                second = subprocess.run(command, cwd=ROOT, env=env, text=True,
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
                print(second.stdout, end="")
                if (second.returncode or not received.is_set() or
                        "network: remembered server session validated" not in second.stdout or
                        "network: ticket login succeeded" in second.stdout or
                        report_countries != [b"gb", b"gb"]):
                    raise SystemExit("grief probe failed; later run did not use saved ticket country")
    finally:
        if server:
            server.shutdown()
            server.server_close()
            thread.join()


if __name__ == "__main__":
    main()
