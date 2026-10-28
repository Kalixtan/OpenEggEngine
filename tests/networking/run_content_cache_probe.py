#!/usr/bin/env python3
"""Check content cache hits and filter invalidation with a local fixture."""
from http.server import ThreadingHTTPServer
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading
import zlib

from run_level_data_probe import Handler, ROOT, SESSION
from run_ticket_probe import ticket

requests = []
missing_requests = []
eviction_requests = []


class CacheHandler(Handler):
    def do_GET(self):
        for level in ("aaa", "bbb", "ccc"):
            if self.path == f"/otg/~level:{level}/~version:v1/~content:level/data.get":
                if self.headers.get("X-OTG-Identity-SessionId") != SESSION:
                    self.reply(403)
                    return
                eviction_requests.append(level)
                self.reply(404 if level == "aaa" else 200, b"" if level == "aaa" else b"X" * (600 * 1024))
                return
        if self.path == "/otg/~level:cache-level/~version:missing/~content:level/data.get":
            if self.headers.get("X-OTG-Identity-SessionId") != SESSION:
                self.reply(403)
                return
            missing_requests.append(self.path)
            if len(missing_requests) == 1:
                self.reply(404)
            else:
                self.reply(200, zlib.compress(b'{"bpm":140,"screenData":[],"entities":[]}'))
            return
        if self.path == "/otg/~level:cache-level/~version:v1/~content:level/data.get":
            if self.headers.get("X-OTG-Identity-SessionId") != SESSION:
                self.reply(403)
                return
            requests.append(self.path)
            self.reply(200, zlib.compress(f'{{"bpm":{119 + len(requests)},"screenData":[],"entities":[]}}'.encode()))
            return
        super().do_GET()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--eviction", action="store_true", help="Check pool eviction edge cases")
    args = parser.parse_args()
    server = ThreadingHTTPServer(("127.0.0.1", 0), CacheHandler)
    server.daemon_threads = True
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-cache-") as name:
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
            probe = "ContentCacheEvictionProbe.gm" if args.eviction else "ContentCacheProbe.gm"
            result = subprocess.run([str(temp / engine.name), str(ROOT / "tests/networking"),
                                     probe, "Main"], env=env, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
            print(result.stdout, end="")
            markers = (("cache empty candidate PASS", "cache eviction PASS") if args.eviction else
                       ("cache hit PASS", "cache filter PASS", "cache purge PASS", "cache error hit PASS", "cache error purge PASS"))
            request_ok = (eviction_requests == ["aaa", "bbb", "ccc", "ccc", "ccc"] if args.eviction else
                          len(requests) == 2 and len(missing_requests) == 2)
            if result.returncode or not request_ok or any(marker not in result.stdout for marker in markers) or "FAIL" in result.stdout:
                raise SystemExit("content cache probe failed")
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


if __name__ == "__main__":
    main()
