# Physics probes

`probe.gm` runs unchanged on retail PS3 and OpenEgg. The checked-in retail
baseline records the exact readings and hashes of the probe scripts.

To run both engines again, first capture retail from your own installed game:

```sh
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/probe.gm \
  --baseline tests/physics/retail-baseline.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/scenes.gm \
  --expected tests/physics/scenes-plan.json \
  --baseline tests/physics/scenes-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/contacts.gm \
  --support-file tests/physics/ProbeContactEntity.gm \
  --expected tests/physics/contacts-plan.json \
  --baseline tests/physics/contacts-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/removal.gm \
  --support-file tests/physics/ProbeContactEntity.gm \
  --expected tests/physics/removal-plan.json \
  --baseline tests/physics/removal-retail.tsv
python3 tests/physics/compare.py
```

The comparison runs OpenEgg headlessly and writes `PHYSICS_TEST_REPORT.md`.
`python3 tests/compare_all.py` runs this and the sequencer suite together.

The static probe covers body and shape setup. Scenes sample motion. Contacts
cover solids, sensors, repeated entry/separation, and callback order. Removal
checks body destruction inside a collision callback. The report lists each
reading and untested physics functions. Joints and raycasts still need probes.
