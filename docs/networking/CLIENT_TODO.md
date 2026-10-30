# OpenEgg networking worklist

Early implementation. Many request paths have local fixture coverage, but those
tests do not establish full retail parity.

The local Ghidra worklist reports 202 of 204 research items checked (99%).
That is checklist completion, not semantic coverage: the 127 API entries have
different trace depth, and several traces stop at wrapper and request-building
behavior.

## Current

- [ ] Add API notes for the newly registered networking functions; missing
      entries currently prevent the API documentation site from building.
- [x] Construct the patched PS3 community screen when a server is configured;
      retain the fallback when offline or the patch helper is unavailable.
- [x] Initialize the PS3 community screen with retail menu helpers in a
      headless probe.
- [x] Run retail greatest-hits, friends, albums, daily and activity tab
      requests and callbacks against a local server with a fixture account;
      the tested responses were empty.
- [x] Create greatest-hits and activity panels from a synthetic server level;
      seed it with the server's `OpenEggCommunityFixture` tool and require it
      with `--expect-populated` in the community probe.
- [x] Create a daily panel from the same synthetic fixture and verify the
      retail daily callback (`--expect-daily`).
- [x] Create an online album panel from the synthetic fixture (`--expect-album`),
      load its thumbnail, and check its incomplete level callback
      (`--expect-album-details`).
- [x] Submit a completion record and refresh the retail album callback; the
      fixture level changes from incomplete to complete (`--expect-album-complete`).
- [x] Fetch the synthetic level's thumbnail and verify the retail callback
      assigns a valid panel texture (`--expect-thumbnail`).
- [x] Retain and clear a quarter-size prepared screenshot using adjusted
      viewport dimensions. Compare source stride, padding, viewport origin,
      channel order and capture timing with retail.
- [x] Download the synthetic level using the panel's retail version ID and
      check its decoded BPM (`--expect-level-data`).
- [x] Accept extra arguments on the traced level, sound, thumbnail, and
      description wrappers; headless fixtures check each request.
- [x] Queue the panel's level, read it back, remove it, and confirm a later
      lookup reports it missing (`--expect-queue`).
- [x] Like, read, unlike, and recheck the panel's level against the local
      server (`--expect-heart`). Its separate rating-delete request gets 404.
- [x] Accept the patched scripts' six-argument activity call. The traced PS3
      platform helper does not post anything. The desktop also retains a
      two-argument fallback for older scripts; its retail behavior is unknown.
- [x] Return the PS3 browser stub's `-4` result for launch and finish.
- [x] Update the supplied Near gift table with the traced field names. Empty
      values are a desktop fallback; retail's unwritten buffer contents are unknown.
- [x] Return the traced integer result from the store launch wrapper; desktop
      still has no Commerce backend.
- [x] Run the retail screen's draw and update callbacks with a minimal menu
      parent; verify its automatic daily request and tab switch (`--navigate`).
- [x] Forward headless key events through a minimal parent to the retail PS3
      input handlers and select the greatest-hits tab (`--harness-input`).
- [x] Start the full retail menu with a disposable save, connect to a local
      fixture account, and verify that the community process attaches.
- [x] Dismiss first-run popups with headless key events and select greatest
      hits through the full menu, receiving a response from the local server
      (`run_full_menu_probe.py --input`). Tutorial and EULA are bypassed in
      the disposable test save.
- [ ] Exercise the full menu process with controller input and compare rendered output.
- [ ] Test other tab callbacks and panels with populated server responses.
- [x] Accept extra asset-download arguments, including the seventh argument
      used by patched scripts; the retail wrapper ignores them.
- [x] Async HTTP worker; callbacks return on the GameMonkey tick thread.
- [x] JSON, raw-text, and large-integer callback conversion paths.
- [x] Configurable server URL, session ID, and user ID.
- [x] Start the coordinator with a configured session after a server check.
      The replacement server checks the supplied identity against the session
      on `~identity:*.hello`; ordinary retail hello requests remain unchanged.
- [x] Sign in to a registered replacement-server account using INI credentials;
      this desktop exchange is separate from retail PSN ticket login.
- [x] Match `GoOffline`'s state-2 result when no identity manager is attached;
      the headless manual-session probe checks the state before reconnecting.
- [x] Hold state 2 while attached identity requests drain, then resume a
      deferred `GoOnline`; a headless probe checks cancellation and reconnect.
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
- [x] Gate leaderboard requests on identity setup; test request paths, score
      bodies, and large-integer conversion with a local fixture.
- [x] Gate album page requests on identity setup; test both page paths and
      empty album IDs with a local fixture.
- [x] Gate index pages on identity setup; test typed and wildcard queries,
      authenticated server access and in-flight purge.
