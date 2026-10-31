#!/usr/bin/env python3
"""Check saved OpenEgg account login against a local HTTP fixture."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading

ROOT = Path(__file__).resolve().parents[2]
SESSION = "00000000-0000-0000-0000-000000000001"
USER = "00000000-0000-0000-0000-000000000002"
EMAIL = "probe@example.test"
PASSWORD = "probe-password"
received_login = threading.Event()
valid_login = False


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        global valid_login
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        if self.path != "/identity/login/account":
            self.send_error(404)
            return
        try:
            data = json.loads(body)
        except ValueError:
            data = {}
        valid_login = (
            self.headers.get("X-OpenEgg-Client") == "OpenEgg" and
            self.headers.get("X-OpenEgg-Client-Version") == "0.1.0" and
            data == {"email": EMAIL, "password": PASSWORD}
        )
        received_login.set()
        response = json.dumps({
            "openegg_client": True,
            "openegg_authorized": True,
            "session": {"id": SESSION, "person": {
                "id": USER, "display_name": "OpenEgg_Probe"}},
        }).encode()
        self.send_response(201)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(response)))
        self.end_headers()
        self.wfile.write(response)

    def do_GET(self):
        if self.headers.get("X-OTG-Identity-SessionId") != SESSION:
            self.send_error(403)
            return
        body = b"[]" if self.path.endswith("~friends.all") else b"{}"
        if "~eula.get" in self.path:
            body = b"Welcome to the test server"
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *_):
        pass


def main():
    global valid_login
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        engine = ROOT / "build" / "OpenEgg"
        with tempfile.TemporaryDirectory(prefix="openegg-account-login-") as name:
            temp = Path(name)
            shutil.copy2(engine, temp / engine.name)
            for archive in engine.parent.glob("*.psarc"):
                (temp / archive.name).symlink_to(archive)
            game_data = engine.parent / "game_data"
            if game_data.is_dir():
                (temp / "game_data").symlink_to(game_data, target_is_directory=True)
            (temp / "OpenEgg.ini").write_text(
                f"[network]\nserver_url = http://127.0.0.1:{server.server_port}\n")
            (temp / "login.ini").write_text(
                f"[login]\nusername = OpenEgg_Probe\nemail = {EMAIL}\npassword = {PASSWORD}\n")
            harness = temp / "harness.txt"
            harness.write_text("wait 500\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt",
                       GE3_HARNESS=str(harness), SDL_AUDIODRIVER="dummy",
                       LIBGL_ALWAYS_SOFTWARE="1")
            result = subprocess.run(
                [str(temp / engine.name), str(ROOT / "tests/networking"),
                 "AccountLoginProbe.gm", "Main"], cwd=ROOT, env=env,
                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
            print(result.stdout, end="")
            login_file = temp / "login.ini"
            config = (temp / "OpenEgg.ini").read_text()
            valid_saved_file = login_file.exists() and f"password = {PASSWORD}" in login_file.read_text()
            private_file = os.name == "nt" or login_file.stat().st_mode & 0o077 == 0
            markers = ("network: replacement account login succeeded",
                       "network: identity hello succeeded",
                       "OpenEgg account identity PASS",
                       "OpenEgg account stays separate from PSN PASS")
            if (result.returncode or not received_login.is_set() or not valid_login or
                    not valid_saved_file or not private_file or "account_password" in config or
                    any(marker not in result.stdout for marker in markers) or " FAIL" in result.stdout):
                raise SystemExit("OpenEgg account login probe failed")
            print("saved login, client marker, identity and file permissions PASS")
    finally:
        server.shutdown()
        server.server_close()


if __name__ == "__main__":
    main()
