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
- [x] Start the coordinator with a configured session after a server check.
      The check does not prove that the supplied identity owns the session.
- [x] Sign in to a registered replacement-server account using INI credentials;
      this desktop exchange is separate from retail PSN ticket login.
- [x] `Watermelon_Index_Page`, album pages, user-list pages,
      `Watermelon_ActivityStream_Page`, and `Watermelon_FeaturedLevel_Get`.
- [x] Following get/put/delete route construction and self-follow short
      circuit.
- [x] Gate rating and favourite requests on identity setup; check their
      request paths, callback responses and rating body with a local fixture.
- [x] Gate featured-level requests on identity setup; exercise signed-in
      publication and retrieval against the local replacement server.
- [x] Gate metadata requests on identity setup and test raw-text and JSON
      callbacks, including the empty-value GET with its content-type header.
- [x] Match traced queue key construction: `get` builds a level key, while
      `put` and `delete` use the supplied key directly.
- [x] Queue, read and remove a synthetic level through the local server.
- [x] Queued page/get/put/delete route construction.
- [x] Metadata all/get/put request construction and callback modes.
- [x] Explicit POST transport; local invalid-ticket request reaches the
      replacement server and returns HTTP 400.
- [x] Authenticated index request from OpenEgg using a locally seeded test session.
- [x] Experimental ticket-file exchange and authenticated request against a
      local replacement server configured to accept unsigned tickets.
- [x] Purging a queued or in-flight request produces a cancelled callback;
      the in-flight transport may still finish, as in the PS3 trace.
- [x] Cancel identity requests on logout and login transitions while allowing
      independent global image requests to complete.
- [x] Start experimental ticket-file exchange from `OnlineCoordinator_GoOnline`.
- [x] Send the identity hello and PS3 SCEA EULA requests after login; cache
      EULA text and digest and poll script acceptance.
- [x] Synchronize an empty desktop platform friend list and expose returned
      friend identities. A real platform friend provider is still needed.
- [x] Fetch and refresh featured and news data into rooted script tables.
- [x] Read the local beta agreement file. A missing file returns null; retail's
      traced missing-file path dereferences a null pointer.
- [x] Parse little-endian network MO catalogs and reach coordinator online
      state after all three localized callbacks have run.
- [x] Match the traced duplicate-translation prefix comparison in the network
      catalog, including empty incoming values.
- [x] Cloud-save GET and multipart POST; tested with a disposable local server
      account and a byte-checking fixture.
- [x] Fetch and inflate level data as XML or JSON, with request-state callback
      status; exercised against a synthetic replacement-server level.
- [x] Fetch level thumbnails and content images into script texture handles;
      exercised against a local fixture and a synthetic server thumbnail.
- [x] Use a separate configurable image base URL and omit the identity session
      header on global content-image requests.
- [x] Follow HTTP redirects for global image requests. Identity-request
      redirect and header behavior still needs comparison with retail.
- [x] Load a featured image URL returned by a disposable replacement server.
- [x] Publish and update a synthetic level using saved resource tickets;
      read it back and remove it on a disposable local server.
- [x] Read the snapshot upload ID before updating, as the retail script does.
- [x] Set and read a featured level through the local replacement server.
- [x] Publish, read back, update and remove a user-supplied retail JSON level
      on a disposable server; the level is not included in the repository.
- [x] Send a report for a synthetic level on a disposable local server.
- [ ] Compare account country, language and console-ID reporting with retail;
      desktop currently uses the supplied ticket and configured language.
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
- [ ] Replace the platform-state fallback with a real platform provider. A
      successful desktop server login currently stands in for platform state;
      it does not establish a PSN sign-in or retail restriction status.
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
- [x] Bound identity and global worker queues to 32 pending requests each;
      waiting admission follows the retail enqueue retry path.
- [ ] Verify request manager update ordering and shutdown draining against
      retail; identity and global image requests use separate desktop workers.
- [ ] Match callback status and JSON modes for malformed JSON, empty bodies,
      HTTP errors, and transport errors.
- [ ] Compare malformed multibyte `Watermelon_BaseKeyValue` inputs with retail;
      normal separator and bounded multibyte cases follow the static trace.
- [ ] Add native-comparison probes for request ordering and callback timing.

## Watermelon APIs

- [x] Download and stage level, sound and image files; test saving against a
      local replacement server.
- [ ] Compare asset staging and save persistence with retail. Flushing staged
      files through `SAVEDATA_SaveAll` is currently a desktop adaptation.
- [x] Implement level-description callbacks and test success, malformed JSON
      and HTTP failure against a local fixture.
- [x] Publish a level and fetch its upload description from the local server.
- [x] Fetch a sound resource and load supported compressed audio scripts;
      exercise success and failure callbacks against a local fixture.
- [ ] Verify the description URL and response against retail. Expand audio
      script parsing to cover unsupported callback classes and compare network
      playback and image cache ownership with retail.
- [ ] Implement remaining ratings, favourites, and hearts, including
      their retail routes and methods.
- [ ] Verify record and leaderboard serialization, callbacks, and authenticated
      server results. Record submission currently rejects unsupported values and
      oversized tables instead of reproducing the retail buffer edge cases.
- [x] Submit a fixture record and read back a large leaderboard score through
      the local server. Compare the same case with retail before claiming parity.
- [ ] Verify cloud-save edge cases and multipart behavior against retail.
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
