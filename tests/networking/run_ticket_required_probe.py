#!/usr/bin/env python3
"""Check that an Online ID without a ticket cannot authenticate."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading

ROOT = Path(__file__).resolve().parents[2]
requests = []


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        self.rfile.read(int(self.headers.get("Content-Length", 0)))
        requests.append(("POST", self.path))
        self.send_response(404)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def do_GET(self):
        requests.append(("GET", self.path))
        self.send_response(404)
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
        with tempfile.TemporaryDirectory(prefix="openegg-ticket-required-") as name:
            temp = Path(name)
            engine = ROOT / "build/OpenEgg"
            shutil.copy2(engine, temp / engine.name)
            for archive in engine.parent.glob("*.psarc"):
                (temp / archive.name).symlink_to(archive)
            data = engine.parent / "game_data"
            if data.is_dir():
                (temp / "game_data").symlink_to(data, target_is_directory=True)
            url = f"http://127.0.0.1:{server.server_port}"
            (temp / "OpenEgg.ini").write_text(
                f"[network]\nserver_url = {url}\npsn_online_id = openegg_probe\n")
            harness = temp / "harness.txt"
            harness.write_text("wait 30\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt",
                       GE3_HARNESS=str(harness), SDL_AUDIODRIVER="dummy",
                       LIBGL_ALWAYS_SOFTWARE="1", PATH="/usr/bin:/bin:" + os.environ.get("PATH", ""))
            result = subprocess.run([str(temp / engine.name), str(ROOT / "tests/networking"),
                                     "TicketRequiredProbe.gm", "Main"], cwd=ROOT, env=env,
                                    text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    timeout=30)
            print(result.stdout, end="")
            if (result.returncode or "ticket required PASS" not in result.stdout or
                    "[gm]" in result.stdout or requests):
                raise SystemExit(f"ticket requirement probe failed; requests={requests}")
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


if __name__ == "__main__":
    main()
