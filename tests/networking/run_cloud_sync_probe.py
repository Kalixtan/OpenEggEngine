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
from urllib.error import HTTPError
from urllib.request import Request, urlopen
from run_ticket_probe import ticket

from run_cloud_probe import BOUNDARY, Handler, ROOT, SESSION, USER, multipart

remote = {"onlineID": "openegg_probe", "version": 1,
          "unlocks": {"unlockedEntities": {"remoteEntity": 1}},
          "campaign": {"asides": {"world": {"remoteLevel": {"completion": 2},
                                                "shared": {"completion": 4}}}, "bsides": {}},
          "editor": {"editorTrophyLevels": {}}}
received = []
get_status = 200
test_case = "success"
EMAIL = "probe@example.test"
PASSWORD = 'pass#word;"quoted"'


class SyncHandler(Handler):
    def do_GET(self):
        if (self.path == f"/otg/~identity:{USER}/~content:progress/data.get" and
                self.headers.get("X-OTG-Identity-SessionId") == SESSION):
            self.reply(get_status, json.dumps(remote).encode() if get_status == 200 else b"")
        else:
            self.reply(404)

    def do_POST(self):
        global get_status
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
            if test_case == "delete":
                valid = save == {"onlineID": "openegg_probe"}
            else:
                world = save["campaign"]["asides"]["world"]
                valid = (save["onlineID"] == "openegg_probe" and world["localLevel"]["completion"] == 3 and
                         world["remoteLevel"]["completion"] == 2 and world["shared"]["completion"] == 4 and
                         save["unlocks"]["unlockedEntities"] == {"localEntity": 1, "remoteEntity": 1})
            valid = valid and fields["mutable"] == b"true" and fields["deleteLevel"] == b"HARD"
        except (KeyError, TypeError, ValueError):
            valid = False
        received.append(valid)
        if valid and test_case == "delete":
            get_status = 404
        self.reply(200 if valid else 400)


def main():
    global get_status, test_case
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server-url", help="Disposable local replacement server")
    parser.add_argument("--user-id", help="Fixture account ID")
    parser.add_argument("--session-id", help="Fixture account session")
    parser.add_argument("--case", choices=("success", "mismatched-id", "server-error", "initial", "delete"), default="success")
    args = parser.parse_args()
    test_case = args.case
    server = None
    if args.server_url:
        if args.case == "server-error":
            parser.error("server-error requires the local HTTP fixture")
        endpoint = urlsplit(args.server_url)
        if (endpoint.scheme != "http" or endpoint.hostname != "127.0.0.1" or not endpoint.port or
                endpoint.username is not None or endpoint.password is not None or
                endpoint.path not in ("", "/") or endpoint.query or endpoint.fragment or
                (args.case != "initial" and (not args.user_id or not args.session_id))):
            parser.error("local server URL and disposable fixture identity are required")
        url, user, session = args.server_url.rstrip("/"), args.user_id, args.session_id
        if args.case == "initial":
            login = Request(url + "/identity/login/account",
                            data=json.dumps({"email": EMAIL, "password": PASSWORD}).encode(),
                            headers={"Content-Type": "application/json",
                                     "X-OpenEgg-Client": "OpenEgg",
                                     "X-OpenEgg-Client-Version": "0.1.0"})
            with urlopen(login, timeout=20) as response:
                identity = json.load(response)["session"]
            user, session = identity["person"]["id"], identity["id"]
        path = f"{url}/otg/~identity:{user}/~content:progress"
        headers = {"X-OTG-Identity-SessionId": session,
                   "Content-Type": "multipart/form-data; boundary=" + BOUNDARY.decode()}
        if args.case != "initial":
            request = Request(path + ".put", data=multipart(json.dumps(remote).encode()), headers=headers)
            with urlopen(request, timeout=10) as response:
                if response.status != 200:
                    raise SystemExit("fixture cloud save could not be seeded")
    else:
        if args.case == "initial":
            parser.error("initial requires a disposable replacement server account")
        get_status = 503 if args.case == "server-error" else 200
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
            config = f"[network]\nserver_url = {url}\n"
            if args.case == "initial":
                (temp / "ticket.bin").write_bytes(ticket())
                config += "psn_online_id = openegg_probe\nticket_file = ticket.bin\n"
            else:
                config += f"user_id = {user}\nsession_id = {session}\n"
            (temp / "OpenEgg.ini").write_text(config)
            harness = temp / "harness.txt"
            harness.write_text(f"wait {900 if args.case == 'initial' else 240}\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                       SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
            script_dir = temp / "probe"
            script_dir.mkdir()
            source = (ROOT / "tests/networking/CloudSyncProbe.gm").read_text()
            (script_dir / "CloudSyncProbe.gm").write_text(f'global CLOUD_TEST_CASE = "{args.case}";\n' + source)
            result = subprocess.run([str(temp / engine.name), str(script_dir),
                                     "CloudSyncProbe.gm", "Main"], env=env, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=90)
            print(result.stdout, end="")
            marker = {"success": "retail cloud sync PASS", "mismatched-id": "retail cloud mismatch PASS",
                      "server-error": "retail cloud server error PASS", "initial": "retail cloud initial sync PASS",
                      "delete": "retail cloud delete PASS"}[args.case]
            expected_posts = [True] if args.case in ("success", "delete") else []
            if result.returncode or (server and received != expected_posts) or marker not in result.stdout or "[gm]" in result.stdout:
                raise SystemExit("retail cloud sync probe failed")
            if args.case not in ("success", "initial", "delete"):
                if (temp / "savedata" / "save.json").exists():
                    raise SystemExit("failed cloud sync unexpectedly wrote a local save")
                if args.server_url:
                    request = Request(path + "/data.get", headers={"X-OTG-Identity-SessionId": session})
                    with urlopen(request, timeout=10) as response:
                        if json.load(response) != remote:
                            raise SystemExit("failed cloud sync changed the server save")
                return
            local = json.loads((temp / "savedata" / "save.json").read_text())
            world = local["campaign"]["asides"]["world"]
            expected_world = ({"localLevel": {"completion": 3}, "shared": {"completion": 1}}
                              if args.case in ("initial", "delete") else
                              {"localLevel": {"completion": 3}, "remoteLevel": {"completion": 2}, "shared": {"completion": 4}})
            expected_unlocks = ({"localEntity": 1} if args.case in ("initial", "delete") else {"localEntity": 1, "remoteEntity": 1})
            if world != expected_world or local["unlocks"]["unlockedEntities"] != expected_unlocks:
                raise SystemExit("local cloud save did not retain the merge")
            if args.server_url:
                request = Request(path + "/data.get", headers={"X-OTG-Identity-SessionId": session})
                if args.case == "delete":
                    try:
                        urlopen(request, timeout=10).close()
                    except HTTPError as error:
                        if error.code == 404:
                            return
                        raise
                    raise SystemExit("retail cloud delete left the server save intact")
                with urlopen(request, timeout=10) as response:
                    saved = json.load(response)
                world = saved["campaign"]["asides"]["world"]
                if world != expected_world or saved["unlocks"]["unlockedEntities"] != expected_unlocks:
                    raise SystemExit("replacement server cloud save did not retain the merge")
    finally:
        if server:
            server.shutdown()
            server.server_close()
            thread.join()


if __name__ == "__main__":
    main()
