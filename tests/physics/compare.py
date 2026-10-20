#!/usr/bin/env python3
"""Compare the physics script probe with its retail PS3 capture."""

import argparse
from collections import defaultdict
import csv
from datetime import datetime, timezone
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from compare_probe import check_baseline_source, read_readings
from probe_expectations import expected_readings
from run import ROOT, percent, run_script

HERE = Path(__file__).resolve().parent
PROBE = HERE / "probe.gm"
BASELINE = HERE / "retail-baseline.tsv"
SCENES = HERE / "scenes.gm"
SCENE_BASELINE = HERE / "scenes-retail.tsv"
SCENE_PLAN = HERE / "scenes-plan.json"
REPORT = ROOT / "PHYSICS_TEST_REPORT.md"
PREFIXES = ("physicsBody.", "physicsData.", "physicsShape.", "shapeDef.",
            "collisionInfo.", "jointType.", "jointIter.", "Physics.")
SPECIAL = {"EntityManager.createEntity", "Entity.getPhysicsData",
           "Global.PHYSICSSHAPECOLLECTIONDEF"}
ALIASES = {"PhysicsBody.": "physicsBody.", "PhysicsData.": "physicsData.",
           "ShapeDef.": "shapeDef."}
ASSOCIATED = {
    "PhysicsBody.appendShapes.index": ("shapeDef.addCircle", "Global.PHYSICSSHAPECOLLECTIONDEF"),
}
SCENE_FIELDS = {
    "x": "physicsBody.getPos", "y": "physicsBody.getPos",
    "vx": "physicsBody.getVel", "vy": "physicsBody.getVel",
    "colliding": "physicsData.isColliding", "bullet": "physicsBody.isBullet",
}


def inventory():
    with (ROOT / "docs/api/function_notes.tsv").open() as source:
        return sorted(row["name"] for row in csv.DictReader(source, delimiter="\t")
                      if row["name"].startswith(PREFIXES) or row["name"] in SPECIAL)


def owner(name, names):
    if name.startswith("scene."):
        function = SCENE_FIELDS[name.rsplit(".", 1)[1]]
        if function not in names:
            raise ValueError(f"unknown scene function: {function}")
        return function
    converted = name
    for old, new in ALIASES.items():
        if name.startswith(old):
            converted = new + name[len(old):]
            break
    matches = [item for item in names if converted == item or converted.startswith(item + ".")]
    if not matches:
        raise ValueError(f"unmapped physics reading: {name}")
    return max(matches, key=len)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, default=ROOT / "build/OpenEgg")
    parser.add_argument("--report", type=Path, default=REPORT)
    args = parser.parse_args()
    names = inventory()
    log_dir = ROOT / "tests/_results/physics"
    log_dir.mkdir(parents=True, exist_ok=True)
    problems = []
    rows = []
    grouped = defaultdict(list)
    scenes = defaultdict(list)
    for probe, baseline, plan in ((PROBE, BASELINE, None),
                                  (SCENES, SCENE_BASELINE, SCENE_PLAN)):
        check_baseline_source(baseline, probe)
        declared = expected_readings(probe, plan)
        if len(declared) != len(set(declared)):
            raise ValueError(f"duplicate probe names: {probe}")
        _, error, log_path, _ = run_script(args.engine.resolve(), HERE, probe.name,
                                           HERE / "smoke.harness", 20, log_dir,
                                           strict="survey")
        retail, retail_dupes = read_readings(baseline)
        local, local_dupes = read_readings(log_path)
        if error:
            problems.append(f"{probe.name}: {error}")
        for label, values, dupes in (("retail", retail, retail_dupes),
                                     ("OpenEgg", local, local_dupes)):
            problems += [f"{probe.name} {label}: missing {key}"
                         for key in declared if key not in values]
            problems += [f"{probe.name} {label}: unexpected {key}"
                         for key in values if key not in declared]
            problems += [f"{probe.name} {label}: duplicate {key}"
                         for key in sorted(dupes)]
        for case in declared:
            function = owner(case, names)
            passed = retail.get(case) == local.get(case) and case in retail and case in local
            rows.append((case, function, retail.get(case), local.get(case), passed))
            grouped[function].append(passed)
            if case.startswith("scene."):
                scenes[case.split(".")[1]].append(passed)
            for extra in ASSOCIATED.get(case, ()):
                if extra not in names:
                    raise ValueError(f"unknown associated function: {extra}")
                grouped[extra].append(passed)
    count = sum(row[4] for row in rows)
    tested = sum(bool(grouped[name]) for name in names)
    lines = ["# Physics test report", "",
             f"Generated: {datetime.now(timezone.utc):%Y-%m-%d %H:%M UTC}", "",
             f"Matched: **{count}/{len(rows)} declared readings ({percent(count, len(rows))})**. "
             "This measures captured cases, not total physics parity.", "",
             f"Functions with readings: **{tested}/{len(names)}**. The inventory comes from "
             "the local API reference and includes OpenEgg-only methods.", "",
             "## Scenes", "",
             "Each scene samples script-visible position, velocity, and relevant contact state "
             "at 12 successive ticks.", "",
             "| Scene | Matched | Cases |", "| --- | ---: | ---: |"]
    for scene, checks in scenes.items():
        lines.append(f"| {scene} | {sum(checks)} | {len(checks)} |")
    lines += ["",
             "## Per function", "",
             "| Function | Matched | Cases | Status |", "| --- | ---: | ---: | --- |"]
    for function in names:
        checks = grouped[function]
        matched = sum(checks)
        status = "untested" if not checks else "PASS" if matched == len(checks) else "FAIL"
        lines.append(f"| `{function}` | {matched} | {len(checks)} | {status} |")
    lines += ["", "## Readings", "", "| Function | Case | Retail | OpenEgg | Result |",
              "| --- | --- | --- | --- | --- |"]
    for case, function, a, b, passed in rows:
        lines.append(f"| `{function}` | {case} | `{a}` | `{b}` | {'PASS' if passed else 'FAIL'} |")
    if problems:
        lines += ["", "## Capture problems", ""] + [f"- {item}" for item in problems]
    args.report.write_text("\n".join(lines) + "\n")
    print(f"physics: {count}/{len(rows)} matched, {len(problems)} capture problems")
    print(f"Report: {args.report}")
    return 1 if count != len(rows) or problems else 0


if __name__ == "__main__":
    sys.exit(main())
