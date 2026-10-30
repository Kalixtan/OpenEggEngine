#!/usr/bin/env python3
"""Exercise OpenEgg ticket login against a real local SoundShapesServer."""

import json
import os
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile
import time

from run_ticket_probe import ticket


ROOT = Path(__file__).resolve().parents[2]


def main():
    engine = ROOT / "build" / "OpenEgg"
    if not engine.is_file():
        raise SystemExit(f"OpenEgg executable not found: {engine}")

    project = ROOT / "networking" / "SoundShapesServer" / "SoundShapesServer" / "SoundShapesServer.csproj"
    with tempfile.TemporaryDirectory(prefix="openegg-live-server-") as temporary:
        temp = Path(temporary)
        publish_dir = temp / "server"
        publish = subprocess.run(
            ["dotnet", "publish", str(project), "-c", "Release", "-o", str(publish_dir),
             "--artifacts-path", str(temp / "artifacts")],
            cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            timeout=180, check=False,
        )
        if publish.returncode:
            print(publish.stdout)
            raise SystemExit("SoundShapesServer publish failed")

        with socket.socket() as port_socket:
            port_socket.bind(("127.0.0.1", 0))
            port = port_socket.getsockname()[1]

        state_dir = temp / "server-state"
        state_dir.mkdir()
        (state_dir / "bunkum.json").write_text(json.dumps({
            "Version": 4,
            "ExternalUrl": f"http://127.0.0.1:{port}",
            "ListenHost": "127.0.0.1",
            "ListenPort": port,
            "ThreadCount": 2,
            "UseForwardedIp": False,
        }))
        (state_dir / "gameServer.json").write_text(json.dumps({
            "Version": 8,
            "RequireAuthentication": False,
            "AccountCreation": True,
            "EulaText": "Welcome back to Sound Shapes!",
            "SourceCodeUrl": "",
            "LevelPublishLimit": 200,
        }))

        server = subprocess.Popen(
            ["dotnet", str(publish_dir / "SoundShapesServer.dll")],
            cwd=state_dir, stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        )
        try:
            ready = False
            for _ in range(200):
                if server.poll() is not None:
                    break
                try:
                    with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                        ready = True
                        break
                except OSError:
                    time.sleep(0.1)
            if not ready:
                log = server.stdout.read() if server.poll() is not None else "startup timeout"
                raise SystemExit(f"SoundShapesServer did not start:\n{log}")

            client_dir = temp / "client"
            client_dir.mkdir()
            shutil.copy2(engine, client_dir / engine.name)
            (client_dir / "ticket.bin").write_bytes(ticket())
            ini = client_dir / "OpenEgg.ini"
            ini.write_text(
                f"[network]\nserver_url = http://127.0.0.1:{port}\n"
                "psn_online_id = openegg_probe\nticket_file = ticket.bin\n"
            )
            harness = temp / "harness.txt"
            harness.write_text("wait 240\nquit\n")
            env = os.environ.copy()
            env.update({
                "GE3_HEADLESS": "1",
                "GE3_TRACE": "1",
                "GE3_STRICT": "halt",
                "GE3_HARNESS": str(harness),
                "SDL_AUDIODRIVER": "dummy",
                "LIBGL_ALWAYS_SOFTWARE": "1",
            })
            def run_probe(name):
                return subprocess.run(
                    [str(client_dir / engine.name), str(ROOT / "tests" / "networking"), name, "Main"],
                    cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT, timeout=45, check=False,
                )

            login = run_probe("TicketLoginProbe.gm")
            print(login.stdout, end="")
            login_markers = (
                "network: ticket login succeeded",
                "network: identity hello succeeded",
                "ticket authenticated request PASS",
            )
            missing = [marker for marker in login_markers if marker not in login.stdout]
            if login.returncode or missing:
                raise SystemExit(f"client/server login failed; missing markers: {missing}")
            if "network: replacement-server session saved to OpenEgg.ini" in login.stdout:
                raise SystemExit("unsigned dev ticket incorrectly received a remembered session")
            if "server_session_id = " in ini.read_text():
                raise SystemExit("unsigned dev ticket unexpectedly persisted a server session")

            services = run_probe("ServerEulaProbe.gm")
            print(services.stdout, end="")
            service_markers = (
                "server EULA PASS",
                "server friends PASS",
                "network: initial community data requests complete",
                "server coordinator online PASS",
                "replacement auth distinct from PSN PASS",
            )
            missing = [marker for marker in service_markers if marker not in services.stdout]
            if services.returncode or missing:
                raise SystemExit(f"client/server services failed; missing markers: {missing}")

            print("live local ticket login and community session PASS (unsigned ticket, short session)")
        finally:
            server.terminate()
            try:
                server.wait(timeout=10)
            except subprocess.TimeoutExpired:
                server.kill()
                server.wait()


if __name__ == "__main__":
    main()
