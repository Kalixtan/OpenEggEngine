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


class FailureHandler(Handler):
    def truncated(self, body):
        self.send_response(200)
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
        elif self.path == "/otg/~level:transport/~version:v1/~content:level/data.get":
            level_requests.append(self.path)
            data = zlib.compress(b'{"bpm":130,"screenData":[],"entities":[]}')
            if len(level_requests) == 1:
                self.truncated(data[:2])
            else:
                self.reply(200, data)
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
            result = subprocess.run([str(temp / engine.name), str(ROOT / "tests/networking"),
                                     "TransportFailureProbe.gm", "Main"], env=env, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
            print(result.stdout, end="")
            markers = ("transport partial status PASS", "transport no status PASS", "transport success PASS",
                       "transport failure not cached PASS")
            if result.returncode or len(level_requests) != 2 or any(marker not in result.stdout for marker in markers) or " FAIL" in result.stdout:
                raise SystemExit("transport failure probe failed")
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


if __name__ == "__main__":
    main()
