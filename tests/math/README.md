# Math probes

Vector, angle, range, geometry, and easing probes run headlessly on retail and
OpenEgg. The current capture matches **63/63** readings, including the four
previously mismatched edge cases.

Capture and compare with:

```sh
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/math/probe.gm \
  --expected tests/math/plan.json --baseline tests/math/retail.tsv
python3 tests/math/compare.py
```

`MATHUTIL_distance` is not in this paired probe: calling it interrupted retail
script execution before later readings.
