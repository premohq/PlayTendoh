#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# tools/triage/fetch_reports.py against a stand-in for the intake (GET /v1/reports, its token, after,
# before and limit as tools/log-intake/src/index.js answers them): the newest first over pages, --new
# going on from where the last fetch stopped, the closed reports sampled, the logs left out with
# --index-only, a wrong token said in words, and a summary that counts consoles and never names one.
# Prints PASS lines; exits 1 on a failure.

import contextlib
import gzip
import http.server
import io
import json
import os
import sys
import tempfile
import threading
import urllib.parse

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "triage"))
import fetch_reports  # noqa: E402

TOKEN = "t" * 40
failures = 0


def check(ok, what):
    global failures
    print(f"{'PASS' if ok else 'FAIL'}  {what}")
    failures += 0 if ok else 1


def report(i, result="crashed", side="switch2", console="0123456789ab"):
    return {"id": i, "received": 1764000000 + i, "console": console, "app": "6.0.0", "commit": "76f7fd0", "side": side,
            "emulator": "eglantine" if side == "switch2" else "cemu", "titleId": "0100000000010000", "name": "Kirby Air Riders",
            "result": result, "error": "Lichii crashed (crash-report:<path>/crash-1.txt)" if result == "crashed" else None,
            "seconds": None, "firmware": "9.60", "model": "pro", "jit": True, "hen": True, "settings": {},
            "log": "[main] the launcher", "logs": [{"file": "hang-1.txt", "text": "[hang] the GPU"}]}


STORE = [report(i, "closed" if i % 3 == 0 else "crashed", console=f"{i % 4:012x}") for i in range(1, 451)]


class Intake(http.server.BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def do_GET(self):
        url = urllib.parse.urlparse(self.path)
        if url.path != "/v1/reports":
            return self.answer(404, {"error": "not found"})
        if self.headers.get("authorization") != "Bearer " + TOKEN:
            return self.answer(401, {"error": "token"})
        q = dict(urllib.parse.parse_qsl(url.query))
        rows = STORE
        if "after" in q:
            rows = sorted((r for r in rows if r["id"] > int(q["after"])), key=lambda r: r["id"])
        else:
            rows = sorted((r for r in rows if "before" not in q or r["id"] < int(q["before"])), key=lambda r: -r["id"])
        if "side" in q:
            rows = [r for r in rows if r["side"] == q["side"]]
        limit = min(int(q.get("limit", 50)), 200)
        rows = rows[:limit]
        if q.get("logs") == "0":
            rows = [{k: v for k, v in r.items() if k not in ("log", "logs")} for r in rows]
        start = q.get("after", q.get("before"))
        nxt = rows[-1]["id"] if rows else (int(start) if start is not None else None)
        self.answer(200, {"reports": rows, "next": nxt, "more": len(rows) == limit})

    def answer(self, status, body):
        data = json.dumps(body).encode()
        self.send_response(status)
        self.send_header("content-type", "application/json")
        self.send_header("content-length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)


def run(*args):
    out = io.StringIO()
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(out):
        code = fetch_reports.main(list(args))
    return code, out.getvalue()


def main():
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Intake)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    url = f"http://127.0.0.1:{server.server_port}"
    with tempfile.TemporaryDirectory() as work:
        out = os.path.join(work, "reports")
        os.environ["PLAYTENDOH_REPORTS_TOKEN"] = "wrong"
        code, text = run("--url", url, "--out", out, "--min-free-gb", "0")
        check(code == 1 and "read token" in text, "a wrong token: said in words, nothing fetched")

        os.environ["PLAYTENDOH_REPORTS_TOKEN"] = TOKEN
        code, text = run("--url", url, "--out", out, "--most", "300", "--min-free-gb", "0", "--store-gz")
        kept = sorted(f for f in os.listdir(out) if f.endswith(".json.gz"))
        check(code == 0 and len(kept) == 300 and kept[-1] == "00000450.json.gz" and kept[0] == "00000151.json.gz",
              "the newest 300, over two pages, each kept compressed")
        with gzip.open(os.path.join(out, kept[-1])) as file:
            check(json.load(file)["logs"][0]["file"] == "hang-1.txt", "a report keeps its emulator's logs")
        check("300 reports" in text and "consoles" in text and not any(f"{i:012x}" in text for i in range(4)),
              "the summary counts consoles and names none")

        code, text = run("--url", url, "--out", out, "--new", "--min-free-gb", "0", "--sample-closed", "3", "--most", "5000")
        with open(os.path.join(out, "state.json")) as file:
            state = json.load(file)
        check(code == 0 and state.get("last") == 450 and "450 reports" in text, "--new the first time: every report, oldest first, to the last")
        lines = open(os.path.join(out, "index.jsonl")).read().splitlines()
        closed = sum(1 for line in lines[300:] if json.loads(line)["result"] == "closed")
        check(closed == 50 and all("logs" not in json.loads(line) for line in lines), "every third closed one kept, the index without logs")

        STORE.append(report(451, "failed"))
        code, text = run("--url", url, "--out", out, "--new", "--min-free-gb", "0")
        with open(os.path.join(out, "state.json")) as file:
            check(code == 0 and "1 reports" in text and json.load(file)["last"] == 451, "--new again: only what came since")

        index_out = os.path.join(work, "index")
        code, text = run("--url", url, "--out", index_out, "--index-only", "--side", "switch2", "--most", "20", "--min-free-gb", "0")
        check(code == 0 and not [f for f in os.listdir(index_out) if f.endswith(".json") and f != "state.json"] and
              len(open(os.path.join(index_out, "index.jsonl")).read().splitlines()) == 20, "--index-only: the index alone")

        code, text = run("--url", url, "--out", out, "--min-free-gb", "100000")
        check(code == 1 and "free" in text, "below the free-disk floor: nothing fetched")
    server.shutdown()
    print("fetch_reports: all passed" if not failures else "fetch_reports: FAILED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
