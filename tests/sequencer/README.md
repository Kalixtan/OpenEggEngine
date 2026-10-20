# Sequencer smoke check

Run from the repo root with your own game data available to OpenEgg:

```sh
GE3_HEADLESS=1 GE3_TRACE=1 GE3_HARNESS="$PWD/tests/sequencer/smoke.harness" \
SDL_AUDIODRIVER=dummy LIBGL_ALWAYS_SOFTWARE=1 \
./build/OpenEgg "$PWD/tests/sequencer" smoke.gm Main
```

Expect `SEQ conversion PASS`, `SEQ order PASS`, and `SEQ removal PASS`.
