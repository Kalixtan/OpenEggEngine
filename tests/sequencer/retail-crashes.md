# Retail crash probes

## Move the final trigger

`unsafe_inputs/tail_move.gm` creates two triggers and passes the final one to
`moveTriggerToFront`. The retail PS3 run printed `unsafe.tail.before=2`, then
RPCS3 reported an access violation reading `0x4` at `0x0016d2c8`. It never
printed `unsafe.tail.after`. OpenEgg printed both readings and stayed running.
The equal-coordinate final trigger in `equal_tail_move.gm` crashed the same way.
Moving the only trigger in `single_move.gm` completed on retail.

The call enters retail `FUN_0016d240`. Its disassembly takes the next list node
before unlinking the selected node, then continues the insertion walk. The
tail position appears to leave that walk with the list sentinel; the exact
faulting dereference still needs a Ghidra trace. Avoid moving the last of
multiple triggers in scripts intended for the retail PS3 engine. OpenEgg warns
and continues.

This probe is separate from the scored suite because matching a process crash
would not be a useful OpenEgg behavior.
