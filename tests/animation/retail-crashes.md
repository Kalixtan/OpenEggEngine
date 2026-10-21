# Confirmed retail fault

`missing_resource.gm` and `entity_missing_resource.gm` request an FSA that is
not present in game data. The PS3 retail build under RPCS3 faults with a VM
access violation reading `0x48` in both load paths. OpenEgg returns an empty
Animation object and warns instead. These cases stay separate from the playback
comparison so the retail fault does not stop other probes.

Captured 2026-10-21 with the headless RPCS3 probe command in `README.md`.
