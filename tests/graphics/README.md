# Shape API tests

These probes check retained `SHAPE` objects and the immediate `Video` shape API.

- `shape_api_ok` exercises the retail three-argument `SHAPE` factory, `VMSHAPE.plotVertex`/`endShape`, and `Video.beginShape`/`plotVertex`/`endShape`.
- `shape_api_bad_arity` omits `SHAPE`'s third argument. Retail produced a null shape and failed when the script used it; OpenEgg reports the invalid factory call directly.
- `shape_api_bad_arity_zero` and `_one` cover the other too-few-arguments cases.
- `shape_api_bad_arity_extra` supplies an extra fourth argument.
- `shape_api_bad_type` supplies a non-table third argument.
- `shape_api_bad_video_vertex_count` calls `Video.plotVertex` with six arguments; the recovered retail wrapper rejects counts outside 1–5.
- `shape_api_bad_video_vertex_zero` checks the rejected zero-argument form.
- `shape_api_bad_video_vertex_selector_low` and `_high` exercise the invalid endpoints of the two-argument selector, which retail accepts only from 1 through 3.
- `shape_api_bad_video_begin_arity` passes four arguments to `Video.beginShape`; the recovered wrapper accepts one through three.
- `shape_api_bad_video_begin_zero` checks the rejected zero-argument form.
- `shape_api_video_vertex_forms` checks the accepted 1–5 argument forms, including the 2-argument selector range.

Run the OpenEgg checks from the repository root:

```sh
python3 tests/graphics/run_shape_api_tests.py
```

For a retail check, temporarily use each folder's `Main.gm` as the retail entry script. Malformed cases should stop at the corresponding script error; accepted forms should reach their pass marker.
