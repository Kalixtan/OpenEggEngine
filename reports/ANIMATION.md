# Animation probe

Generated: 2026-10-21 17:51 UTC
Retail comparison: **39/39 matched**.
Asset: `Platformer/gameAssets/checkpointTriggeredInactive.fsa` (copied from the supplied extracted game data for this run).
Local checks: **16/16 passed**.

## Checks

| Check | Result |
| --- | --- |
| resource loaded | PASS |
| frame count exposed | PASS |
| dimensions positive | PASS |
| play time round trip | PASS |
| seek round trip | PASS |
| frame selection boundaries | PASS |
| play/pause states | PASS |
| reverse state | PASS |
| unload leaves shared cache intact | PASS |
| custom sequence frame selection | PASS |
| shared resource sequence state | PASS |
| cache clear preserves live handles | PASS |
| missing resource stays empty | PASS |
| retail fault warning is precise | PASS |
| entity missing resource warns | PASS |
| module load remains detached; speed has no effect | PASS |

## Readings

| Probe | OpenEgg | Retail | Match |
| --- | --- | --- | --- |
| `Animation.getNumImages.initial` | `35` | `35` | yes |
| `Animation.getNumFrames.initial` | `35` | `35` | yes |
| `Animation.getSourcePixelWidth.initial` | `60.000000` | `60.000000` | yes |
| `Animation.getSourcePixelHeight.initial` | `80.000000` | `80.000000` | yes |
| `Animation.getCurrentFrame.initial` | `0` | `0` | yes |
| `Animation.getPlayTime.initial` | `1000.000000` | `1000.000000` | yes |
| `Animation.isPlaying.initial` | `0` | `0` | yes |
| `Animation.isPaused.initial` | `1` | `1` | yes |
| `Animation.isDone.initial` | `0` | `0` | yes |
| `Animation.getNumImages.sharedResource` | `35` | `35` | yes |
| `Animation.getPlayTime.changed` | `120.000000` | `120.000008` | yes |
| `Animation.getPlayPosition.seek` | `60.000004` | `60.000004` | yes |
| `Animation.getCurrentFrame.seek` | `17` | `17` | yes |
| `Animation.getCurrentFrame.firstFrame` | `0` | `0` | yes |
| `Animation.getCurrentFrame.set` | `8` | `8` | yes |
| `Animation.getCurrentFrame.clampedFrame` | `34` | `34` | yes |
| `Animation.isPlaying.play` | `1` | `1` | yes |
| `Animation.isPaused.play` | `0` | `0` | yes |
| `Animation.isPlaying.pause` | `0` | `0` | yes |
| `Animation.isPaused.pause` | `1` | `1` | yes |
| `Animation.isPlaying.playOnce` | `1` | `1` | yes |
| `Animation.isDone.end` | `1` | `1` | yes |
| `Animation.isPlaying.end` | `0` | `0` | yes |
| `Animation.isReversed.true` | `1` | `1` | yes |
| `Animation.getCurrentFrame.reversed` | `34` | `34` | yes |
| `Animation.isReversed.false` | `0` | `0` | yes |
| `Animation.getNumFrames.customSequence` | `2` | `2` | yes |
| `Animation.getNumFrames.sharedSequence` | `2` | `2` | yes |
| `Animation.getCurrentFrame.sequenceFirst` | `0` | `0` | yes |
| `Animation.getCurrentFrame.sequenceSecond` | `34` | `34` | yes |
| `Animation.getNumImages.unloaded` | `35` | `35` | yes |
| `Animation.getNumImages.sharedStillLoaded` | `35` | `35` | yes |
| `Animation.getNumFrames.individualReload` | `2` | `2` | yes |
| `Animation.getNumImages.afterUnloadAll` | `35` | `35` | yes |
| `Animation.getNumFrames.afterUnloadAll` | `35` | `35` | yes |
| `Animation.getPlayPosition.slowSpeed` | `0.000000` | `0.000000` | yes |
| `Animation.getPlayPosition.fastSpeed` | `0.000000` | `0.000000` | yes |
| `Animation.setPlayTimeSpeed.noEffect` | `1` | `1` | yes |
| `Animation.getPlayPosition.moduleLoadDetached` | `1` | `1` | yes |
| `Animation.getNumImages.missingResource` | `0` | `retail fault at 0x48` | retail crash confirmed |
| `Entity.loadAnimation.missingResource` | `0` | `retail fault at 0x48` | retail crash confirmed |
