#!/usr/bin/env python3
"""Check that offline transition cancels active and queued identity requests."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading
import time

ROOT = Path(__file__).resolve().parents[2]
index_paths = []


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if "/~index:" in self.path:
            index_paths.append(self.path)
            time.sleep(2)
        if self.path.endswith("/~friends.all"):
            body = b'{"friends":[]}'
        elif self.path.endswith("/~translation.get"):
            body = b""
        else:
            body = b'{"items":[]}'
        self.send_response(200)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        self.rfile.read(int(self.headers.get("Content-Length", 0)))
        self.send_response(200)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def log_message(self, *_args):
        pass


def main():
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    server.daemon_threads = True
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-offline-queue-") as name:
            temp = Path(name)
            engine = ROOT / "build/OpenEgg"
            shutil.copy2(engine, temp / engine.name)
            for asset in engine.parent.glob("*.psarc"):
                (temp / asset.name).symlink_to(asset)
            data = engine.parent / "game_data"
            if data.is_dir():
                (temp / "game_data").symlink_to(data, target_is_directory=True)
            (temp / "OpenEgg.ini").write_text(
                f"[network]\nserver_url = http://127.0.0.1:{server.server_port}\n"
                "user_id = probe\nsession_id = probe-session\n")
            harness = temp / "harness.txt"
            harness.write_text("wait 360\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt",
                       GE3_HARNESS=str(harness), SDL_AUDIODRIVER="dummy",
                       LIBGL_ALWAYS_SOFTWARE="1", PATH="/usr/bin:/bin")
            result = subprocess.run(
                [str(temp / engine.name), str(ROOT / "tests/networking"),
                 "OfflineQueueProbe.gm", "Main"],
                cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, timeout=30)
            print(result.stdout, end="")
            expected = (
                "offline queue transition requested",
                "offline active request cancelled PASS",
                "offline queued request cancelled PASS",
            )
            if result.returncode or any(marker not in result.stdout for marker in expected):
                raise SystemExit("offline queue probe failed")
            if len(index_paths) != 1:
                raise SystemExit(f"expected one server request, got {len(index_paths)}")
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


if __name__ == "__main__":
    main()
