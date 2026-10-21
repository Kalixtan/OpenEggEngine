#!/usr/bin/env python3
"""Compare sequencer probe readings from retail and OpenEgg."""

import argparse
from collections import defaultdict
import csv
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

from run import ROOT, api_inventory, percent, run_script


PROBE = ROOT / "tests" / "sequencer" / "probe.gm"
BASELINE = ROOT / "tests" / "sequencer" / "retail-baseline.tsv"
PLANNED = ROOT / "tests" / "sequencer" / "planned.json"
EDGE_BASELINES = ROOT / "tests" / "sequencer" / "edge-baselines"
READING = re.compile(r"OEPV?__([A-Za-z0-9_.]+)__(.*?)\.gm")
DECLARED = re.compile(r'(?:SeqProbe|Probe)\("([A-Za-z0-9_.]+)"')
VOLATILE_DECLARED = re.compile(r'(?:SeqProbeVolatile|ProbeVolatile)\("([A-Za-z0-9_.]+)"')
KNOWN_GAPS = {}
CLOCK_DEPENDENT = {
    "edge.bpm.negativeTickAfter",
    "edge.pattern.zeroTickAfter",
    "edge.pattern.negativeTickAfter",
    "edge.restart.currentStep",
}
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
EDGE_FUNCTIONS = {
    "edge.packed.oversizedLastWord": ["SEQUENCE_ToTable"],
    "edge.packed.oversizedIgnoredWord": ["SEQUENCE_ToTable"],
    "edge.packed.oversizedSourceLast": ["SEQUENCE_ToTable"],
    "edge.packed.exactLimitWord": ["SEQUENCE_ToTable"],
    "edge.packed.exactLimitWordRepeat": ["SEQUENCE_ToTable"],
    "edge.bpm.zeroTickInitial": ["Sequencer.getCurrentStep", "Sequencer.setBPM"],
    "edge.bpm.negativeTickInitial": ["Sequencer.getCurrentStep", "Sequencer.setBPM"],
    "edge.bpm.zeroTickAfter": ["Sequencer.getCurrentStep", "Sequencer.setBPM"],
    "edge.bpm.negativeTickAfter": ["Sequencer.getCurrentStep", "Sequencer.setBPM"],
    "edge.pattern.zeroTickInitial": ["Sequencer.getCurrentStep", "Sequencer.setNumStepsPerPattern"],
    "edge.pattern.negativeTickInitial": ["Sequencer.getCurrentStep", "Sequencer.setNumStepsPerPattern"],
    "edge.pattern.zeroTickAfter": ["Sequencer.getCurrentStep", "Sequencer.setNumStepsPerPattern"],
    "edge.pattern.negativeTickAfter": ["Sequencer.getCurrentStep", "Sequencer.setNumStepsPerPattern"],
    "edge.restart.initialStep": ["Sequencer.getCurrentStep", "Sequencer.restart"],
    "edge.restart.currentStep": ["Sequencer.getCurrentStep", "Sequencer.restart"],
    "edge.restart.actionOrder": ["Sequencer.setTrigger", "Sequencer.restart"],
}


def read_readings(path):
    data = {}
    duplicates = set()
    for line in path.read_text(errors="replace").splitlines():
        # RPCS3 logs each missing load twice. Keep the GameMonkey error line.
        if line.startswith("Error - Failed to load file ["):
            continue
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


def check_baseline_source(path, probe=PROBE, support=()):
    if path.suffix != ".tsv":
        return
    lines = path.read_text(errors="replace").splitlines()
    first = lines[0]
    expected = hashlib.sha256(probe.read_bytes()).hexdigest()
    if first != f"# probe-sha256 {expected}":
        raise ValueError(f"{path}: retail baseline is for an older or unknown probe")
    for source in support:
        expected = hashlib.sha256(source.read_bytes()).hexdigest()
        if f"# support-sha256 {source.name} {expected}" not in lines:
            raise ValueError(f"{path}: retail baseline is for an older support script: {source.name}")


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


