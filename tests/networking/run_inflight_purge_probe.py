#!/usr/bin/env python3
"""Check that purging a request already received by a server cancels its callback."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading
import time

ROOT = Path(__file__).resolve().parents[2]
received = threading.Event()
response_status = 200


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if not self.path.startswith("/otg/~index:"):
            body = b'{}'
            self.send_response(200)
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        received.set()
        time.sleep(2)
        body = b'{"items":[],"count":0}'
        self.send_response(response_status)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args):
        pass


def main():
    global response_status
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, default=ROOT / "build" / "OpenEgg")
    parser.add_argument("--status", type=int, choices=(200, 404), default=200)
    parser.add_argument("--bucket", choices=("int", "float", "string"), default="int")
    args = parser.parse_args()
    response_status = args.status
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    server.daemon_threads = True
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-purge-") as name:
            temp = Path(name)
            engine = args.engine.resolve()
            shutil.copy2(engine, temp / engine.name)
            for asset in engine.parent.glob("*.psarc"):
                (temp / asset.name).symlink_to(asset)
            data = engine.parent / "game_data"
            if data.is_dir():
                (temp / "game_data").symlink_to(data, target_is_directory=True)
            (temp / "OpenEgg.ini").write_text(f"[network]\nserver_url = http://127.0.0.1:{server.server_port}\nuser_id = probe\nsession_id = probe-session\n")
            script_dir = temp / "probe"
            script_dir.mkdir()
            source = (ROOT / "tests/networking/InFlightPurgeProbe.gm").read_text()
            bucket = {"int": "42", "float": "42.5", "string": '"probe-bucket"'}[args.bucket]
            (script_dir / "InFlightPurgeProbe.gm").write_text(
                source.replace("__EXPECTED_ERROR__", str(args.status)).replace("__BUCKET__", bucket))
            harness = temp / "harness.txt"
            harness.write_text("wait 240\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt",
                       GE3_HARNESS=str(harness), SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
            process = subprocess.Popen([str(temp / engine.name), str(script_dir),
                                        "InFlightPurgeProbe.gm", "Main"], cwd=ROOT, env=env,
                                       text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            lines = []
            received_before_purge = []
            def read_output():
                for line in process.stdout:
                    lines.append(line)
                    if "in-flight purge requested" in line:
                        received_before_purge.append(received.is_set())
            reader = threading.Thread(target=read_output)
            reader.start()
            try:
                process.wait(timeout=30)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
                raise
            finally:
                reader.join()
            output = "".join(lines)
            print(output, end="")
            if (process.returncode or received_before_purge != [True] or
                    "in-flight purge PASS" not in output):
                raise SystemExit("in-flight purge probe failed")
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


if __name__ == "__main__":
    main()
