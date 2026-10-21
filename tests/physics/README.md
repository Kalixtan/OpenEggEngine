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
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/joints.gm \
  --expected tests/physics/joints-plan.json \
  --baseline tests/physics/joints-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/ray_motion.gm \
  --expected tests/physics/ray_motion-plan.json \
  --baseline tests/physics/ray_motion-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/entity_lifecycle.gm \
  --support-file tests/physics/LifecycleProbeEntity.gm \
  --expected tests/physics/entity_lifecycle-plan.json \
  --baseline tests/physics/entity_lifecycle-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/entity_manager.gm \
  --support-file tests/physics/EntityManagerProbeEntity.gm \
  --expected tests/physics/entity_manager-plan.json \
  --baseline tests/physics/entity_manager-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/entity_state.gm \
  --support-file tests/physics/EntityStateProbeEntity.gm \
  --expected tests/physics/entity_state-plan.json \
  --baseline tests/physics/entity_state-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/entity_callbacks.gm \
  --support-file tests/physics/EntityCallbackProbe.gm \
  --support-file tests/physics/EntityCallbackNoHooks.gm \
  --expected tests/physics/entity_callbacks-plan.json \
  --baseline tests/physics/entity_callbacks-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/process_order.gm \
  --support-file tests/physics/ProcessParityProbe.gm \
  --expected tests/physics/process_order-plan.json \
  --baseline tests/physics/process_order-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/process_pause.gm \
  --support-file tests/physics/ProcessPauseA.gm \
  --support-file tests/physics/ProcessPauseB.gm \
  --expected tests/physics/process_pause-plan.json \
  --baseline tests/physics/process_pause-retail.tsv
python3 tests/physics/compare.py
```

The comparison runs OpenEgg headlessly and writes `reports/PHYSICS.md`.
`python3 tests/compare_all.py` runs this and the sequencer suite together.

The static probe covers body and shape setup. Scenes sample motion. Contacts
cover solids, sensors, repeated entry/separation, and callback order. Removal
checks body destruction inside a collision callback. Queries cover collision
info and filter changes; rays cover global, body, and PhysicsData calls. Filter
reentry checks a restored mask through small movement and full separation.
Joint scenes sample five joint types over time; moving rays check hit order
after bodies move and overlap. The report keeps mismatches visible.
Entity lifecycle checks enable/disable, contact callbacks, re-enable, and
delete/flush while bodies overlap. Entity manager probes cover pending versus
committed entities, iterator snapshots, repeated tags, observer-queued deletion
and creation, and clear-all behavior. The entity state probe exercises exposed
state, identity, geometry, alarms, particles, compatibility stubs, and perf
cost calls. Its cases without retail captures remain outside the parity score.
Entity callback probe checks enable and disable delegation separately from
the enabled flag, including repeated and missing behavior callbacks.
Process probes compare creation, deferred init, queued termination, pause
state, callback order, and registry counts with retail.

Retail faults at `0x97420` when `CollisionInfo.getNumCollidingShapes()` is
called on an active pair in this probe. That call is excluded from the paired
score. The fault came from a separate headless RPCS3 run of the same pair.
