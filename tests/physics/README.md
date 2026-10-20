# Physics probes

`probe.gm` runs unchanged on retail PS3 and OpenEgg. The checked-in retail
baseline records the exact readings and a hash of the probe source.

To run both engines again, first capture retail from your own installed game:

```sh
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/probe.gm \
  --baseline tests/physics/retail-baseline.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/scenes.gm \
  --expected tests/physics/scenes-plan.json \
  --baseline tests/physics/scenes-retail.tsv
python3 tests/physics/compare.py
```

The comparison runs OpenEgg headlessly and writes `PHYSICS_TEST_REPORT.md`.
`python3 tests/compare_all.py` runs this and the sequencer suite together.

The static probe covers body and shape setup. The scene probe samples position,
velocity, contacts, and bullet state across 12 ticks. The report lists each
reading and untested physics functions. Joints, raycasts, and destruction still
need probes.
