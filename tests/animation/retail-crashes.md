# Confirmed retail fault

`missing_resource.gm` requests an FSA that is not present in game data. The PS3
retail build under RPCS3 faults with a VM access violation reading `0x48` during
that request. OpenEgg returns an empty Animation object instead. This case stays
separate from the playback comparison so its fault does not stop other probes.

Captured 2026-10-21 with the headless RPCS3 probe command in `README.md`.
