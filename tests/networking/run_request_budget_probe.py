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
from run_ticket_probe import ticket

ROOT = Path(__file__).resolve().parents[2]
server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
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
        env = os.environ.copy()
        env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                   SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
        result = subprocess.run([str(temp / engine.name), str(ROOT / "tests/networking"),
                                 "RequestBudgetProbe.gm", "Main"], env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        print(result.stdout, end="")
        if result.returncode or "request budget PASS" not in result.stdout or "FAIL" in result.stdout:
            raise SystemExit("request budget probe failed")
finally:
    server.shutdown()
    server.server_close()
    thread.join()