- [x] Match the traced first-match and missing-equals edge cases when selecting
      an index type from query text.
- [x] Gate activity-stream requests on identity setup; test page and empty-body
      post request construction with a local fixture.
- [x] Gate following and heart requests on identity setup. Test their paths,
      self-follow check and page identity fallback with a local fixture.
- [x] Gate queued-item requests on identity setup; test all four routes and
      confirm a null callback succeeds without sending a request.
- [x] Match `FUN_001d45e8` behavior across its request wrappers: a null
      callback reports acceptance without queuing HTTP. Fixture counts cover
      index, metadata, rating, leaderboard, following and level lookups.
- [x] Keep specialized level-content and heart helpers on their own path.
      Retail helpers `0x001d48e8`–`0x001d59b4` dispatch through `0x001821cc`
      without the generic null-callback guard; headless probes check level,
      sound, description, thumbnail and heart requests with null callbacks.
- [x] Gate record submissions on identity setup; test serialized fields,
      empty records and the traced header value with a local fixture.
- [x] Preserve the HTTP status when a response body fails to arrive; reject
      partial data from the content cache and test a subsequent fetch.
- [x] Keep non-success HTTP status results cacheable when their bodies are
      interrupted; the retail worker does not read those bodies.
- [x] Gate snapshot, version and unpublish requests on identity setup; test
      their routes, null-callback behavior and publish/update/cleanup cycle.
- [x] Convert JSON arrays to integer-keyed tables without adding a `size` key;
      retail scripts count their entries with `tableCount`.
- [x] Return an empty result table for scalar JSON roots, and preserve parsed
      fields on malformed level-description responses.
- [x] Cache the default image, level and sound content paths and support
      identity-cache filter purge. Local tests cover hits, errors and purge.
- [x] Match the traced slash boundary for single and double wildcard cache
      filters; a headless probe checks nested keys, a single component match,
      and the trailing wildcard case.
- [x] Apply the traced content rules to global image keys and keep the global
      cache separate from identity-cache filter purges.
- [x] Admit cache hits ahead of the worker queue and consume them after pending
      worker completions, following the traced manager paths.
- [x] Share the four-completion update budget across identity and global
      requests; exercise both queues in the headless budget probe.
- [x] Limit each worker completion ring to 32 entries; traced worker ignores
      insertion failure. Headless backlog probe checks paced callback delivery.
- [ ] Reproduce a completion-ring overflow and its lost-request lifetime;
      worker may outpace manager updates, which standard headless probes do not.
- [x] Stage requests before worker admission. Retail `HttpManagerUpdate`
      admits up to four per update; a full 32-entry ring calls back with no
      HTTP status, which the retail thunk reports as error `1`. A headless
      saturation probe checks asynchronous rejection and cached/global work.
- [x] Follow the traced cache replacement order and zero-length first-entry
      eviction behavior; the latter has a local pool-pressure test.
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
      an in-flight response can retain its HTTP status, as in the PS3 trace.
- [x] Compare raw bucket payloads for purge, including float and string values.
      Object references use their low 32 bits on desktop, as a PS3 approximation.
- [x] Cancel identity requests on logout and login transitions while allowing
      independent global image requests to complete.
- [x] Start experimental ticket-file exchange from `OnlineCoordinator_GoOnline`.
- [x] Send the identity hello and region-specific PS3 EULA request after login; cache
      EULA text and digest and poll script acceptance.
- [x] Synchronize an empty desktop platform friend list and expose returned
      friend identities. A real platform friend provider is still needed.
- [x] Use the configured publisher region and retail locale mapping for EULA,
      translation, and localized community requests. Desktop language stands
      in for the PSN account region.
- [x] Fetch and refresh featured and news data into rooted script tables.
- [x] Read the local beta agreement file. A missing file returns null; retail's
      traced missing-file path dereferences a null pointer.
- [x] Parse little-endian network MO catalogs and reach coordinator online
      state after all three localized callbacks have run.
- [x] Match the traced duplicate-translation prefix comparison in the network
      catalog, including empty incoming values.
- [x] Cloud-save GET and multipart POST; tested with a disposable local server
      account and a byte-checking fixture. The test also posts the retail
      script's `onlineID`-only delete payload and verifies a subsequent 404.
- [x] Merge nested cloud-save tables in place and compare integer values without
      losing precision; the headless probe covers mixed integer/float values.
      Other type combinations and cyclic tables still need retail comparison.
- [x] Run the retail cloud-sync script with synthetic local and remote saves;
      check the merged save on a local server and in OpenEgg's local save file.
      The save scheduler and unlock handling are simulated (`run_cloud_sync_probe.py`).
