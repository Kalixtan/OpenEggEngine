#!/usr/bin/env python3
"""Compare sequencer probe readings from retail and OpenEgg."""

import argparse
from collections import defaultdict
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import re
import subprocess
import sys

from run import ROOT, api_inventory, percent, run_script


PROBE = ROOT / "tests" / "sequencer" / "probe.gm"
BASELINE = ROOT / "tests" / "sequencer" / "retail-baseline.tsv"
READING = re.compile(r"OEP__([A-Za-z0-9_.]+)__(.*?)\.gm")
DECLARED = re.compile(r'SeqProbe\("([A-Za-z0-9_.]+)"')
KNOWN_GAPS = {
    "SequencerTrigger.getAction.fresh": "retail creates a new action wrapper",
    "SequencerTrigger.getAction.repeat": "retail creates a wrapper on each call",
    "SequencerTriggerList.get.fresh": "retail creates a new trigger wrapper",
}
ALIASES = {
    "SequencerAction.field.default": "SEQUENCERACTION",
    "Sequencer.callback.step": "Sequencer.setTrigger",
    "Sequencer.callback.count": "Sequencer.setTrigger",
}


def read_readings(path):
    data = {}
    duplicates = set()
    for line in path.read_text(errors="replace").splitlines():
        match = READING.search(line)
        if match:
            name, value = match.groups()
        elif "\t" in line and not line.startswith("#"):
            name, value = line.split("\t", 1)
        else:
            continue
        if name in data and data[name] != value:
            duplicates.add(name)
        data[name] = value
    return data, duplicates


def same_value(left, right):
    if left == right:
        return True
    try:
        a, b = float(left), float(right)
    except ValueError:
        return False
    return math.isfinite(a) and math.isfinite(b) and math.isclose(a, b, abs_tol=0.01, rel_tol=0)


def owning_function(name, inventory):
    if name in ALIASES:
        return ALIASES[name]
    matches = [function for function in inventory if name.startswith(function + ".")]
    if not matches:
        raise ValueError(f"probe has no sequencer function: {name}")
    return max(matches, key=len)


def display_path(path):
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


