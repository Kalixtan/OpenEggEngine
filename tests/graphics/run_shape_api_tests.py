#!/usr/bin/env python3
import os
import pathlib
import subprocess
import sys
import time


ROOT = pathlib.Path(__file__).resolve().parents[2]
ENGINE = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build/OpenEgg"
if not ENGINE.is_absolute():
    ENGINE = ROOT / ENGINE


CASES = [
    ("shape_api_ok", "SHAPE_API_TEST_BEGIN", "SHAPE_API_TEST_PASS", False),
    ("shape_api_video_vertex_forms", "VIDEO_VERTEX_FORMS_TEST_BEGIN", "VIDEO_VERTEX_FORMS_TEST_PASS", False),
    ("shape_api_bad_arity", "SHAPE_ARITY_PARITY_TEST_BEGIN", "SHAPE expects 3 arguments", True),
    ("shape_api_bad_arity_zero", "SHAPE_ZERO_ARITY_TEST_BEGIN", "SHAPE expects 3 arguments", True),
    ("shape_api_bad_arity_one", "SHAPE_ONE_ARITY_TEST_BEGIN", "SHAPE expects 3 arguments", True),
    ("shape_api_bad_arity_extra", "SHAPE_EXTRA_ARITY_TEST_BEGIN", "SHAPE expects 3 arguments", True),
    ("shape_api_bad_type", "SHAPE_BAD_TYPE_TEST_BEGIN", "SHAPE expects GameSystem", True),
    ("shape_api_bad_video_vertex_count", "VIDEO_VERTEX_COUNT_TEST_BEGIN", "callstack..", True),
    ("shape_api_bad_video_vertex_zero", "VIDEO_VERTEX_ZERO_TEST_BEGIN", "callstack..", True),
    ("shape_api_bad_video_vertex_selector_low", "VIDEO_SELECTOR_LOW_TEST_BEGIN", "callstack..", True),
    ("shape_api_bad_video_vertex_selector_high", "VIDEO_SELECTOR_HIGH_TEST_BEGIN", "callstack..", True),
    ("shape_api_bad_video_begin_arity", "VIDEO_BEGIN_ARITY_TEST_BEGIN", "callstack..", True),
    ("shape_api_bad_video_begin_zero", "VIDEO_BEGIN_ZERO_TEST_BEGIN", "callstack..", True),
]


def run_case(name, begin_marker, result_marker, expect_error):
    root = ROOT / "tests/graphics" / name
    env = os.environ.copy()
    env["GE3_HEADLESS"] = "1"
    proc = subprocess.Popen(
        [str(ENGINE), "--force-data-root", str(root), "Main.gm", "Main"],
        cwd=ROOT,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    time.sleep(2)
    was_running = proc.poll() is None
    if was_running:
        proc.terminate()
    output, _ = proc.communicate(timeout=5)
    failed = []
    if begin_marker not in output:
        failed.append(f"script did not reach {begin_marker}")
    if expect_error:
        if result_marker not in output:
            failed.append(f"missing expected failure marker: {result_marker}")
        if proc.returncode is not None and proc.returncode < 0:
            failed.append(f"host process crashed with signal {-proc.returncode}")
    else:
        if result_marker not in output:
            failed.append(f"missing success marker: {result_marker}")
        if "callstack.." in output:
            failed.append("valid shape path raised a GameMonkey error")
    status = "FAIL" if failed else "PASS"
    print(f"{status} {name}")
    for reason in failed:
        print(f"  {reason}")
    if failed:
        print(output[-3000:])
    return not failed


results = [run_case(*case) for case in CASES]
sys.exit(0 if all(results) else 1)
