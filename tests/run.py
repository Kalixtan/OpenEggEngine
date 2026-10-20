#!/usr/bin/env python3
"""Run the headless script probes and write TEST_REPORT.md."""

import argparse
from collections import defaultdict
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
MARKER = re.compile(r"^\[trace\] (.+?) (PASS|FAIL)\s*$", re.MULTILINE)
ENTRY = re.compile(r'\{"([^"]+)",\s*gmf_')
LIBRARY = re.compile(r'gmFunctionEntry (s_\w+Lib)\[\] = \{(.*?)\};', re.S)
GLOBAL = re.compile(r'RegisterLibraryFunction\("([^"]+)",')
TYPE_NAMES = {
    "s_sequencerLib": "Sequencer",
    "s_sequencerActionLib": "SequencerAction",
    "s_sequencerTriggerLib": "SequencerTrigger",
    "s_sequencerTriggerListLib": "SequencerTriggerList",
    "s_sequencerListRefLib": "SequencerListRef",
}


def percent(passed, total):
    return f"{passed * 100 / total:.0f}%" if total else "—"


def api_inventory():
    source = (ROOT / "src" / "bindings" / "sequencer_bindings.cpp").read_text()
    functions = set(GLOBAL.findall(source))
    for variable, body in LIBRARY.findall(source):
        if variable in TYPE_NAMES:
            functions.update(f"{TYPE_NAMES[variable]}.{name}" for name in ENTRY.findall(body))
    if len(functions) < 50:
        raise ValueError("sequencer API inventory looks incomplete")
    return sorted(functions)


def run_script(engine, suite_dir, script, harness, timeout, log_dir, strict="halt"):
    env = os.environ.copy()
    env.update({
        "GE3_HEADLESS": "1",
        "GE3_TRACE": "1",
        "GE3_STRICT": strict,
        "GE3_HARNESS": str(harness),
        "SDL_AUDIODRIVER": "dummy",
        "LIBGL_ALWAYS_SOFTWARE": "1",
    })
    command = [str(engine), str(suite_dir), script, "Main"]
    reason = ""
    try:
        result = subprocess.run(
            command, cwd=ROOT, env=env, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True, errors="replace", timeout=timeout,
            check=False,
        )
        output = result.stdout
        if result.returncode != 0:
            reason = f"engine exit {result.returncode}"
    except subprocess.TimeoutExpired as error:
        output = error.stdout or b""
        if isinstance(output, bytes):
            output = output.decode("utf-8", errors="replace")
        reason = f"timeout after {timeout}s"
    except OSError as error:
        output = str(error)
        reason = f"could not start engine: {error.strerror or error}"
    log_path = log_dir / f"{Path(script).stem}.log"
    log_path.write_text(output)
    observed = defaultdict(list)
    for name, result in MARKER.findall(output):
        observed[name].append(result)
    return observed, reason, log_path, output


