# Confirmed retail faults

| Probe | Retail result | OpenEgg result |
| --- | --- | --- |
| `missing_resource.gm` | VM access violation reading `0x48` | Empty Animation plus warning |
| `entity_missing_resource.gm` | VM access violation reading `0x48` | Empty Animation plus warning |
| `negative_frame.gm` | PPU `0x000b6848` reads unmapped `0x38600004` | Script exception plus warning; process stays up |

The missing-resource cases were captured under headless RPCS3 on 2026-10-21.
The negative-index case faults after its `started` marker and before its `returned`
marker. `negative-frame-retail.tsv` records that result. Re-capture it with
`capture_negative_frame.py`; the command succeeds only when the fault and marker
order match. Logs stay under ignored `tests/_results/animation/`.
