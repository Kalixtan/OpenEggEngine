#!/usr/bin/env python3
"""Test desktop account login against a disposable local server."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading

ROOT = Path(__file__).resolve().parents[2]
USER = "00000000-0000-0000-0000-000000000002"
SESSION = "00000000-0000-0000-0000-000000000001"
EMAIL = "probe@example.test"
PASSWORD = 'pass#word;"quoted"'
seen = []

class Handler(BaseHTTPRequestHandler):
    def reply(self, status, body=b""):
        self.send_response(status)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        if self.path == "/identity/login/account":
            credentials = json.loads(body)
            seen.append(credentials == {"email": EMAIL, "password": PASSWORD})
            if not seen[-1]:
                self.reply(403)
                return
            self.reply(201, json.dumps({"session": {"id": SESSION, "person": {"id": USER, "display_name": "openegg_probe"}}}).encode())
        else:
            self.reply(200)

    def do_GET(self):
        if self.headers.get("X-OTG-Identity-SessionId") != SESSION:
            self.reply(403)
        elif self.path.endswith("/~friends.all"):
            self.reply(200, b'{"friends":[]}')
        elif self.path.endswith("/~translation.get"):
            self.reply(200)
        else:
            self.reply(200, b'{"items":[]}')

    def log_message(self, *args):
        pass

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server-url", help="Disposable local server with the synthetic fixture account")
    args = parser.parse_args()
    server = None
    if args.server_url:
        if not args.server_url.startswith("http://127.0.0.1:"):
            parser.error("fixture server must use local 127.0.0.1")
        url = args.server_url.rstrip("/")
    else:
        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        server.daemon_threads = True
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        url = f"http://127.0.0.1:{server.server_port}"
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-account-") as directory:
            root = Path(directory)
            engine = ROOT / "build/OpenEgg"
            shutil.copy2(engine, root / "OpenEgg")
            for asset in engine.parent.glob("*.psarc"):
                (root / asset.name).symlink_to(asset)
            data = engine.parent / "game_data"
            if data.is_dir():
                (root / "game_data").symlink_to(data, target_is_directory=True)
            harness = root / "harness.txt"
            harness.write_text(f"wait {600 if args.server_url else 180}\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                       SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
            cases = [(url, PASSWORD, True)] if args.server_url else [
                (url, PASSWORD, True), (url, "wrong", False),
                ("http://localhost.example.invalid", PASSWORD, False),
                ("http://127.0.0.1.example.invalid", PASSWORD, False),
                ("http://localhost@remote.invalid", PASSWORD, False),
            ]
            for case_url, password, valid in cases:
                (root / "OpenEgg.ini").write_text(f"[network]\nserver_url = {case_url}\naccount_email = {EMAIL}\naccount_password = {password}\n")
                result = subprocess.run([str(root / "OpenEgg"), str(ROOT / "tests/networking"), "AccountLoginProbe.gm", "Main"],
                                        env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=45)
                print(result.stdout, end="")
                markers = ("account name PASS", "account request PASS", "account reconnect PASS", "account coordinator PASS", "account platform state PASS", "account platform persists PASS") if valid else ("account rejection PASS",)
                markers += ("account platform initial PASS",)
                if result.returncode or any(marker not in result.stdout for marker in markers) or "account request FAIL" in result.stdout:
                    raise SystemExit("account login probe failed")
            if server and seen != [True, True, False]:
                raise SystemExit("account requests or credential handling did not match")
    finally:
        if server:
            server.shutdown()
            server.server_close()
            thread.join()

if __name__ == "__main__":
    main()
