#!/usr/bin/env python3
"""Check staged network assets with a local fixture or disposable server."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading
import zlib

from run_image_probe import png
from run_ticket_probe import ticket

ROOT = Path(__file__).resolve().parents[2]
SESSION = "00000000-0000-0000-0000-000000000001"
USER = "00000000-0000-0000-0000-000000000002"
LEVEL = zlib.compress(b'{"bpm":120,"screenData":[],"entities":[]}')
SOUND = b"Fun synthetic sound"
IMAGE = png()
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
        prefix = "/otg/~level:levelABC/~version:"
        if self.path.startswith(prefix) and "/~content:" in self.path:
            version, content = self.path[len(prefix):].split("/~content:", 1)
            kind = content.removesuffix("/data.get")
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                seen.append((version, kind))
            if version in ("missing", "missing8") and kind == "sound":
                self.reply(404)
            elif kind in ("level", "sound", "image"):
                self.reply(200, {"level": LEVEL, "sound": SOUND, "image": IMAGE}[kind])
            else:
                self.reply(404)
        elif self.path == "/otg/~identity:*.hello":
            self.reply(200)
        else:
            self.reply(404)

    def log_message(self, *args):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server-url", help="Disposable local replacement server")
    parser.add_argument("--level-id", help="Fixture level ID")
    parser.add_argument("--expected-dir", type=Path, help="Directory holding fixture resource bytes")
    args = parser.parse_args()
    server = None
    if args.server_url:
        if (not args.server_url.startswith("http://127.0.0.1:") or not args.level_id or
                not args.level_id.isalnum() or not args.expected_dir):
            parser.error("local server, fixture level ID and expected resource directory required")
        url, level = args.server_url.rstrip("/"), args.level_id
        expected = {".json.z": (args.expected_dir / "probe-level").read_bytes(),
                    ".png": (args.expected_dir / "probe-image").read_bytes(),
                    ".jef": (args.expected_dir / "probe-sound").read_bytes()}
    else:
        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        server.daemon_threads = True
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        url, level = f"http://127.0.0.1:{server.server_port}", "levelABC"
        expected = {".json.z": LEVEL, ".png": IMAGE, ".jef": SOUND}
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-assets-") as name:
            temp = Path(name)
            engine = ROOT / "build/OpenEgg"
            shutil.copy2(engine, temp / engine.name)
            for asset in engine.parent.glob("*.psarc"):
                (temp / asset.name).symlink_to(asset)
            data = engine.parent / "game_data"
            if data.is_dir():
                (temp / "game_data").symlink_to(data, target_is_directory=True)
            (temp / "ticket.bin").write_bytes(ticket())
            (temp / "OpenEgg.ini").write_text(f"[network]\nserver_url = {url}\nticket_file = ticket.bin\n")
            script_dir = temp / "probe"
            script_dir.mkdir()
            source = (ROOT / "tests/networking/AssetBundleProbe.gm").read_text().replace("__LEVEL_ID__", level)
            source = source.replace("__FAILURE_TEST__", "no" if args.server_url else "yes")
            source = source.replace("__EXTRA_ARGS__", ', false, "ignored"' if args.server_url else "")
            (script_dir / "AssetBundleProbe.gm").write_text(source)
            harness = temp / "harness.txt"
            harness.write_text("wait 300\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                       SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
            result = subprocess.run([str(temp / engine.name), str(script_dir), "AssetBundleProbe.gm", "Main"],
                                    cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
            print(result.stdout, end="")
            markers = ["asset bundle queued PASS", "asset staging and save PASS"]
            if server:
                markers.extend(["asset partial failure PASS", "asset extra arguments PASS"])
            actual = {suffix: (temp / "savedata/myLevels" / ("ok" + suffix)).read_bytes()
                      for suffix in expected if (temp / "savedata/myLevels" / ("ok" + suffix)).exists()}
            order = [(v, k) for v in ("ok", "missing", "missing8") for k in ("level", "sound", "image")]
            if (result.returncode or actual != expected or (server and seen != order) or
                    (temp / "savedata/myLevels/bad.json.z").exists() or
                    (temp / "savedata/myLevels/bad8.json.z").exists() or
                    any(marker not in result.stdout for marker in markers)):
                raise SystemExit("asset bundle probe failed")
    finally:
        if server:
            server.shutdown()
            server.server_close()
            thread.join()


if __name__ == "__main__":
    main()
