#!/usr/bin/env python3
"""Check HTTP status and cache handling when a response body is interrupted."""
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading
import zlib

from run_level_data_probe import Handler, ROOT
from run_ticket_probe import ticket

level_requests = []
error_requests = []
stall_started = threading.Event()
release_stall = threading.Event()
stall_expired = threading.Event()


class FailureHandler(Handler):
    def truncated(self, body, status=200):
        self.send_response(status)
        self.send_header("Content-Length", str(len(body) + 100))
        self.end_headers()
        self.wfile.write(body)
        self.wfile.flush()
        self.close_connection = True

    def do_GET(self):
        if self.path == "/otg/~index:level.page?type=level&case=partial":
            self.truncated(b'{"count":')
        elif self.path == "/otg/~index:level.page?type=level&case=disconnect":
            self.close_connection = True
        elif self.path == "/otg/~index:level.page?type=level&case=success":
            self.reply(200, b'{"count":0}')
        elif self.path == "/otg/~index:level.page?type=level&case=stall":
            self.send_response(404)
            self.send_header("Content-Length", "100")
            self.end_headers()
            self.wfile.flush()
            stall_started.set()
            if not release_stall.wait(10):
                stall_expired.set()
            self.close_connection = True
        elif self.path == "/otg/~index:level.page?type=level&case=after-stall":
            self.reply(200, b'{"count":0}')
        elif self.path == "/otg/~level:transport/~version:v1/~content:level/data.get":
            level_requests.append(self.path)
            data = zlib.compress(b'{"bpm":130,"screenData":[],"entities":[]}')
            if len(level_requests) == 1:
                self.truncated(data[:2])
            else:
                self.reply(200, data)
        elif self.path == "/otg/~level:transport-error/~version:v1/~content:level/data.get":
            error_requests.append(self.path)
            if len(error_requests) == 1:
                self.truncated(b"missing", 404)
            else:
                self.reply(200, zlib.compress(b'{"bpm":130,"screenData":[],"entities":[]}'))
        else:
            super().do_GET()


def main():
    server = ThreadingHTTPServer(("127.0.0.1", 0), FailureHandler)
    server.daemon_threads = True
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-transport-") as name:
            temp = Path(name)
            engine = ROOT / "build/OpenEgg"
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
            process = subprocess.Popen([str(temp / engine.name), str(ROOT / "tests/networking"),
                                        "TransportFailureProbe.gm", "Main"], env=env, text=True,
                                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            lines, stalled = [], []
            def read_output():
                for line in process.stdout:
                    lines.append(line)
                    if "transport stalled error PASS" in line:
                        stalled.append(stall_started.is_set() and not stall_expired.is_set())
                        release_stall.set()
            reader = threading.Thread(target=read_output)
            reader.start()
            try:
                process.wait(timeout=30)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
                raise
            finally:
                release_stall.set()
                reader.join()
            output = "".join(lines)
            print(output, end="")
            markers = ("transport partial status PASS", "transport no status PASS", "transport success PASS",
                       "transport failure not cached PASS", "transport HTTP error cached PASS",
                       "transport stalled error PASS", "transport after stall PASS")
            if process.returncode or stalled != [True] or len(level_requests) != 2 or len(error_requests) != 1 or any(marker not in output for marker in markers) or " FAIL" in output:
                raise SystemExit("transport failure probe failed")
    finally:
        release_stall.set()
        server.shutdown()
        server.server_close()
        thread.join()


if __name__ == "__main__":
    main()
