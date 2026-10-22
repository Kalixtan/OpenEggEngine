# Math probes

Vector, angle, range, geometry, and easing probes run headlessly on retail and
OpenEgg. The current capture matches **59/63** readings. Four edge cases differ:
zero-vector angle, negative angular distance, negative decay rate, and growth.

Capture and compare with:

```sh
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/math/probe.gm \
  --expected tests/math/plan.json --baseline tests/math/retail.tsv
python3 tests/math/compare.py
```

`MATHUTIL_distance` is not in this paired probe: calling it interrupted retail
script execution before later readings.
