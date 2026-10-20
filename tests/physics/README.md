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
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/queries.gm \
  --support-file tests/physics/ProbeContactEntity.gm \
  --expected tests/physics/queries-plan.json \
  --baseline tests/physics/queries-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/rays.gm \
  --support-file tests/physics/ProbeContactEntity.gm \
  --expected tests/physics/rays-plan.json \
  --baseline tests/physics/rays-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/filter_reentry.gm \
  --support-file tests/physics/ProbeContactEntity.gm \
  --expected tests/physics/filter_reentry-plan.json \
  --baseline tests/physics/filter_reentry-retail.tsv
python3 tests/physics/compare.py
```

The comparison runs OpenEgg headlessly and writes `PHYSICS_TEST_REPORT.md`.
`python3 tests/compare_all.py` runs this and the sequencer suite together.

The static probe covers body and shape setup. Scenes sample motion. Contacts
cover solids, sensors, repeated entry/separation, and callback order. Removal
checks body destruction inside a collision callback. Queries cover collision
info and filter changes; rays cover global, body, and PhysicsData calls. Filter
reentry checks a restored mask through small movement and full separation. The
report lists each reading and untested physics functions. Joints still need
probes.

Retail faults at `0x97420` when `CollisionInfo.getNumCollidingShapes()` is
called on an active pair in this probe. That call is excluded from the paired
score. The fault came from a separate headless RPCS3 run of the same pair.
