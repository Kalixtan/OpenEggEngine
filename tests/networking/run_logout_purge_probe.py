#!/usr/bin/env python3
"""Check identity requests are cancelled across logout and reconnect."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading

from run_image_probe import png
from run_ticket_probe import ticket

ROOT = Path(__file__).resolve().parents[2]
SESSION = "00000000-0000-0000-0000-000000000001"
USER = "00000000-0000-0000-0000-000000000002"
slow_received = threading.Event()
release_slow = threading.Event()
seen = []

class Handler(BaseHTTPRequestHandler):
    def reply(self, status, body=b""):
        self.send_response(status)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        self.rfile.read(int(self.headers.get("Content-Length", 0)))
        self.reply(201, ('{"session":{"id":"' + SESSION + '","person":{"id":"' + USER + '"}}}').encode())

    def do_GET(self):
        if self.path.startswith("/otg/~index:level.page?"):
            seen.append(self.path)
            if "slow=1" in self.path:
                slow_received.set()
                if not release_slow.wait(15):
                    self.reply(503)
                    return
            self.reply(200, b'{"items":[]}')
        elif self.path == "/image.png":
            seen.append("image-with-session" if self.headers.get("X-OTG-Identity-SessionId") else "image")
            self.reply(200, png())
        elif self.path == "/otg/~identity:*.hello":
            self.reply(200)
        else:
            self.reply(404)

    def log_message(self, *args):
        pass

server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
server.daemon_threads = True
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
try:
    with tempfile.TemporaryDirectory(prefix="openegg-logout-") as name:
        temp = Path(name)
        engine = ROOT / "build/OpenEgg"
        shutil.copy2(engine, temp / engine.name)
        for asset in engine.parent.glob("*.psarc"):
            (temp / asset.name).symlink_to(asset)
        data = engine.parent / "game_data"
        if data.is_dir():
            (temp / "game_data").symlink_to(data, target_is_directory=True)
        url = f"http://127.0.0.1:{server.server_port}"
        (temp / "ticket.bin").write_bytes(ticket())
        (temp / "OpenEgg.ini").write_text(f"[network]\nserver_url = {url}\nticket_file = ticket.bin\n")
        script = temp / "probe"
        script.mkdir()
        source = (ROOT / "tests/networking/LogoutPurgeProbe.gm").read_text().replace("__IMAGE_URL__", url + "/image.png")
        (script / "LogoutPurgeProbe.gm").write_text(source)
        harness = temp / "harness.txt"
        harness.write_text("wait 300\nquit\n")
        env = os.environ.copy()
        env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                   SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
        process = subprocess.Popen([str(temp / engine.name), str(script), "LogoutPurgeProbe.gm", "Main"],
                                   env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        lines, received_before_logout = [], []
        def read_output():
            for line in process.stdout:
                lines.append(line)
                if "logout transition requested" in line:
                    received_before_logout.append(slow_received.is_set())
                    release_slow.set()
        reader = threading.Thread(target=read_output)
        reader.start()
        try:
            process.wait(timeout=30)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
            raise
        finally:
            release_slow.set()
            reader.join()
        output = "".join(lines)
        print(output, end="")
        if (process.returncode or received_before_logout != [True] or
                any(marker not in output for marker in ("logout identity requests PASS", "logout global image PASS")) or
                any("queued=1" in item for item in seen) or "image" not in seen or "image-with-session" in seen):
            raise SystemExit("logout purge probe failed")
finally:
    release_slow.set()
    server.shutdown()
    server.server_close()
    thread.join()
