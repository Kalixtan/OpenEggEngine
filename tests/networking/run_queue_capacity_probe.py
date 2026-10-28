#!/usr/bin/env python3
"""Exercise bounded request admission with a stalled local HTTP request."""
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
entered, release, resumed = threading.Event(), threading.Event(), threading.Event()
global_received = threading.Event()
seen = []

class Handler(BaseHTTPRequestHandler):
    def reply(self, status, body=b""):
        self.send_response(status)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        self.rfile.read(int(self.headers.get("Content-Length", 0)))
        self.reply(200)

    def do_GET(self):
        seen.append(self.path)
        if self.path == "/image.png":
            global_received.set()
        if "slow=1" in self.path:
            entered.set()
            if not release.wait(15):
                self.reply(503)
                return
        if self.path.endswith("/~friends.all"):
            body = b'{"friends":[]}'
        elif self.path.endswith("/~translation.get"):
            body = b""
        else:
            body = b'{"items":[]}'
        self.reply(200, body)

    def log_message(self, *args):
        pass

server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
server.daemon_threads = True
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
try:
    with tempfile.TemporaryDirectory(prefix="openegg-capacity-") as name:
        root = Path(name)
        engine = ROOT / "build/OpenEgg"
        shutil.copy2(engine, root / "OpenEgg")
        for asset in engine.parent.glob("*.psarc"):
            (root / asset.name).symlink_to(asset)
        data = engine.parent / "game_data"
        if data.is_dir():
            (root / "game_data").symlink_to(data, target_is_directory=True)
        url = f"http://127.0.0.1:{server.server_port}"
        (root / "OpenEgg.ini").write_text(f"[network]\nserver_url = {url}\nsession_id = {SESSION}\nuser_id = {USER}\n")
        script = root / "probe"
        script.mkdir()
        source = (ROOT / "tests/networking/QueueCapacityProbe.gm").read_text().replace("__IMAGE_URL__", url + "/image.png")
        (script / "QueueCapacityProbe.gm").write_text(source)
        harness = root / "harness.txt"
        harness.write_text("wait 360\nquit\n")
        env = os.environ.copy()
        env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                   SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
        process = subprocess.Popen([str(root / "OpenEgg"), str(script), "QueueCapacityProbe.gm", "Main"],
                                   env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        lines, blocked = [], []
        def read_output():
            for line in process.stdout:
                lines.append(line)
                if "send queue resumed PASS" in line:
                    resumed.set()
                if "global queue admission PASS" in line:
                    blocked.append(entered.is_set() and global_received.wait(3) and not resumed.wait(0.2))
                    release.set()
        reader = threading.Thread(target=read_output)
        reader.start()
        try:
            process.wait(timeout=30)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
            raise
        finally:
            release.set()
            reader.join()
        output = "".join(lines)
        print(output, end="")
        markers = ("send queue full PASS", "global queue admission PASS", "send queue resumed PASS", "send queue drained PASS")
        if (process.returncode or blocked != [True] or any(marker not in output for marker in markers) or
                sum("queued=1" in path for path in seen) != 32 or not any("extra=1" in path for path in seen)):
            raise SystemExit("queue capacity probe failed")
finally:
    release.set()
    server.shutdown()
    server.server_close()
    thread.join()
