#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Playtendoh's reports, from the intake (tools/log-intake, GET /v1/reports), into a local folder for triage
(docs/CRASH-TRIAGE.md): the first of its tools, standard library only.

    python3 -B tools/triage/fetch_reports.py --new                 what came since the last fetch
    python3 -B tools/triage/fetch_reports.py --since 2d --result crashed,failed --side switch2
    python3 -B tools/triage/fetch_reports.py --since 2026-11-20 --match 6.0.0 --index-only

The intake answers only with its READ_TOKEN: PLAYTENDOH_REPORTS_TOKEN in the environment, or, in a cloud
session, the network secret the session's proxy puts on requests to the intake's host (then none is needed
here). PLAYTENDOH_REPORTS_URL names another intake.

Each report goes into --out (build/triage/reports, which git ignores) as <id>.json, or .json.gz with
--store-gz, and its fields without the logs into index.jsonl; --new goes on after the last id there
(state.json). Every crashed, failed and ended report is kept; with --sample-closed N, only every Nth closed
one (the app or console closed under the game: the bulk, and mostly normal). Nothing here writes to the
intake. A report holds no install ID, only a console (a hash of it), and the summary printed counts
consoles and never names one.
"""

import argparse
import datetime
import gzip
import json
import os
import re
import shutil
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

DEFAULT_URL = "https://playtendoh-reports.pixelnet321.workers.dev"
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_OUT = os.path.join(REPO_ROOT, "build", "triage", "reports")
PAGE = 200  # the intake's most a request (READ_MOST)
PROBLEMS = ("crashed", "failed")


def parse_since(text):
    """Seconds since 1970 from "2d", "12h", "90m", "2026-11-20" or a number of seconds."""
    if re.fullmatch(r"\d{9,12}", text):
        return int(text)
    match = re.fullmatch(r"(\d+)([dhm])", text)
    if match:
        unit = {"d": 86400, "h": 3600, "m": 60}[match.group(2)]
        return int(time.time()) - int(match.group(1)) * unit
    try:
        day = datetime.datetime.strptime(text, "%Y-%m-%d").replace(tzinfo=datetime.timezone.utc)
    except ValueError:
        raise argparse.ArgumentTypeError(f"--since: 2d, 12h, 90m, 2026-11-20 or seconds, not {text!r}")
    return int(day.timestamp())


def request(base, params, token, timeout=30):
    url = base.rstrip("/") + "/v1/reports?" + urllib.parse.urlencode(params)
    headers = {"accept": "application/json", "user-agent": "playtendoh-triage"}
    if token:
        headers["authorization"] = "Bearer " + token
    with urllib.request.urlopen(urllib.request.Request(url, headers=headers), timeout=timeout) as answer:
        return json.load(answer)


def free_gb(folder):
    while not os.path.isdir(folder):
        folder = os.path.dirname(folder)
    return shutil.disk_usage(folder).free / 1e9


def load_state(out):
    try:
        with open(os.path.join(out, "state.json"), encoding="utf-8") as file:
            return json.load(file)
    except (OSError, ValueError):
        return {}


def save_state(out, state):
    staged = os.path.join(out, "state.json.new")
    with open(staged, "w", encoding="utf-8") as file:
        json.dump(state, file, indent=1)
    os.replace(staged, os.path.join(out, "state.json"))


def write_report(out, report, gz):
    name = os.path.join(out, f"{report['id']:08d}.json" + (".gz" if gz else ""))
    data = json.dumps(report, ensure_ascii=False, indent=1).encode("utf-8")
    if gz:
        with gzip.open(name, "wb") as file:
            file.write(data)
    else:
        with open(name, "wb") as file:
            file.write(data)


def summary(reports):
    """Counts only: each side's endings, then the games with the most crashes and failures, with how many
    consoles. Never a console's ID."""
    lines = [f"{len(reports)} reports"]
    by_side = {}
    for r in reports:
        counts = by_side.setdefault((r.get("side"), r.get("emulator")), {})
        counts[r.get("result")] = counts.get(r.get("result"), 0) + 1
    for (side, emulator), counts in sorted(by_side.items(), key=lambda item: -sum(item[1].values())):
        lines.append(f"  {side} ({emulator}): " + ", ".join(f"{n} {result}" for result, n in sorted(counts.items(), key=lambda c: -c[1])))
    games = {}
    for r in reports:
        if r.get("result") not in PROBLEMS:
            continue
        game = games.setdefault((r.get("side"), r.get("titleId")), {"name": r.get("name"), "crashed": 0, "failed": 0, "consoles": set(), "why": {}})
        game[r["result"]] += 1
        game["consoles"].add(r.get("console"))
        why = (r.get("error") or "").strip()[:100]
        if why:
            game["why"][why] = game["why"].get(why, 0) + 1
    if games:
        lines.append("crashes and failures, the most first:")
    for (side, title), game in sorted(games.items(), key=lambda item: -(item[1]["crashed"] + item[1]["failed"]))[:30]:
        top = max(game["why"].items(), key=lambda w: w[1])[0] if game["why"] else ""
        lines.append(f"  {side} {title} {game['name']}: {game['crashed']} crashed, {game['failed']} failed, "
                     f"{len(game['consoles'])} consoles" + (f"; most: {top}" if top else ""))
    return "\n".join(lines)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0], formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--new", action="store_true", help="what came since the last fetch into --out (oldest first)")
    parser.add_argument("--since", type=parse_since, help="2d, 12h, 90m, 2026-11-20 or seconds since 1970")
    parser.add_argument("--match", help="the app's version (6.0.0) or the first characters of its commit (76f7fd0)")
    parser.add_argument("--game", help="a title ID (16 hex digits)")
    parser.add_argument("--side", help="switch, switch2, wiiu, 3ds, ds, gamecube, n64...")
    parser.add_argument("--emulator", help="cemu, azahar, melonds, eden, eglantine, porpoise, a core...")
    parser.add_argument("--result", help="ended, failed, crashed, closed (a comma between)")
    parser.add_argument("--most", type=int, default=1000, help="reports at most (1000)")
    parser.add_argument("--index-only", action="store_true", help="the reports without their logs: a first look")
    parser.add_argument("--store-gz", action="store_true", help="each report compressed")
    parser.add_argument("--sample-closed", type=int, default=1, metavar="N", help="keep only every Nth closed report (1: all)")
    parser.add_argument("--min-free-gb", type=float, default=1.0, help="refuse to run with less free disk (1)")
    parser.add_argument("--out", default=DEFAULT_OUT, help="the folder the reports go in (build/triage/reports)")
    parser.add_argument("--url", default=os.environ.get("PLAYTENDOH_REPORTS_URL", DEFAULT_URL), help="the intake (no path)")
    args = parser.parse_args(argv)

    if args.game and not re.fullmatch(r"[0-9a-fA-F]{16}", args.game):
        parser.error("--game: a title ID, 16 hex digits")
    os.makedirs(args.out, exist_ok=True)
    if free_gb(args.out) < args.min_free_gb:
        print(f"fetch_reports: less than {args.min_free_gb} GB free in {args.out}: nothing fetched", file=sys.stderr)
        return 1
    token = os.environ.get("PLAYTENDOH_REPORTS_TOKEN", "")
    state = load_state(args.out)

    params = {"limit": str(PAGE)}
    if args.index_only:
        params["logs"] = "0"
    if args.new:
        params["after"] = str(state.get("last", 0))
    if args.since is not None:
        params["since"] = str(args.since)
    if args.match:
        params["commit" if re.fullmatch(r"[0-9a-f]{4,40}", args.match) and "." not in args.match else "app"] = args.match
    for name in ("game", "side", "emulator", "result"):
        value = getattr(args, name)
        if value:
            params["title" if name == "game" else name] = value.lower() if name == "game" else value

    # --new: oldest first, after the last id kept; else the newest first, each page before the last
    key = "after" if args.new else "before"
    fetched, closed_seen = [], 0
    while len(fetched) < args.most:
        params["limit"] = str(min(PAGE, args.most - len(fetched)))
        try:
            page = request(args.url, params, token)
        except urllib.error.HTTPError as error:
            why = {401: "the intake wants its read token (PLAYTENDOH_REPORTS_TOKEN, or the session's network secret)",
                   404: "the intake has no READ_TOKEN set, or this is not its address (tools/log-intake/README.md)",
                   429: "too many requests: try again in a minute"}.get(error.code, f"HTTP {error.code}")
            print(f"fetch_reports: {why}", file=sys.stderr)
            return 1
        except (urllib.error.URLError, OSError, ValueError) as error:
            print(f"fetch_reports: the intake could not be reached: {error}", file=sys.stderr)
            return 1
        reports = page.get("reports", [])
        fetched.extend(reports)
        if not page.get("more") or not reports:
            break
        params[key] = str(page["next"])

    index = []
    for report in fetched:
        if report.get("result") == "closed":
            closed_seen += 1
            if args.sample_closed > 1 and closed_seen % args.sample_closed:
                continue
        if not args.index_only:
            write_report(args.out, report, args.store_gz)
        index.append({k: v for k, v in report.items() if k not in ("log", "logs")})
    if index:
        with open(os.path.join(args.out, "index.jsonl"), "a", encoding="utf-8") as file:
            for row in index:
                file.write(json.dumps(row, ensure_ascii=False) + "\n")
    if fetched and args.new:
        state["last"] = max(r["id"] for r in fetched)
    state["fetched"] = int(time.time())
    save_state(args.out, state)
    print(summary(fetched))
    print(f"{len(index)} kept in {os.path.relpath(args.out)}" + (" (index only)" if args.index_only else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
