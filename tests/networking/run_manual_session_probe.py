#!/usr/bin/env python3
"""Test configured session login and rejection against a local server."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading

ROOT = Path(__file__).resolve().parents[2]
USER = "00000000-0000-0000-0000-000000000002"
SESSION = "00000000-0000-0000-0000-000000000001"
seen = []

class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        self.rfile.read(int(self.headers.get("Content-Length", 0)))
        self.send_response(200 if self.headers.get("X-OTG-Identity-SessionId") == SESSION else 403)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def do_GET(self):
        seen.append((self.path, self.headers.get("X-OTG-Identity-SessionId")))
        status = 200 if self.headers.get("X-OTG-Identity-SessionId") == SESSION else 403
        body = (b'{"friends":[]}' if self.path.endswith("/~friends.all") else
                b"" if self.path.endswith("/~translation.get") else b'{"items":[]}') if status == 200 else b""
        self.send_response(status)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args):
        pass

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, default=ROOT / "build/OpenEgg")
    parser.add_argument("--server-url", help="Disposable local server")
    parser.add_argument("--session-id", help="Fixture session ID")
    parser.add_argument("--user-id", help="Fixture identity ID")
    args = parser.parse_args()
    server = None
    if args.server_url:
        if not args.server_url.startswith("http://127.0.0.1:") or not args.session_id or not args.user_id:
            parser.error("local server, fixture session and user ID required")
        url = args.server_url.rstrip("/")
    else:
        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        server.daemon_threads = True
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        url = f"http://127.0.0.1:{server.server_port}"
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-manual-") as directory:
            root = Path(directory)
            engine = args.engine.resolve()
            shutil.copy2(engine, root / "OpenEgg")
            # The test runner can point to a temporary build while loading local game data.
            assets = ROOT / "build"
            for asset in assets.glob("*.psarc"):
                (root / asset.name).symlink_to(asset)
            data = assets / "game_data"
            if data.is_dir():
                (root / "game_data").symlink_to(data, target_is_directory=True)
            harness = root / "harness.txt"
            harness.write_text("wait 240\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                       SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
            cases = [(args.session_id, args.user_id, True)] if args.server_url else [(SESSION, USER, True), ("invalid", USER, False)]
            for session, user, valid in cases:
                (root / "OpenEgg.ini").write_text(f"[network]\nserver_url = {url}\nsession_id = {session}\nuser_id = {user}\n")
                result = subprocess.run([str(root / "OpenEgg"), str(ROOT / "tests/networking"), "ManualSessionProbe.gm", "Main"],
                                        env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
                print(result.stdout, end="")
                markers = ("manual session request PASS", "manual session logout PASS", "manual session reconnect PASS", "manual session coordinator PASS") if valid else ("manual session rejection PASS",)
                if result.returncode or any(marker not in result.stdout for marker in markers) or "manual session request FAIL" in result.stdout:
                    raise SystemExit("manual session probe failed")
            if server and sum(path == f"/otg/~identity:{USER}/~metadata:*.get" and auth == SESSION for path, auth in seen) != 2:
                raise SystemExit("manual session was not checked on both logins")
    finally:
        if server:
            server.shutdown()
            server.server_close()
            thread.join()

if __name__ == "__main__":
    main()
