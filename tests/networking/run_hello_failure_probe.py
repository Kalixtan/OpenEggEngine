#!/usr/bin/env python3
"""Check the coordinator's identity hello error path with a local HTTP stub."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import sys
import threading

ROOT = Path(__file__).resolve().parents[2]
SESSION = "00000000-0000-0000-0000-000000000001"
USER = "00000000-0000-0000-0000-000000000002"
hello = threading.Event()


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        self.rfile.read(int(self.headers.get("Content-Length", 0)))
        body = ('{"session":{"id":"' + SESSION + '","person":{"id":"' + USER + '","display_name":"openegg_probe"}}}').encode()
        self.send_response(201)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if (self.path == "/otg/~identity:*.hello" and
                self.headers.get("X-OTG-Identity-SessionId") == SESSION):
            hello.set()
        self.send_response(503)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def log_message(self, *args):
        pass


def main():
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        result = subprocess.run([sys.executable, str(ROOT / "tests/networking/run_ticket_probe.py"),
                                 f"http://127.0.0.1:{server.server_port}", "--expect-hello-failure"],
                                cwd=ROOT, timeout=40)
        if result.returncode or not hello.is_set():
            raise SystemExit("hello failure probe failed")
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


if __name__ == "__main__":
    main()
