"""Names expected from a paired script probe."""

import json
import re

DECLARED = re.compile(r'(?:SeqProbe|AnimProbe|ProbeVolatile|Probe)\("([A-Za-z0-9_.]+)"')


def expected_readings(probe, plan=None):
    if plan is None:
        return DECLARED.findall(probe.read_text())
    config = json.loads(plan.read_text())
    prefix = config.get("prefix", "scene")
    root = f"{prefix}." if prefix else ""
    readings = [f"{root}{scene}.t{tick}.{field}"
                for tick in range(config["frames"])
                for scene, fields in config["scenes"].items()
                for field in fields]
    return readings + config.get("extra", [])
