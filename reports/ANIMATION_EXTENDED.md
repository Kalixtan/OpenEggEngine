# Animation edge probes

Generated: 2026-10-22 14:17 UTC
Local checks: **23/23 passed**.
Captured retail readings: **20/20 matched**.

## Checks

| Check | Result |
| --- | --- |
| Animation.records.images | PASS |
| Animation.records.frames | PASS |
| Animation.records.first | PASS |
| Animation.records.middle | PASS |
| Animation.records.last | PASS |
| Animation.records.clamped | PASS |
| Animation.entity.enabled.initial | PASS |
| Animation.entity.enabled.disabled | PASS |
| Animation.entity.disabled.clockStopped | PASS |
| Animation.entity.enabled.restored | PASS |
| Animation.entity.enabled.clockResumed | PASS |
| Animation.resource.entityFrames | PASS |
| Animation.resource.sharedFrames | PASS |
| Animation.resource.playbackAdvanced | PASS |
| Animation.resource.entityAfterModuleUnload | PASS |
| Animation.resource.sharedAfterModuleUnload | PASS |
| Animation.resource.entityAfterOwnedUnload | PASS |
| Animation.resource.entityDeleted | PASS |
| Animation.resource.reloadFrames | PASS |
| negative index does not crash OpenEgg | PASS |
| retail negative-index fault is recorded | PASS |
| retail probe captures are current | PASS |
| retail-fault warning is limited to negative index | PASS |

## Readings

| Probe | OpenEgg | Retail | Match |
| --- | --- | --- | --- |
| `Animation.entity.disabled.clockStopped` | `1` | `1` | yes |
| `Animation.entity.enabled.clockResumed` | `1` | `1` | yes |
| `Animation.entity.enabled.disabled` | `0` | `0` | yes |
| `Animation.entity.enabled.initial` | `1` | `1` | yes |
| `Animation.entity.enabled.restored` | `1` | `1` | yes |
| `Animation.negativeFrame.started` | `1` | `1` | yes |
| `Animation.records.clamped` | `18` | `18` | yes |
| `Animation.records.first` | `0` | `0` | yes |
| `Animation.records.frames` | `19` | `19` | yes |
| `Animation.records.images` | `19` | `19` | yes |
| `Animation.records.last` | `18` | `18` | yes |
| `Animation.records.middle` | `9` | `9` | yes |
| `Animation.resource.entityAfterModuleUnload` | `35` | `35` | yes |
| `Animation.resource.entityAfterOwnedUnload` | `35` | `35` | yes |
| `Animation.resource.entityDeleted` | `1` | `1` | yes |
| `Animation.resource.entityFrames` | `35` | `35` | yes |
| `Animation.resource.playbackAdvanced` | `1` | `1` | yes |
| `Animation.resource.reloadFrames` | `35` | `35` | yes |
| `Animation.resource.sharedAfterModuleUnload` | `35` | `35` | yes |
| `Animation.resource.sharedFrames` | `35` | `35` | yes |

Negative frame call returned to script: **no**.
Retail fault confirmed: PPU `0x000b6848` read unmapped `0x38600004`.
The invalid-index case stays isolated so its retail fault cannot stop other probes.
