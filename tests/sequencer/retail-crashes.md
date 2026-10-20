# Retail crash probes

## Move the final trigger

`unsafe_inputs/tail_move.gm` creates two triggers and passes the final one to
`moveTriggerToFront`. The retail PS3 run printed `unsafe.tail.before=2`, then
RPCS3 reported an access violation reading `0x4` at `0x0016d2c8`. It never
printed `unsafe.tail.after`. OpenEgg printed both readings and stayed running.
The equal-coordinate final trigger in `equal_tail_move.gm` crashed the same way.
Moving the only trigger in `single_move.gm` completed on retail.

Ghidra shows why. Retail `FUN_0016d240` saves the selected node's successor,
unlinks and frees the node, then walks backward from that successor before
inserting it again. For the tail, the saved successor is the list sentinel.
When another trigger remains, the walk tries to read the sentinel's empty
payload, then its position field at offset `+4`: the read from `0x4` above.
With only one trigger, the list is empty after removal and the walk is skipped.
OpenEgg warns and keeps the last trigger in a safe position.

For other positions, the same retail walk can change trigger order. The paired
`move_order.gm`, `move_matrix.gm`, and `cursor_during_move.gm` probes cover that
behavior, including a cursor held on the moved trigger.

This probe is separate from the scored suite because matching a process crash
would not be a useful OpenEgg behavior.