def make_report(suite, engine, entries, problems, inventory):
    now = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")
    passed = sum(entry["result"] == "PASS" for entry in entries)
    gaps = sum(entry["result"] == "GAP" for entry in entries)
    lines = [
        "# Test report", "",
        f"Generated: {now}  ",
        f"Suite: {suite}  ",
        f"Engine: `{engine}`  ",
        f"Result: **{passed}/{len(entries)} checks passed ({percent(passed, len(entries))}); {gaps} known gaps**",
        "",
        "Scores count passing test cases. They do not measure total engine completeness."
        " One integration case may count for several functions.", "",
        "| System | Section | Passed | Total | Rate |", "| --- | --- | ---: | ---: | ---: |",
    ]
    groups = defaultdict(list)
    for entry in entries:
        groups[(entry["system"], entry["section"])].append(entry)
    for (system, section), cases in groups.items():
        count = sum(case["result"] == "PASS" for case in cases)
        lines.append(f"| {system} | {section} | {count} | {len(cases)} | {percent(count, len(cases))} |")
    by_function = defaultdict(list)
    for entry in entries:
        for function in entry["functions"]:
            by_function[function].append(entry)
    lines.extend(["", "## Functions", "", "| Function | Passed | Cases | Score | Status |",
                  "| --- | ---: | ---: | ---: | --- |"])
    for function in inventory:
        cases = by_function[function]
        count = sum(case["result"] == "PASS" for case in cases)
        status = "untested" if not cases else (
            "FAIL" if any(case["result"] == "FAIL" for case in cases) else
            "GAP" if any(case["result"] == "GAP" for case in cases) else "PASS")
        lines.append(f"| `{function}` | {count} | {len(cases)} | {percent(count, len(cases))} | {status} |")
    lines.extend(["", "## Checks", "", "| System / section | Test | Result | Retail basis |", "| --- | --- | --- | --- |"])
    for entry in entries:
        lines.append(
            f"| {entry['system']} / {entry['section']} | {entry['name']} | "
            f"{entry['result']} | {entry['retail']} |"
        )
    failed = [entry for entry in entries if entry["result"] != "PASS"]
    if failed or problems:
        lines.extend(["", "## Gaps and failures", ""])
        for entry in failed:
            lines.append(f"- **{entry['name']}**: {entry['reason']} (log: `{entry['log']}`)")
        for problem in problems:
            lines.append(f"- {problem}")
    lines.extend(["", "Known safety differences: retail crashes when moving the final trigger "
                  "and leaves a stale cursor after `clearTriggers()`; OpenEgg handles both safely.", ""])
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite", default="sequencer")
    parser.add_argument("--engine", type=Path, default=ROOT / "build" / "OpenEgg")
    parser.add_argument("--report", type=Path,
                        default=ROOT / "tests" / "_results" / "assertions.md")
    parser.add_argument("--json", type=Path,
                        default=ROOT / "tests" / "_results" / "assertions.json")
    parser.add_argument("--timeout", type=int, default=30)
    parser.add_argument("--strict-parity", action="store_true",
                        help="fail if a known retail parity gap remains")
    args = parser.parse_args()

    suite_dir = ROOT / "tests" / args.suite
    manifest = json.loads((suite_dir / "manifest.json").read_text())
    log_dir = ROOT / "tests" / "_results" / args.suite
    log_dir.mkdir(parents=True, exist_ok=True)
    engine = args.engine.resolve()
    report = args.report.resolve()
    entries = []
    problems = []
    names = set()
    inventory = api_inventory()

    for section in manifest["sections"]:
        script = section["script"]
        observed, process_error, log_path, output = run_script(
            engine, suite_dir, script, suite_dir / manifest["harness"],
            args.timeout, log_dir,
        )
        expected = {case["name"] for case in section["cases"]}
        for case in section["cases"]:
            name = case["name"]
            if name in names:
                raise ValueError(f"duplicate test name: {name}")
            names.add(name)
            signals = observed.get(name, [])
            if process_error:
                reason = process_error
            elif signals == ["PASS"]:
                reason = ""
            elif not signals:
                reason = "no result marker"
            else:
                reason = "reported " + ", ".join(signals)
            functions = case.get("functions", [])
            unknown = set(functions) - set(inventory)
            if unknown:
                raise ValueError(f"{name}: unknown functions: {', '.join(sorted(unknown))}")
            if not functions:
                raise ValueError(f"{name}: no functions assigned")
            status = "PASS" if not reason else "FAIL"
            if signals == ["FAIL"] and not process_error and case.get("known_gap"):
                status = "GAP"
                reason = case["known_gap"]
            entries.append({
                "system": section.get("system", args.suite.title()),
                "section": section["name"], "name": name,
                "retail": case.get("retail", section.get("retail", "local check")),
                "result": status, "reason": reason, "functions": functions,
                "log": str(log_path.relative_to(ROOT)),
            })
        for name, signals in observed.items():
            if name not in expected:
                problems.append(f"Unexpected result from `{script}`: {name} {'/'.join(signals)}")
        if not expected and not process_error:
            problems.append(f"`{script}` has no declared checks")

    report.write_text(make_report(args.suite, engine, entries, problems, inventory))
    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps({"entries": entries, "problems": problems}, indent=2) + "\n")
    passed = sum(entry["result"] == "PASS" for entry in entries)
    gaps = sum(entry["result"] == "GAP" for entry in entries)
    failures = sum(entry["result"] == "FAIL" for entry in entries)
    print(f"{args.suite}: {passed}/{len(entries)} passed, {gaps} known gaps, {failures} failures")
    print(f"Report: {report}")
    if problems:
        print(f"Unexpected results: {len(problems)}")
    return 1 if failures or problems or (args.strict_parity and gaps) else 0


if __name__ == "__main__":
    sys.exit(main())