def make_report(rows, inventory, retail_path, local_path, problems, assertions):
    matched = sum(row["status"] == "PASS" for row in rows)
    paired = sum(row["retail"] is not None for row in rows)
    gaps = sum(row["status"] == "GAP" for row in rows)
    uncaptured = sum(row["status"] == "NO RETAIL" for row in rows)
    checks = assertions["entries"]
    local_passed = sum(case["result"] == "PASS" for case in checks)
    lines = [
        "# Sequencer test report", "",
        f"Generated: {datetime.now(timezone.utc):%Y-%m-%d %H:%M UTC}  ",
        f"Retail: `{display_path(retail_path)}`  ",
        f"OpenEgg: `{display_path(local_path)}`  ",
        f"Matched: **{matched}/{paired} retail readings ({percent(matched, paired)})**  ",
        f"Known gaps: **{gaps}**  Uncaptured retail readings: **{uncaptured}**", "",
        f"Local assertions: **{local_passed}/{len(checks)} passed "
        f"({percent(local_passed, len(checks))})**", "",
        "One row is one function reading from the same `probe.gm` on both engines. "
        "Scores measure tested cases only; they are not total implementation progress. "
        "Integration checks can count for several functions. Missing retail callback "
        "readings stay outside the paired score.", "",
        "## Sections", "",
        "| Section | Passed | Cases | Score |", "| --- | ---: | ---: | ---: |",
    ]
    sections = defaultdict(list)
    for case in checks:
        sections[case["section"]].append(case)
    for section, cases in sections.items():
        count = sum(case["result"] == "PASS" for case in cases)
        lines.append(f"| {section} | {count} | {len(cases)} | {percent(count, len(cases))} |")
    lines.extend([
        "", "## Per function", "",
        "| Function | Retail | Local checks | Combined score | Status |",
        "| --- | ---: | ---: | ---: | --- |",
    ])
    grouped = defaultdict(list)
    for row in rows:
        grouped[row["function"]].append(row)
    local_grouped = defaultdict(list)
    for case in checks:
        for function in case["functions"]:
            local_grouped[function].append(case)
    for function in inventory:
        cases = grouped[function]
        paired_cases = [case for case in cases if case["retail"] is not None]
        count = sum(case["status"] == "PASS" for case in paired_cases)
        local_cases = local_grouped[function]
        local_count = sum(case["result"] == "PASS" for case in local_cases)
        total_passed = count + local_count
        total_cases = len(paired_cases) + len(local_cases)
        status = "untested" if not total_cases else (
            "FAIL" if any(case["status"] == "FAIL" for case in cases) or
                      any(case["result"] == "FAIL" for case in local_cases) else
            "GAP" if any(case["status"] == "GAP" for case in cases) or
                     any(case["result"] == "GAP" for case in local_cases) else
            "NO RETAIL" if not paired_cases else
            "PARTIAL" if any(case["status"] == "NO RETAIL" for case in cases) else "PASS"
        )
        lines.append(f"| `{function}` | {count}/{len(paired_cases)} | "
                     f"{local_count}/{len(local_cases)} | "
                     f"{total_passed}/{total_cases} ({percent(total_passed, total_cases)}) | {status} |")
    lines.extend(["", "## Readings", "",
                  "| Function | Case | Retail | OpenEgg | Result |",
                  "| --- | --- | --- | --- | --- |"])
    for row in rows:
        lines.append(f"| `{row['function']}` | {row['name']} | "
                     f"`{row['retail'] if row['retail'] is not None else '—'}` | "
                     f"`{row['local'] if row['local'] is not None else '—'}` | "
                     f"{row['status']} |")
    lines.extend(["", "## Known gaps", ""])
    for name, reason in KNOWN_GAPS.items():
        lines.append(f"- `{name}`: {reason}.")
    if problems or assertions["problems"]:
        lines.extend(["", "## Run problems", ""])
        lines.extend(f"- {problem}" for problem in problems)
        lines.extend(f"- {problem}" for problem in assertions["problems"])
    if uncaptured:
        lines.extend(["", "The retail capture has no callback readings; the bundled run "
                      "faulted before those results appeared. They are marked NO RETAIL."])
    lines.extend(["", "Local assertion details: `tests/_results/assertions.md`.", ""])
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--retail-log", type=Path, default=BASELINE,
                        help="full RPCS3 TTY.log or saved TSV readings")
    parser.add_argument("--local-log", type=Path,
                        help="use a saved OpenEgg log instead of running the engine")
    parser.add_argument("--engine", type=Path, default=ROOT / "build" / "OpenEgg")
    parser.add_argument("--report", type=Path, default=ROOT / "TEST_REPORT.md")
    parser.add_argument("--strict-parity", action="store_true")
    parser.add_argument("--skip-assertions", action="store_true",
                        help="reuse the last local assertion results")
    args = parser.parse_args()
    assertions_json = ROOT / "tests" / "_results" / "assertions.json"
    if not args.skip_assertions:
        result = subprocess.run(
            [sys.executable, str(ROOT / "tests" / "run.py"),
             "--engine", str(args.engine.resolve())],
            cwd=ROOT, check=False)
        if result.returncode:
            print("local assertion runner reported failures", file=sys.stderr)
    assertions = json.loads(assertions_json.read_text())
    inventory = api_inventory()
    expected = DECLARED.findall(PROBE.read_text())
    if len(expected) != len(set(expected)):
        raise ValueError("duplicate SeqProbe case names")
    if args.local_log:
        local_path = args.local_log.resolve()
    else:
        log_dir = ROOT / "tests" / "_results" / "sequencer"
        log_dir.mkdir(parents=True, exist_ok=True)
        _, process_error, local_path, _ = run_script(
            args.engine.resolve(), PROBE.parent, PROBE.name,
            PROBE.parent / "smoke.harness", 30, log_dir, strict="survey")
        if process_error:
            print(process_error, file=sys.stderr)
    retail_path = args.retail_log.resolve()
    retail, retail_duplicates = read_readings(retail_path)
    local, local_duplicates = read_readings(local_path)
    rows = []
    for name in expected:
        a, b = retail.get(name), local.get(name)
        if b is None:
            status = "FAIL"
        elif a is None:
            status = "NO RETAIL"
        elif same_value(a, b):
            status = "PASS"
        elif name in KNOWN_GAPS:
            status = "GAP"
        else:
            status = "FAIL"
        rows.append({"name": name, "function": owning_function(name, inventory),
                     "retail": a, "local": b, "status": status})
    problems = []
    for origin, readings, duplicates in (("retail", retail, retail_duplicates),
                                         ("OpenEgg", local, local_duplicates)):
        for name in sorted(set(readings) - set(expected)):
            problems.append(f"Unexpected {origin} reading: {name}")
        for name in sorted(duplicates):
            problems.append(f"Conflicting {origin} readings: {name}")
    args.report.write_text(make_report(rows, inventory, retail_path, local_path,
                                       problems, assertions))
    passed = sum(row["status"] == "PASS" for row in rows)
    gaps = sum(row["status"] == "GAP" for row in rows)
    failures = sum(row["status"] == "FAIL" for row in rows)
    print(f"probe: {passed} matched, {gaps} known gaps, {failures} failures")
    print(f"Report: {args.report}")
    local_failures = sum(case["result"] == "FAIL" for case in assertions["entries"])
    local_gaps = sum(case["result"] == "GAP" for case in assertions["entries"])
    return 1 if failures or local_failures or problems or assertions["problems"] or (
        args.strict_parity and (gaps or local_gaps)) else 0


if __name__ == "__main__":
    sys.exit(main())
