# OpenEgg networking worklist

Early implementation. Only a small read path has been exercised against the
replacement server; this list is not a parity claim.

The local Ghidra worklist reports 202 of 204 research items checked (99%).
That is checklist completion, not semantic coverage: the 127 API entries have
different trace depth, and several traces stop at wrapper and request-building
behavior.

## Current

- [x] Async HTTP worker; callbacks return on the GameMonkey tick thread.
- [x] JSON, raw-text, and large-integer callback conversion paths.
- [x] Configurable server URL, session ID, and user ID.
- [ ] Add account login; the INI session and user IDs are temporary test inputs,
      not username/password credentials or a complete online flow.
- [x] `Watermelon_Index_Page`, album pages, user-list pages,
      `Watermelon_ActivityStream_Page`, and `Watermelon_FeaturedLevel_Get`.
- [x] Following get/put/delete route construction and self-follow short
      circuit.
- [x] Queued page/get/put/delete route construction.
- [x] Metadata all/get/put request construction and callback modes.
- [x] Explicit POST transport; local invalid-ticket request reaches the
      replacement server and returns HTTP 400.
- [x] Authenticated index request from OpenEgg using a locally seeded test session.
- [x] Experimental ticket-file exchange and authenticated request against a
      local replacement server configured to accept unsigned tickets.
- [x] Purging a queued or in-flight request produces a cancelled callback;
      the in-flight transport may still finish, as in the PS3 trace.
- [x] Start experimental ticket-file exchange from `OnlineCoordinator_GoOnline`.
- [x] Send the identity hello and PS3 SCEA EULA requests after login; cache
      EULA text and digest and poll script acceptance.
- [x] Synchronize an empty desktop platform friend list and expose returned
      friend identities. A real platform friend provider is still needed.
- [x] Fetch and refresh featured and news data into rooted script tables.
- [x] Parse little-endian network MO catalogs and reach coordinator online
      state after all three localized callbacks have run.
- [x] Cloud-save GET and multipart POST; tested with a disposable local server
      account and a byte-checking fixture.
- [x] Fetch and inflate level data as XML or JSON, with request-state callback
      status; exercised against a synthetic replacement-server level.
- [x] Fetch level thumbnails and content images into script texture handles;
      exercised against a local fixture and a synthetic server thumbnail.
- [x] Publish and update a synthetic level using saved resource tickets;
      read it back and remove it on a disposable local server.
- [ ] Compare publish failures, multipart boundaries and ticket ownership
      against retail. Desktop tickets currently reload their named files.
- [ ] Compare level XML/JSON conversion and malformed input with retail;
      desktop inflate currently caps output at 64 MiB.
- [ ] Connect the separate network translation map to traced native string
      consumers; compare malformed catalogs and platform sign-in timing.
- [ ] Replace the configurable desktop chat-permission fallback with platform
      restriction data where a platform provider is available.
- [ ] Replace the ticket-derived online ID with a platform provider when one
      exists; this desktop fallback does not query PSN sign-in state.
- [ ] Implement other product regions and later coordinator phases. Match
      retail update timing and malformed EULA edge cases.
- [ ] Match retail login reset, request purging, malformed responses and error
      transitions. Test genuine platform tickets against a suitable server.
- [ ] Implement and verify the retail ticket login and coordinator state flow.

## Request core

- [ ] Match retail URL building: `/otg` suffix, operation suffix, absolute
      URLs, separators, query placement, and byte limits.
- [ ] Match retail HTTP verb selection, body ownership, content-type handling,
      and the traced record-post header quirk where required for parity.
- [ ] Match request buckets, purge/cancel behavior, FIFO ordering, retry
      admission, stale status handling, and shutdown draining.
- [ ] Match callback status and JSON modes for malformed JSON, empty bodies,
      HTTP errors, and transport errors.
- [ ] Compare malformed multibyte `Watermelon_BaseKeyValue` inputs with retail;
      normal separator and bounded multibyte cases follow the static trace.
- [ ] Add native-comparison probes for request ordering and callback timing.

## Watermelon APIs

- [ ] Finish level description, sound and save-to-disk paths; compare image
      cache ownership and malformed image behavior with retail.
- [ ] Implement remaining ratings, favourites, and hearts, including
      their retail routes and methods.
- [ ] Verify record and leaderboard serialization, callbacks, and authenticated
      server results. Record submission currently rejects unsupported values and
      oversized tables instead of reproducing the retail buffer edge cases.
- [x] Submit a fixture record and read back a large leaderboard score through
      the local server. Compare the same case with retail before claiming parity.
- [ ] Implement grief reports; verify cloud-save edge cases
      and multipart behavior against retail.
- [ ] Implement friend, identity, EULA, and online coordinator flows.
- [ ] Review every currently offline request binding against the tracked
      `Watermelon` and `Network` symbol entries in
      `src/core/retail_symbols.inc` and its corresponding PS3 trace.

## Platform Network APIs

- [ ] Inventory which of the 45 `Network_*` calls have meaningful desktop
      equivalents and keep Sony-only APIs explicitly unsupported where needed.
- [ ] Implement and probe the applicable account, trophy, store, DLC, language,
      SNS, activity, and Near behavior.
- [ ] Match online/offline state and error transitions exposed to scripts.

## Verification

- [ ] Establish a reproducible authenticated test account and session for the
      local replacement server.
- [ ] Capture comparable request/response logs from retail and OpenEgg for each
      implemented family; compare method, URL, query, headers, body, status,
      callback data, and callback order.
- [ ] Keep server rejection, missing test data, unimplemented client behavior,
      and confirmed parity failures as distinct outcomes in reports.
- [ ] Run the network probe headlessly after each client networking change.