def run_edge_probes(engine, planned):
    probe_dir = ROOT / "tests" / "sequencer"
    probes = [probe_dir / "edge_probe.gm", *sorted((probe_dir / "edge_inputs").glob("*.gm"))]
    log_dir = ROOT / "tests" / "_results" / "sequencer"
    log_dir.mkdir(parents=True, exist_ok=True)
    rows = []
    problems = []
    volatile_samples = []
    for probe in probes:
        baseline = EDGE_BASELINES / f"{probe.stem}.tsv"
        check_baseline_source(baseline, probe)
        retail, retail_duplicates = read_readings(baseline)
        _, process_error, local_path, _ = run_script(
            engine, probe.parent, probe.name, probe_dir / "smoke.harness",
            15, log_dir, strict="survey")
        local, local_duplicates = read_readings(local_path)
        source = probe.read_text()
        expected = DECLARED.findall(source)
        volatile = set(VOLATILE_DECLARED.findall(source))
        if len(expected) != len(set(expected)):
            raise ValueError(f"{probe}: duplicate declared reading")
        for name in expected:
            matches = [case for case in planned if any(
                name.startswith(prefix) for prefix in case["prefixes"])]
            if len(matches) != 1:
                raise ValueError(f"{probe}: {name} maps to {len(matches)} cases")
            case = matches[0]
            a, b = retail.get(name), local.get(name)
            status = "FAIL" if b is None else "NO RETAIL" if a is None else (
                "CLOCK" if name in CLOCK_DEPENDENT else
                "PASS" if a == b else "FAIL")
            functions = EDGE_FUNCTIONS.get(name, case["functions"])
            rows.append({"name": name, "function": functions[0],
                         "functions": functions, "case": case["name"],
                         "retail": a, "local": b, "status": status})
        if process_error:
            problems.append(f"{probe.name}: {process_error}")
        volatile_samples.extend((name, local.get(name)) for name in sorted(volatile))
        for origin, readings, duplicates in (("retail", retail, retail_duplicates),
                                             ("OpenEgg", local, local_duplicates)):
            for name in sorted(set(readings) - set(expected) - volatile):
                problems.append(f"Unexpected {origin} reading in {probe.name}: {name}")
            for name in sorted(duplicates - volatile):
                problems.append(f"Duplicate {origin} reading in {probe.name}: {name}")
    return rows, problems, volatile_samples


