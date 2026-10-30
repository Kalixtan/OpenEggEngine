#!/usr/bin/env python3
"""Check compressed level callbacks against a local HTTP fixture."""
import argparse
from collections import Counter
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import threading
import zlib
import re

from run_ticket_probe import ticket

ROOT = Path(__file__).resolve().parents[2]
SESSION = "00000000-0000-0000-0000-000000000001"
USER = "00000000-0000-0000-0000-000000000002"
seen = set()
descriptions = set()
sounds = set()
queued = set()
queued_counts = Counter()
request_counts = Counter()
ratings_favourites = set()
rating_put_bodies = Counter()
metadata = set()
metadata_requests = Counter()
leaderboard = set()
albums = set()
indexes = set()
activity = set()
social = set()
records = set()
level_lookups = set()
level_lookup_counts = Counter()
rating_path = f"/otg/~level:level-1/~rating:%2F%7Eidentity%3A{USER}"


class Handler(BaseHTTPRequestHandler):
    def reply(self, status, body=b""):
        self.send_response(status)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        request_counts[("POST", self.path)] += 1
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        if self.path == f"/otg/~identity:{USER}/~record:%2F%7Elevel%3Alevel-1.post":
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION and self.headers.get("Content-Type") == "www-form-urlencoded":
                if not body:
                    records.add("empty")
                elif set(body.decode().split("&")) == {"score=4294967296", "completed=1", "playTime=1.250000"}:
                    records.add("values")
            self.reply(200, b'{"ok":1}')
            return
        if self.path == "/otg/~identity:user-1/~activity:*/entry.post":
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION and body == b"":
                activity.add("post")
            self.reply(200, b'{"ok":1}')
            return
        if self.path == "/otg/~level:level-1/~leaderboard.post":
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION and body in (b"score=42", b"score=4294967296"):
                leaderboard.add(body.decode())
            self.reply(200, b'{"score":4294967296}')
            return
        if self.path == "/otg/~level:level-1/~metadata:field.put":
            metadata_requests[("POST", self.headers.get("Content-Type", ""), body)] += 1
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION and body == b"value" and self.headers.get("Content-Type") == "text/plain":
                metadata.add("put")
            self.reply(200, b'{"ok":1}')
            return
        if self.path == rating_path + ".put":
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                rating_put_bodies[body] += 1
                ratings_favourites.add(self.path)
            self.reply(200, b'{"ok":1}')
            return
        if self.path != "/identity/login/token/psn":
            self.reply(404)
            return
        if len(body) != len(ticket()) or body[:2] != b"\x20\x01" or b"openegg_probe" not in body:
            self.reply(400)
            return
        data = ('{"session":{"id":"' + SESSION + '","person":{"id":"' + USER + '"}}}').encode()
        self.reply(201, data)

    def do_GET(self):
        request_counts[("GET", self.path)] += 1
        prefix = "/otg/~level:level-1/~version:"
        suffix = "/~content:level/data.get"
        if self.path.startswith(prefix) and self.path.endswith(suffix):
            version = self.path[len(prefix):-len(suffix)]
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                seen.add(version)
            if version == "json":
                self.reply(200, zlib.compress(b'{"bpm":120,"screenData":[{"screenX":3}],"entities":[]}'))
            elif version == "xml":
                xml = b'<element key="bpm" value="120"/><tableElement key="screenData"><tableElement><element key="screenX" value="3"/></tableElement><indexSize/></tableElement>'
                self.reply(200, zlib.compress(xml))
            elif version == "invalid":
                self.reply(200, b"invalid compressed data")
            elif version == "null-callback":
                self.reply(200, zlib.compress(b'{"bpm":120,"screenData":[],"entities":[]}'))
            else:
                self.reply(404)
        elif self.path.startswith("/otg/~level:level-1/~version:") and self.path.endswith("/~content:sound/data.get"):
            version = self.path[len("/otg/~level:level-1/~version:"):-len("/~content:sound/data.get")]
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                sounds.add(version)
            if version == "compressed":
                signatures = re.findall(r'^\s*"([^"]+)",?$', (ROOT / "src/bindings/audio_recorder_signatures.h").read_text(), re.MULTILINE)
                text = "\n".join(["FunctionObjectRecorderV4", str(len(signatures)), *signatures, "0", "60000", "0", ""])
                self.reply(200, zlib.compress(text.encode()))
            elif version == "fun":
                self.reply(200, b"Fun raw payload")
            elif version == "null-callback":
                self.reply(200, b"Fun raw payload")
            elif version == "invalid":
                self.reply(200, b"not compressed")
            else:
                self.reply(404)
        elif self.path.startswith("/otg/~level:level-1/~upload:") and self.path.endswith(".get"):
            upload = self.path[len("/otg/~level:level-1/~upload:"):-len(".get")]
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                descriptions.add(upload)
            if upload == "good":
                self.reply(200, b'{"title":"Fixture"}')
            elif upload == "invalid":
                self.reply(200, b'{invalid')
            elif upload == "partial":
                self.reply(200, b'{"title":"Retained","other":')
            elif upload == "null-callback":
                self.reply(200, b'{"title":"Fixture"}')
            else:
                self.reply(404)
        elif self.path.startswith(f"/otg/~identity:{USER}/~queued:"):
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                queued.add(self.path)
                queued_counts[self.path] += 1
            self.reply(200, b'{"ok":1}')
        elif self.path in {rating_path + ".get", "/otg/~level:level-1/~favourite:*.all",
                           "/otg/~level:level-1/~favourite:item.put",
                           "/otg/~level:level-1/~favourite:item.delete"}:
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                ratings_favourites.add(self.path)
            self.reply(200, b'{"ok":1}')
        elif self.path in {"/otg/~level:level-1/~metadata:*.get", "/otg/~level:level-1/~metadata:field.get", "/otg/~level:level-1/~metadata:field.put"}:
            if self.path.endswith("field.put"):
                metadata_requests[("GET", self.headers.get("Content-Type", ""), b"")] += 1
                if self.headers.get("X-OTG-Identity-SessionId") == SESSION and self.headers.get("Content-Type") == "text/plain":
                    metadata.add("empty")
            elif self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                metadata.add({"/otg/~level:level-1/~metadata:*.get": "all", "/otg/~level:level-1/~metadata:field.get": "get"}[self.path])
            self.reply(200, b'{"ok":1}')
        elif self.path in {"/otg/~level:level-1/~leaderboard.get", "/otg/~level:level-1/~leaderboard.all", "/otg/~level:level-1/~leaderboard.page?count=2", "/otg/~level:level-1/~leaderboard.page?broken=1", "/otg/~level:level-1/~leaderboard.near?entrant=%2F%7Eidentity%3Aone&count=2"}:
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                leaderboard.add(self.path)
            self.reply(200, b'{"score":4294967296,' if "broken=1" in self.path else b'{"score":4294967296}')
        elif self.path in {"/otg/~albums/~link:*.page?from=1&count=2", "/otg/~album:album-1/~link:*.page?from=1&count=2", "/otg/~album:/~link:*.page"}:
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                albums.add(self.path)
            self.reply(200, b'{"ok":1}')
        elif self.path in {"/otg/~index:level.page?type=level&count=1", "/otg/~index:*.page?count=1", "/otg/~index:.page?typeX", "/otg/~index:*.page?type", "/otg/~index:alpha.page?type=alpha&type=beta", "/otg/~index:.page?type=&type=alpha", *(f"/otg/~index:{name}.page?type={name}" for name in ("broken", "partial", "array", "items", "number", "string", "null", "bool", "integer-overflow", "negative-integer-overflow", "float-overflow", "float-underflow"))}:
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                indexes.add(self.path)
            bodies = {
                "/otg/~index:broken.page?type=broken": b'{invalid',
                "/otg/~index:partial.page?type=partial": b'{"first":7,"nested":{"done":2,"unfinished":',
                "/otg/~index:array.page?type=array": b'{"values":[1,2,',
                "/otg/~index:items.page?type=items": b'{"items":[{"id":"first"},{"id":"second"}]}',
                "/otg/~index:number.page?type=number": b'42',
                "/otg/~index:string.page?type=string": b'"hello"',
                "/otg/~index:null.page?type=null": b'null',
                "/otg/~index:bool.page?type=bool": b'true',
                "/otg/~index:integer-overflow.page?type=integer-overflow": b'{"before":1,"overflow":9223372036854775808,"after":2}',
                "/otg/~index:negative-integer-overflow.page?type=negative-integer-overflow": b'{"before":1,"overflow":-9223372036854775809,"after":2}',
                "/otg/~index:float-overflow.page?type=float-overflow": b'{"before":1,"overflow":1e400,"after":2}',
                "/otg/~index:float-underflow.page?type=float-underflow": b'{"before":1,"tiny":1e-4000,"after":2}',
            }
            self.reply(200, bodies.get(self.path, b'{"count":0,"items":[]}'))
        elif self.path in {"/otg/~identity:user-1/~stream:news.page?count=2", "/otg/~identity:/~stream:.page"}:
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                activity.add("page" if "user-1" in self.path else "empty")
            self.reply(200, b'{"ok":1}')
        elif self.path in {"/otg/~level:level-1.latest", "/otg/~level:level-1/~version:*.get", "/otg/~level:level-1.delete"}:
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                level_lookups.add(self.path)
                level_lookup_counts[self.path] += 1
                if self.path.endswith(".delete"):
                    social.add(self.path)
            self.reply(200, b'{"ok":1}')
        elif self.path in {
            "/otg/~identity:/~follow:*.page?count=2", f"/otg/~identity:{USER}/~followers.page?count=2",
            f"/otg/~identity:{USER}/~follow:*.all", f"/otg/~identity:{USER}/~like:*.page?count=2",
            *(f"/otg/~identity:{USER}/~follow:%2F%7Eidentity%3Atarget.{op}" for op in ("get", "put", "delete")),
            "/otg/~level:level-1.get", "/otg/~level:level-1.put",
            f"/otg/~identity:{USER}/~like:%2F%7Elevel%3Alevel-1.delete",
            f"/otg/~level:level-1/~rating:%2F%7Eidentity%3A{USER}.delete",
        }:
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                social.add(self.path)
            self.reply(200, b'{"ok":1}')
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
    parser.add_argument("--version-id", help="Current level version timestamp for the server probe")
    args = parser.parse_args()
    server = None
    if args.server_url:
        if (not args.server_url.startswith("http://127.0.0.1:") or not args.level_id or
                not args.level_id.isalnum() or not args.version_id or not args.version_id.isdecimal()):
            parser.error("local server URL, fixture level ID, and current numeric version ID required")
        url = args.server_url.rstrip("/")
    else:
        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        server.daemon_threads = True
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        url = f"http://127.0.0.1:{server.server_port}"
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-level-") as name:
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
            harness = temp / "harness.txt"
            harness.write_text("wait 240\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                       SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
            script_dir = ROOT / "tests/networking"
            probe = "LevelDataProbe.gm"
            if args.server_url:
                probe = "ServerLevelDataProbe.gm"
                script_dir = temp / "probe"
                script_dir.mkdir()
                source = ((ROOT / "tests/networking" / probe).read_text()
                          .replace("__LEVEL_ID__", args.level_id)
                          .replace("__VERSION_ID__", args.version_id))
                (script_dir / probe).write_text(source)
            result = subprocess.run([str(temp / engine.name), str(script_dir), probe, "Main"],
                                    cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
            print(result.stdout, end="")
            markers = (("server level data PASS", "server sound data PASS", "server queue roundtrip PASS") if args.server_url else
                       ("level requests queued PASS", "level JSON PASS", "level XML PASS", "level invalid data PASS", "level missing PASS", "level null callback queued PASS", "description requests queued PASS", "description JSON PASS", "description malformed PASS", "description missing PASS", "description null callback queued PASS", "sound request queued PASS", "sound compressed PASS", "sound fun prefix PASS", "sound invalid PASS", "sound missing PASS", "sound null callback queued PASS", "queued get route PASS", "queued put route PASS", "queued delete route PASS"))
            expected_queue = {f"/otg/~identity:{USER}/~queued:%2F%7Elevel%3Asimple.get", f"/otg/~identity:{USER}/~queued:simple.put", f"/otg/~identity:{USER}/~queued:%2F%7Elevel%3Asimple.delete", f"/otg/~identity:{USER}/~queued:*.page?count=2"}
            expected_ratings = {rating_path + ".get", rating_path + ".put", "/otg/~level:level-1/~favourite:*.all", "/otg/~level:level-1/~favourite:item.put", "/otg/~level:level-1/~favourite:item.delete"}
            expected_leaderboard = {"/otg/~level:level-1/~leaderboard.get", "/otg/~level:level-1/~leaderboard.all", "/otg/~level:level-1/~leaderboard.page?count=2", "/otg/~level:level-1/~leaderboard.page?broken=1", "/otg/~level:level-1/~leaderboard.near?entrant=%2F%7Eidentity%3Aone&count=2", "score=42", "score=4294967296"}
            expected_albums = {"/otg/~albums/~link:*.page?from=1&count=2", "/otg/~album:album-1/~link:*.page?from=1&count=2", "/otg/~album:/~link:*.page"}
            expected_indexes = {"/otg/~index:level.page?type=level&count=1", "/otg/~index:*.page?count=1", "/otg/~index:.page?typeX", "/otg/~index:*.page?type", "/otg/~index:alpha.page?type=alpha&type=beta", "/otg/~index:.page?type=&type=alpha", *(f"/otg/~index:{name}.page?type={name}" for name in ("broken", "partial", "array", "items", "number", "string", "null", "bool", "integer-overflow", "negative-integer-overflow", "float-overflow", "float-underflow"))}
            expected_lookups = {"/otg/~level:level-1.latest", "/otg/~level:level-1/~version:*.get", "/otg/~level:level-1.delete"}
            expected_social = {"/otg/~identity:/~follow:*.page?count=2", f"/otg/~identity:{USER}/~followers.page?count=2", f"/otg/~identity:{USER}/~follow:*.all", f"/otg/~identity:{USER}/~like:*.page?count=2", *(f"/otg/~identity:{USER}/~follow:%2F%7Eidentity%3Atarget.{op}" for op in ("get", "put", "delete")), "/otg/~level:level-1.get", "/otg/~level:level-1.put", "/otg/~level:level-1.delete", f"/otg/~level:level-1/~rating:%2F%7Eidentity%3A{USER}.delete"}
            if server:
                markers += ("queued page route PASS", "queued null callback PASS", "rating/favourite offline gate PASS", "rating get response PASS", "rating put response PASS", "rating put zero response PASS", "rating put negative response PASS", "rating put max response PASS", "rating put min response PASS", "favourite all response PASS", "favourite put response PASS", "favourite delete response PASS", "metadata all PASS", "metadata get PASS", "metadata put PASS", "metadata empty PASS", "leaderboard get PASS", "leaderboard all PASS", "leaderboard page PASS", "leaderboard partial PASS", "leaderboard near PASS", "leaderboard integer PASS", "leaderboard string PASS", "leaderboard invalid score PASS", "album list PASS", "album contents PASS", "album empty PASS", "index typed PASS", "index wildcard PASS", "index malformed PASS", "index partial PASS", "index partial array PASS", "index item count PASS", "activity page PASS", "activity empty PASS", "activity post PASS", "record values PASS", "record empty PASS", "level lookup snapshot PASS", "level lookup versions PASS", "level lookup unpublish PASS", "null callback request routes PASS", *(f"social {name} PASS" for name in ("following", "followers", "all", "get", "put", "delete", "hearts", "heart get", "heart put", "heart delete", "self follow")))
                markers += tuple(f"index scalar {name} PASS" for name in ("number", "string", "null", "bool"))
                markers += tuple(f"index {name} PASS" for name in ("integer overflow", "negative integer overflow", "float overflow", "float underflow"))
                markers += tuple(f"index {name} PASS" for name in ("flag", "bare", "duplicate", "empty type"))
                markers += ("snapshot null callback PASS", "versions null callback PASS")
                markers += ("heart null callbacks queued PASS",)
            checks = {
                "levels": (seen, {"json", "xml", "invalid", "missing", "null-callback"}),
                "descriptions": (descriptions, {"good", "invalid", "missing", "partial", "null-callback"}),
                "sounds": (sounds, {"compressed", "fun", "invalid", "missing", "null-callback"}),
                "queue": (queued, expected_queue),
                "ratings and favourites": (ratings_favourites, expected_ratings),
                "metadata": (metadata, {"all", "get", "put", "empty"}),
                "leaderboard": (leaderboard, expected_leaderboard),
                "albums": (albums, expected_albums),
                "indexes": (indexes, expected_indexes),
                "activity": (activity, {"page", "empty", "post"}),
                "social": (social, expected_social),
                "records": (records, {"values", "empty"}),
                "level lookups": (level_lookups, expected_lookups),
            }
            failed = False
            if server:
                for name, (actual, expected) in checks.items():
                    if actual != expected:
                        print(f"{name}: missing={sorted(expected - actual)}, unexpected={sorted(actual - expected)}")
                        failed = True
                null_callback_path = f"/otg/~identity:{USER}/~queued:simple.put"
                if queued_counts[null_callback_path] != 1:
                    print(f"queued null callback: expected one PUT request, got {queued_counts[null_callback_path]}")
                    failed = True
                for path in ("/otg/~level:level-1.latest", "/otg/~level:level-1/~version:*.get"):
                    if level_lookup_counts[path] != 1:
                        print(f"level lookup null callback: expected one request to {path}, got {level_lookup_counts[path]}")
                        failed = True
                heart_paths = (
                    "/otg/~level:level-1.get",
                    "/otg/~level:level-1.put",
                    f"/otg/~level:level-1/~rating:%2F%7Eidentity%3A{USER}.delete",
                )
                for path in heart_paths:
                    count = request_counts[("GET", path)]
                    if count != 2:
                        print(f"heart null callback: expected two GET {path}, got {count}")
                        failed = True
                delete_count = request_counts[("GET", "/otg/~level:level-1.delete")]
                if delete_count != 3:  # unlike callback, null callback, and unpublish
                    print(f"level delete route: expected three GET requests, got {delete_count}")
                    failed = True
                once = (
                    ("GET", f"/otg/~index:level.page?type=level&count=1"),
                    ("GET", f"/otg/~level:level-1/~metadata:field.get"),
                    ("POST", f"/otg/~level:level-1/~metadata:field.put"),
                    ("GET", f"/otg/~level:level-1/~metadata:field.put"),
                    ("GET", rating_path + ".get"),
                    ("GET", f"/otg/~identity:{USER}/~follow:%2F%7Eidentity%3Atarget.put"),
                )
                for key in once:
                    if request_counts[key] != 1:
                        print(f"null callback: expected one {key[0]} {key[1]}, got {request_counts[key]}")
                        failed = True
                expected_rating_put_bodies = Counter({b"value=1": 1, b"value=0": 1, b"value=-1": 1,
                                                      b"value=2147483647": 1, b"value=-2147483648": 1})
                if rating_put_bodies != expected_rating_put_bodies:
                    print(f"rating put bodies: expected {expected_rating_put_bodies}, got {rating_put_bodies}")
                    failed = True
                rating_put_request = ("POST", rating_path + ".put")
                if request_counts[rating_put_request] != 5:
                    print(f"rating put count: expected five posts, got {request_counts[rating_put_request]}")
                    failed = True
                expected_metadata_requests = Counter({("POST", "text/plain", b"value"): 1,
                                                      ("GET", "text/plain", b""): 1})
                if metadata_requests != expected_metadata_requests:
                    print(f"metadata methods/bodies: expected {expected_metadata_requests}, got {metadata_requests}")
                    failed = True
                empty_metadata_put = ("GET", "/otg/~level:level-1/~metadata:field.put")
                if request_counts[empty_metadata_put] != 1:
                    print(f"empty metadata PUT operation: expected one GET request, got {request_counts[empty_metadata_put]}")
                    failed = True
                leaderboard_post = ("POST", "/otg/~level:level-1/~leaderboard.post")
                if request_counts[leaderboard_post] != 2:
                    print(f"null callback: expected two score posts, got {request_counts[leaderboard_post]}")
                    failed = True
            missing = [marker for marker in markers if marker not in result.stdout]
            if server and "description partial PASS" not in result.stdout:
                missing.append("description partial PASS")
            if missing:
                print(f"missing markers: {missing}")
            if result.returncode or failed or missing or "callback FAIL" in result.stdout:
                raise SystemExit("level data probe failed")
    finally:
        if server:
            server.shutdown()
            server.server_close()
            thread.join()


if __name__ == "__main__":
    main()
