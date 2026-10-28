#!/usr/bin/env python3
"""Check the desktop network-language fallback and EULA request."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading

from run_ticket_probe import ticket

ROOT = Path(__file__).resolve().parents[2]
SESSION = "00000000-0000-0000-0000-000000000001"
USER = "00000000-0000-0000-0000-000000000002"
seen = []


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        self.rfile.read(int(self.headers.get("Content-Length", 0)))
        self.reply(201, ('{"session":{"id":"' + SESSION + '","person":{"id":"' + USER + '"}}}').encode())

    def do_GET(self):
        if self.path == "/otg/~identity:*.hello":
            self.reply(200, b"")
        elif self.path.startswith("/otg/ps3/SCEA/") and self.path.endswith("/~eula.get"):
            if self.headers.get("X-OTG-Identity-SessionId") != SESSION:
                self.reply(403, b"")
                return
            seen.append(self.path)
            self.reply(200, b"Fixture agreement")
        else:
            self.reply(404, b"")

    def reply(self, status, body):
        self.send_response(status)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args):
        pass


server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
server.daemon_threads = True
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
try:
    with tempfile.TemporaryDirectory(prefix="openegg-network-locale-") as name:
        temp = Path(name)
        engine = ROOT / "build/OpenEgg"
        shutil.copy2(engine, temp / engine.name)
        for archive in engine.parent.glob("*.psarc"):
            (temp / archive.name).symlink_to(archive)
        (temp / "ticket.bin").write_bytes(ticket())
        harness = temp / "harness.txt"
        harness.write_text("wait 180\nquit\n")
        script_dir = temp / "probe"
        script_dir.mkdir()
        env = os.environ.copy()
        env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                   SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
        for language, expected in ((1, "en"), (2, "fr"), (3, "es_mx"), (4, "en")):
            (temp / "OpenEgg.ini").write_text(
                f"[system]\nlanguage = {language}\n[network]\nserver_url = http://127.0.0.1:{server.server_port}\nticket_file = ticket.bin\n")
            source = (ROOT / "tests/networking/NetworkLocaleProbe.gm").read_text().replace("__EXPECTED__", expected)
            (script_dir / "NetworkLocaleProbe.gm").write_text(source)
            seen.clear()
            result = subprocess.run([str(temp / engine.name), str(script_dir), "NetworkLocaleProbe.gm", "Main"],
                                    env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=45)
            print(result.stdout, end="")
            if (result.returncode or "network locale PASS" not in result.stdout or
                    "localized EULA PASS" not in result.stdout or " FAIL" in result.stdout or "[gm]" in result.stdout or
                    seen != [f"/otg/ps3/SCEA/{expected}/~eula.get"]):
                raise SystemExit(f"network locale probe failed for language {language}")
finally:
    server.shutdown()
    server.server_close()
    thread.join()
