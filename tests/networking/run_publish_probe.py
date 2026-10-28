#!/usr/bin/env python3
"""Publish, update and remove a synthetic level on a local fixture server."""
import argparse
from email import policy
from email.parser import BytesParser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
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
LEVEL = zlib.compress(b'{"bpm":120,"screenData":[],"entities":[],"entityTypesUsed":[]}')
IMAGE = png()
SOUND = b"Fun synthetic --fOsBKwDjKn sound"
seen = []


class Handler(BaseHTTPRequestHandler):
    def reply(self, status, body=b""):
        self.send_response(status)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        if self.path == "/identity/login/token/psn":
            response = ('{"session":{"id":"' + SESSION + '","person":{"id":"' + USER + '"}}}').encode()
            self.reply(201, response)
            return
        stage = "create" if self.path == "/otg/~level:*.create" else "update" if self.path == "/otg/~level:levelABC.update" else None
        if not stage or self.headers.get("X-OTG-Identity-SessionId") != SESSION:
            self.reply(400)
            return
        content_type = self.headers.get("Content-Type", "")
        if content_type != "multipart/form-data; boundary=----BouNdaRy":
            self.reply(400)
            return
        message = BytesParser(policy=policy.default).parsebytes(("Content-Type: " + content_type + "\r\nMIME-Version: 1.0\r\n\r\n").encode() + body)
        parts = list(message.iter_parts())
        names = [part.get_param("name", header="content-disposition") for part in parts]
        expected = (["baseVersionId"] if stage == "update" else []) + ["title", "description", "image", "level", "sound", "sce_np_language"]
        values = {part.get_param("name", header="content-disposition"): part.get_payload(decode=True) for part in parts}
        mimes = {part.get_param("name", header="content-disposition"): part.get_content_type() for part in parts}
        valid = (names == expected and values.get("title") == b"Synthetic probe" and values.get("description") == b"Test description" and
                 values.get("level") == LEVEL and values.get("image") == IMAGE and values.get("sound") == SOUND and
                 values.get("sce_np_language") == b"1" and (stage != "update" or values.get("baseVersionId") == b"42") and
                 all(part.get_filename() == "file.tmp" for part in parts if part.get_param("name", header="content-disposition") in ("image", "level", "sound")) and
                 mimes.get("image") == "image/png" and mimes.get("level") == "application/vnd.soundshapes.level" and
                 mimes.get("sound") == "application/vnd.soundshapes.sound")
        if not valid:
            self.reply(400)
            return
        seen.append(stage)
        self.reply(201, b'{"id":"/~level:levelABC/~upload:42"}')

    def do_GET(self):
        if self.path == "/otg/~identity:*.hello":
            self.reply(200)
        elif self.path == "/otg/~level:levelABC/~upload:42.get":
            seen.append("description")
            self.reply(200, b'{"id":"/~level:levelABC/~upload:42","title":"Synthetic probe"}')
        elif self.path == "/otg/~level:levelABC/~version:42/~content:level/data.get":
            seen.append("readback")
            self.reply(200, LEVEL)
        elif self.path == "/otg/~level:levelABC.latest":
            seen.append("snapshot")
            self.reply(200, b'{"id":"/~level:levelABC/~upload:42"}')
        elif self.path == "/otg/~level:levelABC.delete":
            seen.append("remove")
            self.reply(200)
        else:
            self.reply(404)

    def log_message(self, *args):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server-url", help="Disposable local replacement server")
    args = parser.parse_args()
    server = None
    if args.server_url:
        if not args.server_url.startswith("http://127.0.0.1:"):
            parser.error("server must be a disposable local 127.0.0.1 instance")
        url = args.server_url.rstrip("/")
    else:
        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        server.daemon_threads = True
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        url = f"http://127.0.0.1:{server.server_port}"
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-publish-") as name:
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
            savedata = temp / "savedata"
            savedata.mkdir()
            for filename, content in (("level.bin", LEVEL), ("image.bin", IMAGE), ("sound.bin", SOUND)):
                (savedata / filename).write_bytes(content)
            script_dir = temp / "probe"
            script_dir.mkdir()
            source = (ROOT / "tests/networking/PublishProbe.gm").read_text()
            for key, value in (("__LEVEL_SIZE__", len(LEVEL)), ("__IMAGE_SIZE__", len(IMAGE)), ("__SOUND_SIZE__", len(SOUND))):
                source = source.replace(key, str(value))
            (script_dir / "PublishProbe.gm").write_text(source)
            harness = temp / "harness.txt"
            harness.write_text("wait 300\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                       SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
            result = subprocess.run([str(temp / engine.name), str(script_dir), "PublishProbe.gm", "Main"], cwd=ROOT, env=env,
                                    text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
            print(result.stdout, end="")
            markers = ("publish missing ticket PASS", "publish invalid IDs PASS", "publish create queued PASS", "publish create PASS", "publish description PASS", "publish level readback PASS", "publish snapshot PASS", "publish update queued PASS",
                       "publish update PASS", "publish cleanup PASS")
            if result.returncode or any(marker not in result.stdout for marker in markers) or (server and seen != ["create", "description", "readback", "snapshot", "update", "remove"]):
                raise SystemExit("publish probe failed")
    finally:
        if server:
            server.shutdown()
            server.server_close()
            thread.join()


if __name__ == "__main__":
    main()
