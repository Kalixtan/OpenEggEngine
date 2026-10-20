# Sequencer smoke check

Run from the repo root with your own game data available to OpenEgg:

```sh
GE3_HEADLESS=1 GE3_TRACE=1 GE3_HARNESS="$PWD/tests/sequencer/smoke.harness" \
SDL_AUDIODRIVER=dummy LIBGL_ALWAYS_SOFTWARE=1 \
./build/OpenEgg "$PWD/tests/sequencer" smoke.gm Main
```

Expect `SEQ conversion PASS`, `SEQ order PASS`, `SEQ removal PASS`, and `SEQ timing PASS`.

Run the same command with `edge.gm` to check OpenEgg's safe handling of two
retail edge cases. Retail faults when moving the last trigger, and a cursor
held through `clearTriggers()` reads freed memory. OpenEgg returns normally
and invalidates the cleared cursor.

Run it with `state.gm` to check constructor state, tempo, note divisions,
navigation, and playback flags. These checks also passed against the retail
PS3 build.

Run it with `global.gm` to check the live sequencer list and global pause.
The same cases passed against retail.

Run it with `timer.gm` to check the BPM keep/reset flag and pattern-length
timer reset during a callback. These also passed against retail.

Run it with `loop.gm` to check repeating triggers across two patterns.
Both callback positions and repeats passed against retail.
