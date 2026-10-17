# Compatibility status

> **Early development.** OpenEgg is not close to full retail compatibility.

## API reference

| Measure | Current |
| --- | ---: |
| Registered script functions and methods | 1,494 |
| Individual reference pages | 1,494 / 1,494 (100%) |
| Binding categories | 34 |
| Entries with unresolved parameter placeholders | 23 |
| OpenEgg-specific type pages | 7 |
| Recovered retail type names | 52 |
| Known recovered retail methods not implemented | 7 |

The API generator checks that every local registration has one reference row,
and rejects duplicate or stale rows, incomplete descriptions, and malformed
parameter records. The retail type and missing-method inventories are
incomplete.

## Implementation and parity

Project-wide implementation and retail-match percentages are not measured.
A registered name may be a stub or partial implementation. Each API page
confirms registration, while runtime behavior and full retail matching remain
unverified unless a note on that page records narrower source evidence.

Reference coverage measures documentation only; it does not measure how much
of the game runs or how closely OpenEgg matches retail behavior. See
[TODO.md](TODO.md) for remaining work.
