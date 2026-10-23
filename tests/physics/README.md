# Physics probes

`probe.gm` runs unchanged on retail PS3 and OpenEgg. The checked-in retail
baseline records the exact readings and hashes of the probe scripts.

The collision filter edge probe checks category masks, signed groups, and
full-width group values against retail. It also checks that groups `65537`
and `1` remain distinct when the masks reject contact.

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
  --eboot /path/to/EBOOT.BIN --probe tests/physics/body_commands.gm \
  --support-file tests/physics/BodyCommandProbeEntity.gm \
  --expected tests/physics/body_commands-plan.json \
  --baseline tests/physics/body_commands-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/shape_geometry.gm \
  --support-file tests/physics/EntityQueryProbeEntity.gm \
  --expected tests/physics/shape_geometry-plan.json \
  --baseline tests/physics/shape_geometry-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/data_geometry.gm \
  --support-file tests/physics/EntityQueryProbeEntity.gm \
  --expected tests/physics/data_geometry-plan.json \
  --baseline tests/physics/data_geometry-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/data_commands.gm \
  --support-file tests/physics/EntityQueryProbeEntity.gm \
  --expected tests/physics/data_commands-plan.json \
  --baseline tests/physics/data_commands-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/data_settings.gm \
  --support-file tests/physics/EntityQueryProbeEntity.gm \
  --expected tests/physics/data_settings-plan.json \
  --baseline tests/physics/data_settings-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/data_dynamics.gm \
  --support-file tests/physics/EntityQueryProbeEntity.gm \
  --expected tests/physics/data_dynamics-plan.json \
  --baseline tests/physics/data_dynamics-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/math.gm \
  --baseline tests/physics/math-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/math_edges.gm \
  --expected tests/physics/math_edges-plan.json \
  --baseline tests/physics/math_edges-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/body_surface.gm \
  --support-file tests/physics/EntityQueryProbeEntity.gm \
  --baseline tests/physics/body_surface-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/contacts.gm \
  --support-file tests/physics/ProbeContactEntity.gm \
  --expected tests/physics/contacts-plan.json \
  --baseline tests/physics/contacts-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/collision_filters.gm \
  --support-file tests/physics/EntityQueryProbeEntity.gm \
  --expected tests/physics/collision_filters-plan.json \
  --baseline tests/physics/collision_filters-retail.tsv
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
  --support-file tests/physics/EntityGCProbe.gm \
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
  --eboot /path/to/EBOOT.BIN --probe tests/physics/entity_gc.gm \
  --support-file tests/physics/EntityGCProbe.gm \
  --expected tests/physics/entity_gc-plan.json \
  --baseline tests/physics/entity_gc-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/entity_gc_cycles.gm \
  --support-file tests/physics/EntityGCProbe.gm \
  --expected tests/physics/entity_gc_cycles-plan.json \
  --baseline tests/physics/entity_gc_cycles-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/entity_gc_iterator.gm \
  --support-file tests/physics/EntityGCProbe.gm \
  --expected tests/physics/entity_gc_iterator-plan.json \
  --baseline tests/physics/entity_gc_iterator-retail.tsv
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
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/callback_mutation.gm \
  --support-file tests/physics/MutationProbeEntity.gm \
  --expected tests/physics/callback_mutation-plan.json \
  --baseline tests/physics/callback_mutation-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/entity_gc_pressure.gm \
  --support-file tests/physics/EntityGCProbe.gm \
  --expected tests/physics/entity_gc_pressure-plan.json \
  --baseline tests/physics/entity_gc_pressure-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/process_transition.gm \
  --support-file tests/physics/EntityGCProbe.gm \
  --support-file tests/physics/DeepOldProcess.gm \
  --support-file tests/physics/DeepSceneProcess.gm \
  --expected tests/physics/process_transition-plan.json \
  --baseline tests/physics/process_transition-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/EBOOT.BIN --probe tests/physics/process_pause_scale.gm \
  --support-file tests/physics/ProcessPauseScaleA.gm \
  --support-file tests/physics/ProcessPauseScaleB.gm \
  --expected tests/physics/process_pause_scale-plan.json \
  --baseline tests/physics/process_pause_scale-retail.tsv
python3 tests/physics/compare.py
```

The comparison runs OpenEgg headlessly and writes `reports/PHYSICS.md`.
`python3 tests/compare_all.py` runs this and the sequencer suite together.

The static probe covers body and shape setup. Scenes sample motion. Contacts
cover solids, sensors, repeated entry/separation, callback order, and the
PhysicsData colliding-entity iterator. Removal checks body destruction inside
a collision callback. Queries cover collision
info and filter changes; rays cover global, body, and PhysicsData calls. Filter
reentry checks a restored mask through small movement and full separation.
Body command probe follows thrust, braking, target rotation, and sleep/wake state
over 32 ticks per scenario; mismatches remain in the report for follow-up.
Joint scenes sample five joint types over time; moving rays check hit order
after bodies move and overlap. The report keeps mismatches visible.
Shape geometry probes polygon reads and edits, mass rebuilds, circle/rectangle
replacement, edge edits, and fixture removal.
PhysicsData geometry probes multi-body shape replacement and forwarded sensor,
filter, speed, and rotation-response settings.
PhysicsData command probe samples thrust, reverse thrust, braking, and rotation
commands over several physics updates.
PhysicsData settings probe checks forwarded pose, shape, filter, material, and
movement settings by reading the resulting body state.
PhysicsData dynamics probe follows velocity and rotation decay, speed limits,
and sleep controls over several updates.
Math probe covers script math functions, geometry helpers, and vector methods
with boundary and wraparound inputs.
Math edge probe adds vector operators, zero-length inputs, circle tangencies,
and range boundaries.
PhysicsBody surface probe covers null/no-result accessors, mutable direction,
mass rebuild, fixture edits/removal, continuous physics, and sleep state.
Entity lifecycle checks enable/disable, contact callbacks, re-enable, and
delete/flush while bodies overlap. Entity manager probes cover pending versus
committed entities, iterator snapshots, live-iterator deletion/appends,
type-filter behavior, repeated tags, observer-queued deletion and creation,
and clear-all behavior. The entity state probe exercises exposed
state, identity, geometry, alarms, particles, compatibility stubs, and perf
cost calls. Its cases without retail captures remain outside the parity score.
Entity callback probe checks enable and disable delegation separately from
the enabled flag, including repeated and missing behavior callbacks.
Entity GC probe checks entity/body retention through collection, detached-body
safety after deletion, and ID invalidation. Heap reclamation itself is not
observable from GameMonkeyScript. Retail faults on the retained-body
`isSleeping()` call after owner deletion; OpenEgg logs a warning at that call
and continues safely. Later readings run only on OpenEgg.
GC-cycle probe checks body positions through repeated collection, single
deletion, recreation, and clear-all.
GC-iterator probe checks iterator-held entities through collection, removal,
and reuse after collection.
Process probes compare creation, deferred init, queued termination, pause
state, callback order, and registry counts with retail.

Advanced lifecycle probes flush a newly created entity inside a contact
callback, stress collection with 96 entities, replace a process list while it
owns an entity, and combine simulation-speed changes with pause dispatch. Run
`python3 tests/physics/compare.py` for paired headless checks.

Retail faults at `0x97420` when `CollisionInfo.getNumCollidingShapes()` is
called on an active pair in this probe. That call is excluded from the paired
score. The fault came from a separate headless RPCS3 run of the same pair.
