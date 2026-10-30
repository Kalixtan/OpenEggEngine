#!/usr/bin/env python3
"""Test EULA handling against a local HTTP fixture."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import threading

from run_ticket_probe import ticket

ROOT = Path(__file__).resolve().parents[2]
SESSION = "00000000-0000-0000-0000-000000000001"
USER = "00000000-0000-0000-0000-000000000002"
received = threading.Event()
friends_fetched = threading.Event()
featured_fetched = threading.Event()
news_fetched = threading.Event()
featured_localized = threading.Event()
news_localized = threading.Event()
translation_fetched = threading.Event()
fail = False
friends_fail = ""
community_fail = False
translation_body = False
translation_invalid = False
network_region = "SCEA"
network_locale = "en"


def locale_for(region, language):
    locales = ("ja", "en", "fr", "es", "de", "it", "nl", "pt", "ru", "ko",
               "zh_hant", "zh", "fi", "sv", "da", "no", "pl", "pt_br", "en_gb", "es_mx")
    if region == "SCEJ":
        return "ja" if language == 0 else "en"
    if region == "SCEE":
        return locales[language] if language in (2, 3, 4, 5, 6, 7, 8, 16) else "en_gb"
    if region == "SCEH":
        return locales[language] if language in (9, 10) else "en"
    return locales[language] if language in (2, 3) else "en"


def translation_catalog():
    entries = [(b"probe-key", b"probe-value"), (b"probe-key", b"probe"),
               (b"probe-key", b""), (b"replacement", b"old"),
               (b"replacement", b"new"), (b"empty-key", b"")]
    count = len(entries)
    originals, values = 28, 28 + count * 8
    blob = bytearray()
    keys, translations = [], []
    for key, value in entries:
        keys.append((len(key), 28 + count * 16 + len(blob)))
        blob.extend(key + b"\0")
        translations.append((len(value), 28 + count * 16 + len(blob)))
        blob.extend(value + b"\0")
    header = struct.pack("<7I", 0x950412de, 0, count, originals, values, 0, 0)
    return header + b"".join(struct.pack("<2I", *entry) for entry in keys + translations) + blob


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        self.rfile.read(int(self.headers.get("Content-Length", 0)))
        body = ('{"session":{"id":"' + SESSION + '","person":{"id":"' + USER +
                '","display_name":"openegg_probe"}}}').encode()
        self.reply(201, body)

    def do_GET(self):
        if self.path == "/otg/~identity:*.hello":
            self.reply(200, b"")
        elif self.path == f"/otg/ps3/{network_region}/{network_locale}/~eula.get":
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                received.set()
            self.reply(503 if fail else 200, b"" if fail else b"\xef\xbb\xbfOne\tTwo\n")
        elif self.path == f"/otg/~identity:{USER}/~friends.all":
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                friends_fetched.set()
            self.reply(503 if friends_fail == "fetch" else 200, b'{"friends":[{"id":"friend-1","displayName":"First"},{"id":"friend-2","displayName":"Second"}]}')
        elif self.path in ("/otg/global/featured/~metadata:*.get", "/otg/global/news/~metadata:*.get"):
            featured = "/featured/" in self.path
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                (featured_fetched if featured else news_fetched).set()
            self.reply(503 if community_fail else 200,
                       b'{"test":"featured"}' if featured else b'{"test":"news"}')
        elif self.path in (f"/otg/global/featured/{network_locale}/~metadata:*.get",
                           f"/otg/global/news/{network_locale}/~metadata:*.get"):
            featured = "/featured/" in self.path
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                (featured_localized if featured else news_localized).set()
            self.reply(503 if community_fail else 200,
                       ('{"localized":"featured ' + network_locale + '"}' if featured else
                        '{"localized":"news ' + network_locale + '"}').encode())
        elif self.path == f"/otg/ps3/{network_region}/{network_locale}/~translation.get":
            if self.headers.get("X-OTG-Identity-SessionId") == SESSION:
                translation_fetched.set()
            body = b"invalid" if translation_invalid else translation_catalog() if translation_body else b""
            self.reply(200, body)
        else:
            self.reply(404, b"")

    def reply(self, code, body):
        self.send_response(code)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args):
        pass


def main():
    global fail, friends_fail, community_fail, translation_body, translation_invalid, network_region, network_locale
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fail", action="store_true", help="Return an HTTP error for EULA")
    parser.add_argument("--friends-fail", choices=("fetch",), help="Return an error fetching replacement-server friends")
    parser.add_argument("--community-fail", action="store_true", help="Return errors for featured and news")
    parser.add_argument("--translation-body", action="store_true", help="Send a nonempty MO translation catalog")
    parser.add_argument("--translation-invalid", action="store_true", help="Send a malformed catalog")
    parser.add_argument("--ticket-in-ini", action="store_true", help="Store ticket bytes as inline INI hex")
    parser.add_argument("--region", choices=("SCEA", "SCEE", "SCEJ", "SCEH"), default="SCEA")
    parser.add_argument("--language", type=int, choices=range(20), default=1)
    parser.add_argument("--engine", type=Path, default=ROOT / "build" / "OpenEgg")
    args = parser.parse_args()
    fail = args.fail
    friends_fail = args.friends_fail or ""
    community_fail = args.community_fail
    translation_body = args.translation_body
    translation_invalid = args.translation_invalid
    network_region = args.region
    network_locale = locale_for(args.region, args.language)
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    server.daemon_threads = True
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        with tempfile.TemporaryDirectory(prefix="openegg-eula-") as name:
            temp = Path(name)
            engine = args.engine.resolve()
            shutil.copy2(engine, temp / engine.name)
            for asset in engine.parent.glob("*.psarc"):
                (temp / asset.name).symlink_to(asset)
            data = engine.parent / "game_data"
            if data.is_dir():
                (temp / "game_data").symlink_to(data, target_is_directory=True)
            ticket_bytes = ticket()
            if not args.ticket_in_ini:
                (temp / "ticket.bin").write_bytes(ticket_bytes)
            ticket_setting = (f"psn_ticket_hex = {ticket_bytes.hex()}\n" if args.ticket_in_ini
                              else "ticket_file = ticket.bin\n")
            (temp / "OpenEgg.ini").write_text(
                f"[system]\nlanguage = {args.language}\n[network]\nserver_url = http://127.0.0.1:{server.server_port}\n"
                f"psn_online_id = openegg_probe\n{ticket_setting}product_region = {args.region}\n")
            harness = temp / "harness.txt"
            harness.write_text("wait 240\nquit\n")
            env = os.environ.copy()
            env.update(GE3_HEADLESS="1", GE3_TRACE="1", GE3_STRICT="halt", GE3_HARNESS=str(harness),
                       SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1")
            result = subprocess.run([str(temp / engine.name), str(ROOT / "tests/networking"), "EulaProbe.gm", "Main"],
                                    cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, timeout=30)
            print(result.stdout, end="")
            markers = (("EULA failure PASS",) if fail else ("friends failure PASS",) if friends_fail else
                       ("network: initial community data requests complete",) if community_fail else
                       ("localized community data PASS", "coordinator online PASS", "network translation PASS") if translation_body else
                       ("network: invalid translation catalog", "coordinator online PASS") if translation_invalid else
                       ("EULA text and digest PASS", "EULA acceptance PASS", "friends snapshot PASS", "community data PASS",
                        "localized community data PASS", "coordinator online PASS", "coordinator offline PASS",
                        "replacement auth distinct from PSN PASS",
                        "ready offline while waiting PASS", "ready offline while online PASS",
                        "ready offline while offline PASS"))
            requests_ok = fail or friends_fetched.is_set()
            if not fail and not friends_fail:
                requests_ok = requests_ok and featured_fetched.is_set() and news_fetched.is_set()
                requests_ok = requests_ok and featured_localized.is_set() and news_localized.is_set() and translation_fetched.is_set()
            if (result.returncode or not received.is_set() or not requests_ok or
                    any(x not in result.stdout for x in markers) or "EULA acceptance version FAIL" in result.stdout or
                    "ready offline while waiting FAIL" in result.stdout or
                    "ready offline while online FAIL" in result.stdout or
                    "ready offline while offline FAIL" in result.stdout):
                raise SystemExit("EULA probe failed")
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


if __name__ == "__main__":
    main()
