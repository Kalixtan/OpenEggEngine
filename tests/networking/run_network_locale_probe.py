#!/usr/bin/env python3
"""Check region language selection through the full local online flow."""
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
seen = set()


def reply(handler, status, body):
    handler.send_response(status)
    handler.send_header("Content-Length", str(len(body)))
    handler.end_headers()
    handler.wfile.write(body)


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        self.rfile.read(int(self.headers.get("Content-Length", 0)))
        if self.path == "/identity/login/token/psn":
            body = ('{"session":{"id":"' + SESSION + '","person":{"id":"' + USER + '","display_name":"openegg_probe"}}}').encode()
            reply(self, 201, body)
        elif self.path == f"/identity/person/{USER}/data/psn/friends-list":
            seen.add(self.path)
            reply(self, 200, b"")
        else:
            reply(self, 404, b"")

    def do_GET(self):
        if self.path == "/otg/~identity:*.hello":
            reply(self, 200, b"")
        elif self.path.startswith("/otg/ps3/") and self.path.endswith("/~eula.get"):
            if self.headers.get("X-OTG-Identity-SessionId") != SESSION:
                reply(self, 403, b"")
                return
            seen.add(self.path)
            reply(self, 200, b"Fixture agreement")
        elif self.path == f"/otg/~identity:{USER}/~friends.all":
            seen.add(self.path)
            reply(self, 200, b'{"friends":[]}')
        elif self.path in ("/otg/global/featured/~metadata:*.get", "/otg/global/news/~metadata:*.get"):
            seen.add(self.path)
            reply(self, 200, b"{}")
        elif self.path.startswith("/otg/global/featured/") or self.path.startswith("/otg/global/news/"):
            seen.add(self.path)
            reply(self, 200, b"{}")
        elif self.path.startswith("/otg/ps3/") and self.path.endswith("/~translation.get"):
            seen.add(self.path)
            reply(self, 200, b"")
        else:
            reply(self, 404, b"")

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
        harness.write_text("wait 120\nquit\n")
        script_dir = temp / "probe"
        script_dir.mkdir()
        env = os.environ.copy()
        env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                   SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
        cases = (("SCEA", 1, "en"), ("SCEA", 2, "fr"), ("SCEA", 3, "es_mx"),
                 ("SCEE", 4, "de"), ("SCEE", 3, "es"), ("SCEE", 16, "pl"), ("SCEE", 19, "en_gb"),
                 ("SCEJ", 0, "ja"), ("SCEJ", 2, "en"), ("SCEH", 9, "ko"),
                 ("SCEH", 10, "zh_hant"), ("SCEH", 2, "en"))
        for region, language, expected in cases:
            (temp / "OpenEgg.ini").write_text(
                f"[system]\nlanguage = {language}\n[network]\n"
                f"server_url = http://127.0.0.1:{server.server_port}\n"
                f"psn_online_id = openegg_probe\n"
                f"ticket_file = ticket.bin\nproduct_region = {region}\n")
            source = (ROOT / "tests/networking/NetworkLocaleProbe.gm").read_text().replace("__EXPECTED__", expected)
            (script_dir / "NetworkLocaleProbe.gm").write_text(source)
            seen.clear()
            result = subprocess.run([str(temp / engine.name), str(script_dir), "NetworkLocaleProbe.gm", "Main"],
                                    env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=45)
            print(result.stdout, end="")
            expected_paths = {
                f"/otg/ps3/{region}/{expected}/~eula.get",
                f"/identity/person/{USER}/data/psn/friends-list",
                f"/otg/~identity:{USER}/~friends.all",
                "/otg/global/featured/~metadata:*.get", "/otg/global/news/~metadata:*.get",
                f"/otg/global/featured/{expected}/~metadata:*.get",
                f"/otg/global/news/{expected}/~metadata:*.get",
                f"/otg/ps3/{region}/{expected}/~translation.get",
            }
            if (result.returncode or "network locale PASS" not in result.stdout or
                    "localized EULA PASS" not in result.stdout or
                    "localized community requests PASS" not in result.stdout or
                    " FAIL" in result.stdout or "[gm]" in result.stdout or not expected_paths.issubset(seen)):
                missing = sorted(expected_paths - seen)
                raise SystemExit(f"network locale probe failed for {region}/{language}; missing {missing}")
            print(f"localized paths PASS: {region} language {language} -> {expected}")
finally:
    server.shutdown()
    server.server_close()
    thread.join()
