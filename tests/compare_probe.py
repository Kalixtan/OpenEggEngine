#!/usr/bin/env python3
"""Compare sequencer probe readings from retail and OpenEgg."""

import argparse
from collections import defaultdict
import csv
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import subprocess
import sys

from run import ROOT, api_inventory, percent, run_script


PROBE = ROOT / "tests" / "sequencer" / "probe.gm"
BASELINE = ROOT / "tests" / "sequencer" / "retail-baseline.tsv"
PLANNED = ROOT / "tests" / "sequencer" / "planned.json"
READING = re.compile(r"OEP__([A-Za-z0-9_.]+)__(.*?)\.gm")
DECLARED = re.compile(r'SeqProbe\("([A-Za-z0-9_.]+)"')
KNOWN_GAPS = {}
ALIASES = {
    "SequencerAction.field.default": "SEQUENCERACTION",
    "Sequencer.callback.step": "Sequencer.setTrigger",
    "Sequencer.callback.count": "Sequencer.setTrigger",
}
ASSOCIATED = {
    "Sequencer.getBPM.default": ["SEQUENCER"],
    "SequencerListRef.size.initial": ["SEQUENCER_GetAllSequencers"],
    "Sequencer.isPlaying.paused": ["SEQUENCER_PauseAllSequencers"],
    "Sequencer.isPlaying.unpaused": ["SEQUENCER_UnpauseAllSequencers"],
    "Sequencer.getNumTriggers.cleared": ["Sequencer.clearTriggers"],
    "SequencerTriggerList.size.three": ["Sequencer.getAllTriggers"],
    "Sequencer.getCurrentStep.raw": ["Sequencer.gotoStep"],
    "Sequencer.getCurrentStep.restarted": ["Sequencer.restart"],
    "Sequencer.getBPM.changed": ["Sequencer.setBPM"],
    "Sequencer.getCurrentStep.set": ["Sequencer.setCurrentStep"],
    "Sequencer.getNextNotePattern.wrap": ["Sequencer.setLoopLen"],
    "Sequencer.getNumStepsPerPattern.float": ["Sequencer.setNumStepsPerPattern"],
    "Sequencer.isPlaying.started": ["Sequencer.start"],
    "Sequencer.isPlaying.stopped": ["Sequencer.stop"],
    "SequencerAction.isTaggedForRemoval.tagged": ["SequencerAction.tagForRemoval"],
    "SequencerListRef.isValid.next": ["SequencerListRef.goToNext"],
    "SequencerListRef.isValid.reset": ["SequencerListRef.reset"],
    "Sequencer.setTrigger.implicit.pattern": ["SequencerTrigger.getPattern"],
    "Sequencer.setTrigger.implicit.step": ["SequencerTrigger.getStep"],
    "Sequencer.setTrigger.implicit.looped": ["SequencerTrigger.getLooped"],
    "SequencerTriggerList.get.nextStep": ["SequencerTriggerList.goToNext"],
    "SequencerTriggerList.get.resetStep": ["SequencerTriggerList.reset"],
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
        if name in data:
            duplicates.add(name)
        data[name] = value
    return data, duplicates


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


def documented_sequencer_api():
    names = set()
    prefixes = {
        "sequencer.": "Sequencer.",
        "sequencerAction.": "SequencerAction.",
        "sequencerTrigger.": "SequencerTrigger.",
        "sequencerTriggerList.": "SequencerTriggerList.",
        "sequencerListRef.": "SequencerListRef.",
    }
    with (ROOT / "docs" / "api" / "function_notes.tsv").open() as source:
        for row in csv.DictReader(source, delimiter="\t"):
            name = row["name"]
            if name.startswith("Global.SEQUENC"):
                names.add(name.removeprefix("Global."))
            for prefix, renamed in prefixes.items():
                if name.startswith(prefix):
                    names.add(renamed + name[len(prefix):])
    return names


def make_report(rows, inventory, retail_path, local_path, problems, assertions,
                planned, missing_api):
    matched = sum(row["status"] == "PASS" for row in rows)
    total = len(rows)
    gaps = sum(row["status"] == "GAP" for row in rows)
    uncaptured = sum(row["status"] == "NO RETAIL" for row in rows)
    checks = assertions["entries"]
    local_passed = sum(case["result"] == "PASS" for case in checks)
    lines = [
        "# Sequencer test report", "",
        f"Generated: {datetime.now(timezone.utc):%Y-%m-%d %H:%M UTC}",
        f"Retail: `{display_path(retail_path)}` | OpenEgg: `{display_path(local_path)}`", "",
        f"Matched: **{matched}/{total} declared readings ({percent(matched, total)})** | "
        f"Known gaps: **{gaps}** | Uncaptured: **{uncaptured}** | "
        f"Planned: **{len(planned)}**", "",
        f"Local assertions: **{local_passed}/{len(checks)} passed "
        f"({percent(local_passed, len(checks))})**", "",
        f"Verdict: **{'all captured readings match' if matched == total and not problems else 'paired run incomplete or mismatched'}**. "
        f"**{len(planned)} edge cases still need paired retail runs.**", "",
        "One row is one exact string comparison from the same `probe.gm` on both engines. "
        "A missing reading or duplicate fails the run. Scores measure tested cases only; "
        "they are not total implementation progress. "
        "Integration checks can count for several functions. `OEPV__` clock readings "
        "stay outside the score.", "",
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
        "| Function | Paired score | Local checks | Planned | Status |",
        "| --- | ---: | ---: | ---: | --- |",
    ])
    grouped = defaultdict(list)
    for row in rows:
        for function in row["functions"]:
            grouped[function].append(row)
    local_grouped = defaultdict(list)
    for case in checks:
        for function in case["functions"]:
            local_grouped[function].append(case)
    planned_grouped = defaultdict(list)
    for case in planned:
        for function in case["functions"]:
            planned_grouped[function].append(case)
    for function in inventory:
        cases = grouped[function]
        count = sum(case["status"] == "PASS" for case in cases)
        local_cases = local_grouped[function]
        local_count = sum(case["result"] == "PASS" for case in local_cases)
        status = "untested" if not cases else (
            "FAIL" if any(case["status"] == "FAIL" for case in cases) or
                      any(case["result"] == "FAIL" for case in local_cases) else
            "GAP" if any(case["status"] == "GAP" for case in cases) or
                     any(case["result"] == "GAP" for case in local_cases) else
            "PARTIAL" if any(case["status"] == "NO RETAIL" for case in cases) else "PASS"
        )
        if status == "PASS" and planned_grouped[function]:
            status = "PENDING"
        lines.append(f"| `{function}` | {count}/{len(cases)} ({percent(count, len(cases))}) | "
                     f"{local_count}/{len(local_cases)} | "
                     f"{len(planned_grouped[function])} | {status} |")
    lines.extend(["", "## Planned cases", "",
                  "| Case | Functions | Why |", "| --- | --- | --- |"])
    for case in planned:
        functions = ", ".join(f"`{name}`" for name in case["functions"])
        lines.append(f"| {case['name']} | {functions} | {case['note']} |")
    lines.extend(["", f"Documented sequencer APIs missing from bindings: **{len(missing_api)}**."])
    if missing_api:
        lines.extend(f"- `{name}` — not implemented" for name in missing_api)
    lines.extend(["", "## Readings", "",
                  "| Function | Case | Retail | OpenEgg | Result |",
                  "| --- | --- | --- | --- | --- |"])
    for row in rows:
        lines.append(f"| `{row['function']}` | {row['name']} | "
                     f"`{row['retail'] if row['retail'] is not None else '—'}` | "
                     f"`{row['local'] if row['local'] is not None else '—'}` | "
                     f"{row['status']} |")
    if gaps:
        lines.extend(["", "## Known gaps", ""])
        for name, reason in KNOWN_GAPS.items():
            lines.append(f"- `{name}`: {reason}.")
    if problems or assertions["problems"]:
        lines.extend(["", "## Run problems", ""])
        lines.extend(f"- {problem}" for problem in problems)
        lines.extend(f"- {problem}" for problem in assertions["problems"])
    if uncaptured:
        lines.extend(["", "Missing retail readings are marked NO RETAIL and fail the run."])
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
    planned = json.loads(PLANNED.read_text())
    planned_names = set()
    for case in planned:
        if case["name"] in planned_names:
            raise ValueError(f"duplicate planned case: {case['name']}")
        planned_names.add(case["name"])
        unknown = set(case["functions"]) - set(inventory)
        if unknown:
            raise ValueError(f"{case['name']}: unknown planned functions: {sorted(unknown)}")
    missing_api = sorted(documented_sequencer_api() - set(inventory))
    expected = DECLARED.findall(PROBE.read_text())
    if len(expected) != len(set(expected)):
        raise ValueError("duplicate SeqProbe case names")
    process_error = ""
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
        elif a == b:
            status = "PASS"
        elif name in KNOWN_GAPS:
            status = "GAP"
        else:
            status = "FAIL"
        function = owning_function(name, inventory)
        functions = [function, *ASSOCIATED.get(name, [])]
        if set(functions) - set(inventory):
            raise ValueError(f"{name}: unknown assigned function")
        rows.append({"name": name, "function": function, "functions": functions,
                     "retail": a, "local": b, "status": status})
    problems = []
    if process_error:
        problems.append(f"OpenEgg probe run: {process_error}")
    for origin, readings, duplicates in (("retail", retail, retail_duplicates),
                                         ("OpenEgg", local, local_duplicates)):
        for name in sorted(set(readings) - set(expected)):
            problems.append(f"Unexpected {origin} reading: {name}")
        for name in sorted(duplicates):
            problems.append(f"Duplicate {origin} reading: {name}")
    args.report.write_text(make_report(rows, inventory, retail_path, local_path,
                                       problems, assertions, planned, missing_api))
    passed = sum(row["status"] == "PASS" for row in rows)
    gaps = sum(row["status"] == "GAP" for row in rows)
    failures = sum(row["status"] in ("FAIL", "NO RETAIL") for row in rows)
    print(f"probe: {passed}/{len(rows)} matched, {gaps} known gaps, {failures} failures")
    print(f"Report: {args.report}")
    local_failures = sum(case["result"] == "FAIL" for case in assertions["entries"])
    local_gaps = sum(case["result"] == "GAP" for case in assertions["entries"])
    return 1 if failures or local_failures or problems or missing_api or assertions["problems"] or (
        args.strict_parity and (gaps or local_gaps or planned)) else 0


if __name__ == "__main__":
    sys.exit(main())
