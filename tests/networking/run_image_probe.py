#!/usr/bin/env python3
"""Check network texture callbacks with synthetic images and a local HTTP stub."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import threading
import zlib

from run_ticket_probe import ticket

ROOT = Path(__file__).resolve().parents[2]
SESSION = "00000000-0000-0000-0000-000000000001"
USER = "00000000-0000-0000-0000-000000000002"
seen = set()
content_requests = 0


def chunk(name, data):
    return struct.pack(">I", len(data)) + name + data + struct.pack(">I", zlib.crc32(name + data))


def png():
    header = struct.pack(">IIBBBBB", 2, 3, 8, 6, 0, 0, 0)
    pixels = (b"\x00" + b"\xff\x00\x00\xff" * 2) * 3
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(pixels)) + chunk(b"IEND", b"")


class Handler(BaseHTTPRequestHandler):
    def reply(self, status, body=b""):
        self.send_response(status)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        self.rfile.read(int(self.headers.get("Content-Length", 0)))
        data = ('{"session":{"id":"' + SESSION + '","person":{"id":"' + USER + '"}}}').encode()
        self.reply(201, data)

    def do_GET(self):
        global content_requests
        paths = {
            "/otg/~level:level-1/~version:version-1/~content:image/data.get": "thumbnail",
            "/otg/~communityTab:1/~content:thumbnail/data.get?foo=bar": "content",
            "/absolute-image": "absolute", "/otg/invalid.get": "invalid", "/otg/missing.get": "missing"
        }
        name = paths.get(self.path)
        if name:
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                seen.add(name)
            if name == "content":
                content_requests += 1
            self.reply(404 if name == "missing" else 200,
                       b"invalid" if name == "invalid" or (name == "content" and content_requests > 1) else png())
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
    args = parser.parse_args()
    server = None
    if args.server_url:
        if not args.server_url.startswith("http://127.0.0.1:") or not args.level_id or not args.level_id.isalnum():
            parser.error("local server URL and fixture level ID required")
        url = args.server_url.rstrip("/")
    else:
        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        server.daemon_threads = True
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        url = f"http://127.0.0.1:{server.server_port}"
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-image-") as name:
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
            probe = "ServerImageProbe.gm" if args.server_url else "ImageProbe.gm"
            source = (ROOT / "tests/networking" / probe).read_text().replace("__ABSOLUTE_URL__", url + "/absolute-image")
            if args.server_url:
                source = source.replace("__LEVEL_ID__", args.level_id)
            (script_dir / probe).write_text(source)
            harness = temp / "harness.txt"
            harness.write_text("wait 240\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                       SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
            result = subprocess.run([str(temp / engine.name), str(script_dir), probe, "Main"],
                                    cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
            print(result.stdout, end="")
            names = {"thumbnail", "content", "absolute", "invalid", "missing"}
            markers = (["server image PASS"] if args.server_url else
                       ["image requests queued PASS", "image cached-invalid PASS"] + [f"image {name} PASS" for name in names])
            if (result.returncode or (server and (seen != names or content_requests != 2)) or
                    any(marker not in result.stdout for marker in markers)):
                raise SystemExit("image probe failed")
    finally:
        if server:
            server.shutdown()
            server.server_close()
            thread.join()


if __name__ == "__main__":
    main()
