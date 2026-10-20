"""Names expected from a paired script probe."""

import json
import re

DECLARED = re.compile(r'(?:SeqProbe|Probe)\("([A-Za-z0-9_.]+)"')


def expected_readings(probe, plan=None):
    if plan is None:
        return DECLARED.findall(probe.read_text())
    config = json.loads(plan.read_text())
    prefix = config.get("prefix", "scene")
    readings = [f"{prefix}.{scene}.t{tick}.{field}"
                for tick in range(config["frames"])
                for scene, fields in config["scenes"].items()
                for field in fields]
    return readings + config.get("extra", [])