- [x] Check the script's mismatched-ID and server-error paths do not write a
      local save; the mismatched-ID case also leaves the server save intact.
- [x] Sign in with a disposable account and run first-time cloud sync with no
      server save; verify the new local and remote saves (`--case initial`).
- [x] Run the retail cloud-delete script with a seeded server save; verify its
      removal and the resulting local save (`--case delete`).
- [ ] Audit other `RetailFctiwz` helpers: several return `INT_MIN` for positive
      overflow, unlike the PowerPC instruction's `INT_MAX` result.
- [x] Fetch and inflate level data as XML or JSON, with request-state callback
      status; exercised against a synthetic replacement-server level.
- [x] Fetch level thumbnails and content images into script texture handles;
      exercised against a local fixture and a synthetic server thumbnail.
- [x] Use a separate configurable image base URL and omit the identity session
      header on global content-image requests.
- [x] Follow HTTP redirects for global image requests. Identity-request
      redirects follow root-relative locations only, keeping the session header
      on the same origin. Other identity redirects are rejected until retail's
      redirect callback and header policy are understood.
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
- [ ] Match PSN account-region locale selection, retail update timing, and
      malformed EULA edge cases. Desktop language remains a proxy.
- [ ] Match retail login reset, request purging, malformed responses and error
      transitions. Test genuine platform tickets against a suitable server.
- [ ] Match the manager thread's exact stop/restart timing and native PSN
      startup states. Desktop treats an idle identity worker as transport-ready;
      ticket login tests use synthetic tickets, not PSN sign-in.

## Request core

- [ ] Match retail URL building: `/otg` suffix, operation suffix, absolute
      URLs, separators, query placement, and byte limits.
- [ ] Match retail HTTP verb selection, body ownership, content-type handling,
      and the traced record-post header quirk where required for parity.
- [x] Stop reading non-success response bodies after headers; test a stalled
      404 response without blocking the following request.
- [ ] Match request buckets, purge/cancel behavior, FIFO ordering, retry
      admission, stale status handling, and shutdown draining.
- [ ] Compare cache enable flags, wildcard edges, capacity and pool eviction
      against retail. The default rule state is inferred from the PS3 trace.
- [x] Bound identity and global worker queues to 32 pending requests each;
      stage new requests and admit up to four per update (see request-core item).
- [ ] Verify request manager update ordering and shutdown draining against
      retail; identity and global image requests use separate desktop workers.
- [ ] Verify the lifetime of requests whose completion ring insertion fails.
      OpenEgg retains their callback roots until shutdown.
- [ ] Match callback status and JSON modes for malformed JSON, empty bodies,
      HTTP errors, and transport errors.
- [ ] Compare partial JSON tables and array handling with retail for malformed
      tokens, truncated strings, nesting, numeric limits and trailing bytes.
      OpenEgg retains values parsed before an error, but has no retail capture.
- [ ] Compare malformed multibyte `Watermelon_BaseKeyValue` inputs with retail;
      normal separator and bounded multibyte cases follow the static trace.
- [ ] Compare `Watermelon_UrlEncode`'s native escape table and too-small-buffer
      result. Ghidra shows a zeroed 512-byte buffer passed through an adapter
      to `cellHttpUtilEscapeUri`; its output length sizes the returned string.
      The system-library escape and failure behavior remain unknown.
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
- [x] Run the retail offline-score callback against successful and rejected
      records; check dirty flags, leaderboard page and current-user near
      readback (`run_offline_record_probe.py`).
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
- [x] Use bundled Russian and CJK font resources for the traced language-font
      setup and clear calls. Desktop uses glyph fallback; retail's resource
      cache and paired font lists remain different.
- [x] Match retail's base-0 DLC pack ID parsing in the desktop debug fallback,
      including hexadecimal, octal, sign, and trailing-text cases. PSN
      entitlement checks still require a platform provider.
- [ ] Match online/offline state and error transitions exposed to scripts.
- [x] Match the traced `Network_IsSignedIn` (`>2`) and `Network_IsOnline`
      (`>3`) tests. Retail initializes the shared state to 0, writes 4 on
      online status/event, and writes 2 on disconnect. No writer of state 3
      was found; desktop maps an authenticated replacement-server session to 4.

## Verification

- [ ] Establish a reproducible authenticated test account and session for the
      local replacement server.
- [ ] Capture comparable request/response logs from retail and OpenEgg for each
      implemented family; compare method, URL, query, headers, body, status,
      callback data, and callback order.
- [ ] Keep server rejection, missing test data, unimplemented client behavior,
      and confirmed parity failures as distinct outcomes in reports.
- [ ] Run the network probe headlessly after each client networking change.
