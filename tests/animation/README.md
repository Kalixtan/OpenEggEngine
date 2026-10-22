# Animation probes

Run local playback and edge checks with extracted game data:

```sh
python3 tests/animation/run.py --game-data /path/to/game_data
python3 tests/animation/extended.py --game-data /path/to/game_data
```

The probes copy only the needed files to temporary folders. Results go to
`reports/ANIMATION.md` and `reports/ANIMATION_EXTENDED.md`.

Refresh the paired retail readings with RPCS3:

```sh
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/USRDIR/EBOOT.BIN --probe tests/animation/record_boundaries.gm \
  --baseline tests/animation/records-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/USRDIR/EBOOT.BIN --probe tests/animation/entity_clock.gm \
  --support-file tests/animation/AnimationProbeEntity.gm \
  --baseline tests/animation/entity-clock-retail.tsv
python3 tests/capture_retail_probe.py --main /path/to/game_data/Main.gm \
  --eboot /path/to/USRDIR/EBOOT.BIN --probe tests/animation/resource_lifetime.gm \
  --support-file tests/animation/AnimationProbeEntity.gm \
  --baseline tests/animation/resource-lifetime-retail.tsv
python3 tests/animation/capture_negative_frame.py \
  --main /path/to/game_data/Main.gm --eboot /path/to/USRDIR/EBOOT.BIN
```

The negative-index capture is isolated because retail faults there. See
`retail-crashes.md` for the observed failure and OpenEgg warning. A failed or
incomplete capture does not count as a parity mismatch.

The resource-lifetime probe keeps an entity animation playing while unloading
module and entity-owned references, then deletes the entity and reloads the
resource.
