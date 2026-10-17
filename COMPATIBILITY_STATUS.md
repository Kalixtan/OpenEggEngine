# Compatibility status

> **OpenEgg is very WIP!** 

OpenEgg aims to reproduce the observable behavior of the GameEngine3 runtime
used by *Sound Shapes*. These figures describe a provisional inventory of
1,469 native API wrapper records from the project's earlier audit. The raw
retail analysis is not included in this repository, and these totals have not
been revalidated against every function in this OpenEgg revision.

## Current measurable status

| Subsystem | API records | Implementation coverage | Evidence checkpointed | Complete matches | Out of scope |
| --- | ---: | ---: | ---: | ---: |
| Audio and sequencing | 194 | Unmeasured | 103 / 194 (53.1%) | 29 / 194 (14.9%) | 0 |
| Physics | 273 | Unmeasured | 42 / 273 (15.4%) | 1 / 273 (0.4%) | 0 |
| Rendering and graphics | 232 | Unmeasured | 22 / 232 (9.5%) | 8 / 232 (3.4%) | 0 |
| Entities and process runtime | 341 | Unmeasured | 93 / 341 (27.3%) | 4 / 341 (1.2%) | 0 |
| Input and platform | 64 | Unmeasured | 25 / 64 (39.1%) | 1 / 64 (1.6%) | 12 |
| Math and utility | 96 | Unmeasured | 9 / 96 (9.4%) | 0 / 96 (0%) | 0 |
| File, save, and online services | 269 | Unmeasured | 15 / 269 (5.6%) | 0 / 269 (0%) | 0 |
| **Total** | **1,469** | **Unmeasured** | **309 / 1,469 (21.0%)** | **43 / 1,469 (2.9%)** | **12** |

The inventory records a receiver type for 525 entries (35.7%). The remaining
944 entries have not had their registration owner confirmed. This makes the
inventory denominator provisional.

## How to read the figures

- **Evidence checkpointed** means the earlier audit recorded a durable
  investigation checkpoint for that wrapper. It does not mean the function is
  implemented or correct.
- **Complete match** means the earlier audit marked the wrapper complete with
  no known parity gap. The 43 matches have not been independently retested
  against the retail build as part of this repository cleanup.
- **Out of scope** means the earlier audit intentionally excluded the entry
  from desktop parity. It is not a successful match.
- **Implementation percentage is not measured.** The available inventory
  cannot reliably distinguish a useful implementation from a stub, a partial
  implementation, or a registered function with missing behavior. Counting
  names or registrations would overstate progress.

The percentages are a starting baseline, not a claim that the game is 2.9%
playable or that all uncheckpointed functions are absent. Update the inventory
and this report together as work is validated. See [TODO.md](TODO.md) for the
next work items.
