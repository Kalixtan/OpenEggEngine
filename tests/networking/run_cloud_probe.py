#!/usr/bin/env python3
"""Probe cloud save requests with a local stub or disposable local server account."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading

ROOT = Path(__file__).resolve().parents[2]
USER = "cloud-probe"
SESSION = "cloud-session"
SAVE = b'{"onlineID":"probe","progress":7,"note":"----BouNdaRy","padding":"' + b"x" * 3000 + b'"}'
BOUNDARY = b"--fOsBKwDjKn"
received = threading.Event()
posted = threading.Event()


def multipart(save=SAVE, boundary=BOUNDARY):
    body = b""
    for name, value in ((b"mutable", b"true"), (b"deleteLevel", b"HARD"), (b"file", save)):
        body += b"--" + boundary + b'\r\nContent-Disposition: form-data; name="' + name + b'"\r\n\r\n' + value + b"\r\n"
    return body + b"--" + boundary + b"--\r\n"


class Handler(BaseHTTPRequestHandler):
    def reply(self, status, body=b""):
        self.send_response(status)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if (self.path == f"/otg/~identity:{USER}/~content:progress/data.get" and
                self.headers.get("X-OTG-Identity-SessionId") == SESSION):
            received.set()
            self.reply(200, SAVE) if posted.is_set() else self.reply(404)
        else:
            self.reply(404)

    def do_POST(self):
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        delete = b'{"onlineID":"probe"}'
        if (self.path == f"/otg/~identity:{USER}/~content:progress.put" and
                self.headers.get("X-OTG-Identity-SessionId") == SESSION and
                self.headers.get("Content-Type") == "multipart/form-data; boundary=----BouNdaRy" and
                body == multipart(delete, b"----BouNdaRy")):
            posted.clear()
            self.reply(200, b"ok")
            return
        if (self.path == f"/otg/~identity:{USER}/~content:progress.put" and
                self.headers.get("X-OTG-Identity-SessionId") == SESSION and
                self.headers.get("Content-Type") == "multipart/form-data; boundary=" + BOUNDARY.decode() and
                body == multipart() and len(body) == 231 + len(SAVE)):
            posted.set()
            self.reply(200, b"ok")
        else:
            self.reply(400)

    def log_message(self, *args):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server-url", help="Disposable local server; uses a throwaway account")
    parser.add_argument("--user-id", help="Identity for the throwaway account")
    parser.add_argument("--session-id", help="Session for the throwaway account")
    parser.add_argument("--engine", type=Path, default=ROOT / "build" / "OpenEgg")
    args = parser.parse_args()
    server = None
    if args.server_url:
        if not args.server_url.startswith("http://127.0.0.1:") or not args.user_id or not args.session_id:
            parser.error("local server URL, disposable user ID and session are required")
        url, user, session = args.server_url.rstrip("/"), args.user_id, args.session_id
    else:
        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        server.daemon_threads = True
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        url, user, session = f"http://127.0.0.1:{server.server_port}", USER, SESSION
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-cloud-") as name:
            temp = Path(name)
            engine = args.engine.resolve()
            shutil.copy2(engine, temp / engine.name)
            for asset in engine.parent.glob("*.psarc"):
                (temp / asset.name).symlink_to(asset)
            data = engine.parent / "game_data"
            if data.is_dir():
                (temp / "game_data").symlink_to(data, target_is_directory=True)
            (temp / "OpenEgg.ini").write_text(f"[network]\nserver_url = {url}\nuser_id = {user}\nsession_id = {session}\n")
            harness = temp / "harness.txt"
            harness.write_text("wait 240\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                       SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
            result = subprocess.run([str(temp / engine.name), str(ROOT / "tests/networking"), "CloudSaveProbe.gm", "Main"],
                                    cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
            print(result.stdout, end="")
            markers = ("cloud nested merge PASS", "cloud boundary exhaustion PASS", "cloud save missing PASS", "cloud save queued PASS",
                       "cloud save post PASS", "cloud save readback PASS", "cloud save delete PASS", "cloud save deleted PASS")
            if result.returncode or any(marker not in result.stdout for marker in markers) or (server and not received.is_set()):
                raise SystemExit("cloud save probe failed")
    finally:
        if server:
            server.shutdown()
            server.server_close()
            thread.join()


if __name__ == "__main__":
    main()
