#!/usr/bin/env python3
"""Exercise the retail cloud-sync script with a disposable HTTP fixture."""
from email.parser import BytesParser
from email.policy import default
import argparse
from http.server import ThreadingHTTPServer
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading
from urllib.parse import urlsplit
from urllib.request import Request, urlopen

from run_cloud_probe import BOUNDARY, Handler, ROOT, SESSION, USER, multipart

remote = {"onlineID": "openegg_probe", "version": 1,
          "unlocks": {"unlockedEntities": {"remoteEntity": 1}},
          "campaign": {"asides": {"world": {"remoteLevel": {"completion": 2},
                                                "shared": {"completion": 4}}}, "bsides": {}},
          "editor": {"editorTrophyLevels": {}}}
received = []


class SyncHandler(Handler):
    def do_GET(self):
        if (self.path == f"/otg/~identity:{USER}/~content:progress/data.get" and
                self.headers.get("X-OTG-Identity-SessionId") == SESSION):
            self.reply(200, json.dumps(remote).encode())
        else:
            self.reply(404)

    def do_POST(self):
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        if (self.path != f"/otg/~identity:{USER}/~content:progress.put" or
                self.headers.get("X-OTG-Identity-SessionId") != SESSION):
            self.reply(403)
            return
        message = BytesParser(policy=default).parsebytes(
            b"Content-Type: " + self.headers["Content-Type"].encode() + b"\r\nMIME-Version: 1.0\r\n\r\n" + body)
        fields = {part.get_param("name", header="content-disposition"): part.get_payload(decode=True)
                  for part in message.iter_parts()}
        try:
            save = json.loads(fields["file"])
            world = save["campaign"]["asides"]["world"]
            valid = (fields["mutable"] == b"true" and fields["deleteLevel"] == b"HARD" and
                     save["onlineID"] == "openegg_probe" and world["localLevel"]["completion"] == 3 and
                     world["remoteLevel"]["completion"] == 2 and world["shared"]["completion"] == 4 and
                     save["unlocks"]["unlockedEntities"] == {"localEntity": 1, "remoteEntity": 1})
        except (KeyError, TypeError, ValueError):
            valid = False
        received.append(valid)
        self.reply(200 if valid else 400)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server-url", help="Disposable local replacement server")
    parser.add_argument("--user-id", help="Fixture account ID")
    parser.add_argument("--session-id", help="Fixture account session")
    args = parser.parse_args()
    server = None
    if args.server_url:
        endpoint = urlsplit(args.server_url)
        if (endpoint.scheme != "http" or endpoint.hostname != "127.0.0.1" or not endpoint.port or
                endpoint.username is not None or endpoint.password is not None or
                endpoint.path not in ("", "/") or endpoint.query or endpoint.fragment or
                not args.user_id or not args.session_id):
            parser.error("local server URL and disposable fixture identity are required")
        url, user, session = args.server_url.rstrip("/"), args.user_id, args.session_id
        path = f"{url}/otg/~identity:{user}/~content:progress"
        headers = {"X-OTG-Identity-SessionId": session,
                   "Content-Type": "multipart/form-data; boundary=" + BOUNDARY.decode()}
        request = Request(path + ".put", data=multipart(json.dumps(remote).encode()), headers=headers)
        with urlopen(request, timeout=10) as response:
            if response.status != 200:
                raise SystemExit("fixture cloud save could not be seeded")
    else:
        server = ThreadingHTTPServer(("127.0.0.1", 0), SyncHandler)
        server.daemon_threads = True
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        url, user, session = f"http://127.0.0.1:{server.server_port}", USER, SESSION
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-cloud-sync-") as name:
            temp = Path(name)
            engine = ROOT / "build/OpenEgg"
            shutil.copy2(engine, temp / engine.name)
            for archive in engine.parent.glob("*.psarc"):
                (temp / archive.name).symlink_to(archive)
            data = engine.parent / "game_data"
            if data.is_dir():
                (temp / "game_data").symlink_to(data, target_is_directory=True)
            (temp / "OpenEgg.ini").write_text(
                f"[network]\nserver_url = {url}\nuser_id = {user}\nsession_id = {session}\n")
            harness = temp / "harness.txt"
            harness.write_text("wait 240\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                       SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
            result = subprocess.run([str(temp / engine.name), str(ROOT / "tests/networking"),
                                     "CloudSyncProbe.gm", "Main"], env=env, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
            print(result.stdout, end="")
            if result.returncode or (server and received != [True]) or "retail cloud sync PASS" not in result.stdout or "[gm]" in result.stdout:
                raise SystemExit("retail cloud sync probe failed")
            local = json.loads((temp / "savedata" / "save.json").read_text())
            world = local["campaign"]["asides"]["world"]
            if (world["localLevel"]["completion"] != 3 or world["remoteLevel"]["completion"] != 2 or
                    world["shared"]["completion"] != 4 or
                    local["unlocks"]["unlockedEntities"] != {"localEntity": 1, "remoteEntity": 1}):
                raise SystemExit("local cloud save did not retain the merge")
            if args.server_url:
                request = Request(path + "/data.get", headers={"X-OTG-Identity-SessionId": session})
                with urlopen(request, timeout=10) as response:
                    saved = json.load(response)
                world = saved["campaign"]["asides"]["world"]
                if (world["localLevel"]["completion"] != 3 or world["remoteLevel"]["completion"] != 2 or
                        world["shared"]["completion"] != 4 or
                        saved["unlocks"]["unlockedEntities"] != {"localEntity": 1, "remoteEntity": 1}):
                    raise SystemExit("replacement server cloud save did not retain the merge")
    finally:
        if server:
            server.shutdown()
            server.server_close()
            thread.join()


if __name__ == "__main__":
    main()
