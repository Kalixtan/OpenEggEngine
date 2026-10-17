# OpenEgg development plan

This list distills the highest-priority open work from the project's earlier
reverse-engineering notes. It contains task summaries only; no retail scripts,
assets, or extracted implementation code are included.

## First: make progress measurable

- [ ] Add a local API inventory with one row per registered native function.
- [ ] Track implementation state separately from parity state: `missing`,
      `stub`, `implemented`, `partially verified`, and `matched`.
- [ ] Record the behavior under review, the evidence used, the validation run,
      known differences, and the next step for each row.
- [ ] Reconcile the provisional API inventory with registration tables before
      treating percentages as project-wide totals.
- [ ] Recalculate [COMPATIBILITY_STATUS.md](COMPATIBILITY_STATUS.md) from that
      inventory whenever its data is updated.

## Runtime and physics

- [ ] Match frame timing, pause handling, update order, and physics-step inputs.
- [ ] Compare contact creation, callback order, deferred events, and body
      destruction against observed game behavior.
- [ ] Verify contact queries, shape membership, collision filtering, and
      manifold edge cases.
- [ ] Test rolling, slope movement, and other player movement modes with
      repeatable input captures.
- [ ] Verify entity and process creation, update, and destruction order.

## Rendering and interface

- [ ] Compare draw-list command production and consumption, including state
      stacks, blend, alpha, depth, stencil, and draw order.
- [ ] Match viewport, projection, and matrix state on representative scenes.
- [ ] Compare loading screens, menus, and gameplay frames against captures
      made from a legally obtained retail copy.
- [ ] Verify texture formats, palette handling, animation resources, and
      texture lifetime.
- [ ] Check font metrics, glyph coverage, alignment, and text substitutions.

## Animation and entities

- [ ] Exercise multi-record animation resources, custom sequences, and boundary
      frame values.
- [ ] Verify load/unload order, callbacks, object retention, and cleanup during
      scene transitions.
- [ ] Compare entity iterators, filters, and mutation during iteration.
- [ ] Verify native object identity and garbage-collection behavior.

## Audio and sequencing

- [ ] Compare voice allocation, priority, routing, pan, pitch, and timing.
- [ ] Implement and compare effect-chain behavior and streamed audio paths.
- [ ] Verify sequencer triggers, event ordering, removal, and timer boundaries.
- [ ] Compare playback and recording behavior with repeatable input and output
      captures.

## Input, files, and platform behavior

- [ ] Compare controller selection, axis values, button mapping, and rumble.
- [ ] Verify keyboard and controller behavior through menus and gameplay.
- [ ] Check file-area resolution, asynchronous operations, save requests, and
      shutdown behavior.
- [ ] Document unsupported online services and decide which are in scope for
      native offline play.
- [ ] Run the same build and smoke scenarios on each supported desktop OS.
