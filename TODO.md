# OpenEgg development plan

Open work is grouped by engine area. Retail compatibility remains incomplete.

## API documentation and evidence

- [x] Keep one reference row for each registered script function and method.
- [x] Check names, required fields, and parameter records against the current
      bindings when generating the API site.
- [ ] Record implementation state separately from runtime verification and
      retail parity for each entry.
- [ ] Audit every entry's behavior, arguments, return value, and custom types
      against the binding source and available retail evidence.
- [ ] Recalculate [COMPATIBILITY_STATUS.md](COMPATIBILITY_STATUS.md) from
      per-entry evidence before reporting implementation percentages.
- [ ] Expand the recovered retail type and missing-method inventories.

## Runtime and physics

- [ ] Match frame timing, pause handling, update order, and physics-step inputs.
- [x] Compare process creation, queued termination, pause/resume timing, and
      callback order with retail probes.
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

- [ ] Recheck `Entity.callMethod` and `Entity.getWhichSide` against retail.
      Current capture is inconclusive: headless RPCS3 produced no TTY output.
- [x] Add headless probes for FSA loading, playback state, seeking, custom
      sequences, shared resources, and unload. All 39 readings match retail;
      missing-resource handling remains an intentional safety difference.
- [x] Check PS3 FSA record bounds, negative frame indices, and entity animation
      stop/resume. 12 readings match retail; negative indices fault retail and
      are rejected with a warning in OpenEgg.
- [ ] Verify load/unload order, callbacks, object retention, and cleanup during
      scene transitions.
- [ ] Compare entity iterators, filters, and mutation during iteration.
      Removal, append, type-filter, and empty-iterator behavior now match in
      10 readings; remaining filters and mutation cases need coverage.
- [ ] Verify native object identity and garbage collection. Active-entity
      and owned-body retention match retail. RPCS3 faults after deletion when
      calling `PhysicsBody.isSleeping()` on retained body; direct object
      reclamation remains unobserved. Repeated body retention, deletion,
      recreation, and clear-all match in 14 readings.

## Audio and sequencing

- [x] Convert the 48 kHz mixer stream to the SDL device format, prebuffer two
      callback periods, and clear audio backlog above 250 ms. Headless dummy-
      device startup passed; retail mixer and DSP parity remain open.
- [x] Return retail's integer `-1` for unsupported load, play, and Slicker
      audio-call arities; middleware behavior remains open.
- [ ] Compare voice allocation, priority, routing, pan, pitch, and timing.
- [ ] Implement and compare effect-chain behavior and streamed audio paths.
- [ ] Verify sequencer triggers, event ordering, removal, and timer boundaries.
- [ ] Compare playback and recording behavior with repeatable input and output
      captures.

## Input, files, and platform behavior

- [ ] Compare the SDL text-entry path for `Input.openIME`, `getIMEStatus`, and
      `getStringIME` against retail callbacks; title/keyboard-mask behavior and
      completion/cancel status parity remain unverified.
- [ ] Compare controller selection, axis values, button mapping, and rumble.
- [ ] Verify keyboard and controller behavior through menus and gameplay.
- [ ] Check file-area resolution, asynchronous operations, save requests, and
      shutdown behavior.
- [ ] Document unsupported online services and decide which are in scope for
      native offline play.
- [ ] Run the same build and smoke scenarios on each supported desktop OS.
