#!/usr/bin/env python3
"""Test EULA handling against a local HTTP fixture."""
import argparse
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
received = threading.Event()
fail = False


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        self.rfile.read(int(self.headers.get("Content-Length", 0)))
        body = ('{"session":{"id":"' + SESSION + '","person":{"id":"' + USER + '"}}}').encode()
        self.reply(201, body)

    def do_GET(self):
        if self.path == "/otg/~identity:*.hello":
            self.reply(200, b"")
        elif self.path == "/otg/ps3/SCEA/en/~eula.get":
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                received.set()
            self.reply(503 if fail else 200, b"" if fail else b"\xef\xbb\xbfOne\tTwo\n")
        else:
            self.reply(404, b"")

    def reply(self, code, body):
        self.send_response(code)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args):
        pass


def main():
    global fail
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fail", action="store_true", help="Return an HTTP error for EULA")
    parser.add_argument("--engine", type=Path, default=ROOT / "build" / "OpenEgg")
    args = parser.parse_args()
    fail = args.fail
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    server.daemon_threads = True
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-eula-") as name:
            temp = Path(name)
            engine = args.engine.resolve()
            shutil.copy2(engine, temp / engine.name)
            for asset in engine.parent.glob("*.psarc"):
                (temp / asset.name).symlink_to(asset)
            data = engine.parent / "game_data"
            if data.is_dir():
                (temp / "game_data").symlink_to(data, target_is_directory=True)
            (temp / "ticket.bin").write_bytes(ticket())
            (temp / "OpenEgg.ini").write_text(f"[network]\nserver_url = http://127.0.0.1:{server.server_port}\nticket_file = ticket.bin\n")
            harness = temp / "harness.txt"
            harness.write_text("wait 240\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                       SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
            result = subprocess.run([str(temp / engine.name), str(ROOT / "tests/networking"), "EulaProbe.gm", "Main"],
                                    cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, timeout=30)
            print(result.stdout, end="")
            markers = ("EULA failure PASS",) if fail else ("EULA text and digest PASS", "EULA acceptance PASS")
            if result.returncode or not received.is_set() or any(x not in result.stdout for x in markers) or "EULA acceptance version FAIL" in result.stdout:
                raise SystemExit("EULA probe failed")
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


if __name__ == "__main__":
    main()
