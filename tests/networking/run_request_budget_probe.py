#!/usr/bin/env python3
"""Check the completion budget with a backlog of local requests."""
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading

from run_level_data_probe import Handler
from run_image_probe import png
from run_ticket_probe import ticket

ROOT = Path(__file__).resolve().parents[2]
image_backlog = threading.Event()
image_count = 0
class BudgetHandler(Handler):
    def do_GET(self):
        global image_count
        if self.path == "/budget-image":
            image_count += 1
            self.reply(200, png())
            if image_count >= 8:
                image_backlog.set()
        elif self.path == "/otg/~level:level-1/~upload:good.get" and not image_backlog.wait(5):
            self.reply(503)
        else:
            super().do_GET()


server = ThreadingHTTPServer(("127.0.0.1", 0), BudgetHandler)
server.daemon_threads = True
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
try:
    with tempfile.TemporaryDirectory(prefix="openegg-budget-") as name:
        temp = Path(name)
        engine = ROOT / "build/OpenEgg"
        shutil.copy2(engine, temp / engine.name)
        for asset in engine.parent.glob("*.psarc"):
            (temp / asset.name).symlink_to(asset)
        data = engine.parent / "game_data"
        if data.is_dir():
            (temp / "game_data").symlink_to(data, target_is_directory=True)
        (temp / "ticket.bin").write_bytes(ticket())
        (temp / "OpenEgg.ini").write_text(
            f"[network]\nserver_url = http://127.0.0.1:{server.server_port}\nticket_file = ticket.bin\n")
        harness = temp / "harness.txt"
        harness.write_text("wait 240\nquit\n")
        script_dir = temp / "probe"
        script_dir.mkdir()
        source = (ROOT / "tests/networking/RequestBudgetProbe.gm").read_text()
        (script_dir / "RequestBudgetProbe.gm").write_text(
            source.replace("__IMAGE_URL__", f"http://127.0.0.1:{server.server_port}/budget-image"))
        env = os.environ.copy()
        env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                   SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
        result = subprocess.run([str(temp / engine.name), str(script_dir),
                                 "RequestBudgetProbe.gm", "Main"], env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        print(result.stdout, end="")
        if result.returncode or "request budget PASS" not in result.stdout or "FAIL" in result.stdout:
            raise SystemExit("request budget probe failed")
finally:
    server.shutdown()
    server.server_close()
    thread.join()