def make_report(rows, inventory, retail_path, local_path, problems, assertions,
                planned, missing_api, volatile_samples):
    matched = sum(row["status"] == "PASS" for row in rows)
    total = sum(row["status"] != "CLOCK" for row in rows)
    gaps = sum(row["status"] == "GAP" for row in rows)
    uncaptured = sum(row["status"] == "NO RETAIL" for row in rows)
    failures = sum(row["status"] == "FAIL" for row in rows)
    clock_samples = sum(row["status"] == "CLOCK" for row in rows)
    checks = assertions["entries"]
    local_passed = sum(case["result"] == "PASS" for case in checks)
    edge_rows = [row for row in rows if "case" in row]
    lines = [
        "# Sequencer test report", "",
        f"Generated: {datetime.now(timezone.utc):%Y-%m-%d %H:%M UTC}",
        f"Retail: `{display_path(retail_path)}` and `tests/sequencer/edge-baselines/` | "
        f"OpenEgg: `tests/_results/sequencer/`", "",
        f"Matched: **{matched}/{total} declared readings ({percent(matched, total)})** | "
        f"Known gaps: **{gaps}** | Failures: **{failures}** | Uncaptured: **{uncaptured}** | "
        f"Edge groups run: **{len(planned)}** | Clock samples: **{clock_samples}**", "",
        f"Local assertions: **{local_passed}/{len(checks)} passed "
        f"({percent(local_passed, len(checks))})**", "",
        f"Verdict: **{'all captured readings match' if matched == total and not problems else 'paired run incomplete or mismatched'}**. "
        "Coverage is limited to the readings below.", "",
        "One row is one exact string comparison from the same probe file on both engines. "
        "A missing reading or duplicate fails the run. Scores measure tested cases only; "
        "they are not total implementation progress. "
        "Integration checks can count for several functions. Clock samples "
        "stay visible but outside the score. Retail's sequencer reads elapsed "
        "wall time, so frame-count samples vary between runs.", "",
        "## Local assertion sections", "",
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
        "| Function | Paired score | Local checks | Edge groups | Status |",
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
        scored_cases = [case for case in cases if case["status"] != "CLOCK"]
        status = "untested" if not scored_cases else (
            "FAIL" if any(case["status"] == "FAIL" for case in cases) or
                      any(case["result"] == "FAIL" for case in local_cases) else
            "GAP" if any(case["status"] == "GAP" for case in cases) or
                     any(case["result"] == "GAP" for case in local_cases) else
            "PARTIAL" if any(case["status"] == "NO RETAIL" for case in cases) else "PASS"
        )
        if status == "PASS" and any(case["limit"] for case in planned_grouped[function]):
            status = "LIMITED"
        lines.append(f"| `{function}` | {count}/{len(scored_cases)} ({percent(count, len(scored_cases))}) | "
                     f"{local_count}/{len(local_cases)} | "
                     f"{len(planned_grouped[function])} | {status} |")
    lines.extend(["", "## Edge cases", "",
                  "| Case | Matched | Limits |", "| --- | ---: | --- |"])
    for case in planned:
        cases = [row for row in edge_rows if row["case"] == case["name"]]
        count = sum(row["status"] == "PASS" for row in cases)
        scored_cases = [row for row in cases if row["status"] != "CLOCK"]
        lines.append(f"| {case['name']} | {count}/{len(scored_cases)} | {case['limit']} |")
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
    if volatile_samples:
        lines.extend(["", "## Volatile readings (excluded from score)", "",
                      "These samples vary with runtime timing and are retained for review.", "",
                      "| Reading | OpenEgg |", "| --- | --- |"])
        for name, value in volatile_samples:
            lines.append(f"| `{name}` | `{value if value is not None else '—'}` |")
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
    lines.extend(["", "Retail crash warning: moving the last of multiple triggers faulted on PS3. "
                  "See `tests/sequencer/retail-crashes.md`."])
    lines.extend(["", "Local assertion details: `reports/LOCAL_ASSERTIONS.md`.", ""])
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--retail-log", type=Path, default=BASELINE,
                        help="full RPCS3 TTY.log or saved TSV readings")
    parser.add_argument("--local-log", type=Path,
                        help="use a saved OpenEgg log instead of running the engine")
    parser.add_argument("--engine", type=Path, default=ROOT / "build" / "OpenEgg")
    parser.add_argument("--report", type=Path, default=ROOT / "reports" / "SEQUENCER.md")
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
        if not case.get("prefixes"):
            raise ValueError(f"{case['name']}: no edge reading prefix")
    missing_api = sorted(documented_sequencer_api() - set(inventory))
    expected = DECLARED.findall(PROBE.read_text())
    volatile = set(VOLATILE_DECLARED.findall(PROBE.read_text()))
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
    check_baseline_source(retail_path)
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
    volatile_samples = [(name, local.get(name)) for name in sorted(volatile)]
    for origin, readings, duplicates in (("retail", retail, retail_duplicates),
                                         ("OpenEgg", local, local_duplicates)):
        for name in sorted(set(readings) - set(expected) - volatile):
            problems.append(f"Unexpected {origin} reading: {name}")
        for name in sorted(duplicates - volatile):
            problems.append(f"Duplicate {origin} reading: {name}")
    edge_rows, edge_problems, edge_volatile = run_edge_probes(args.engine.resolve(), planned)
    rows.extend(edge_rows)
    volatile_samples.extend(edge_volatile)
    problems.extend(edge_problems)
    for case in planned:
        if not any(row["case"] == case["name"] for row in edge_rows):
            problems.append(f"No paired readings for edge case: {case['name']}")
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(make_report(rows, inventory, retail_path, local_path,
                                       problems, assertions, planned, missing_api,
                                       volatile_samples))
    passed = sum(row["status"] == "PASS" for row in rows)
    gaps = sum(row["status"] == "GAP" for row in rows)
    failures = sum(row["status"] in ("FAIL", "NO RETAIL") for row in rows)
    scored = sum(row["status"] != "CLOCK" for row in rows)
    print(f"probe: {passed}/{scored} matched, {gaps} known gaps, {failures} failures")
    print(f"Report: {args.report}")
    local_failures = sum(case["result"] == "FAIL" for case in assertions["entries"])
    local_gaps = sum(case["result"] == "GAP" for case in assertions["entries"])
    return 1 if failures or local_failures or problems or missing_api or assertions["problems"] or (
        args.strict_parity and (gaps or local_gaps or any(case["limit"] for case in planned))) else 0


if __name__ == "__main__":
    sys.exit(main())
