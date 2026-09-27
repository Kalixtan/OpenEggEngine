# Patches to vendored GameMonkeyScript (`vendor/gmscript`)

Upstream is an unmodified checkout of `publicrepo/gmscript` (stripped of its
`.git` dir for vendoring). Everything below is a deliberate deviation, made
to match the retail engine's actual dialect -- confirmed against the full
`game_data` script corpus, not guessed. Every change is also marked inline
with an `ENGINE PATCH` comment at its location.

## 1. `?` "exists" prefix operator

Confirmed real retail syntax, used pervasively (`!?x`, `?a.b.c`, `?(expr)` --
435 of 728 real `.gm` files, ~60%). Not present in stock GameMonkeyScript's
grammar at all. Semantics: `?X` evaluates to a boolean, true if `X` is
non-null, **without** letting a null anywhere in the middle of a
member/index chain kill the thread the way a bare `a.b.c` would when `a.b`
is a valid-but-absent table key (confirmed: `BC_GETDOT`/`BC_GETIND` on a
null operand normally jump straight to the thread's exception handler,
which kills the whole thread -- not scoped/catchable).

Files touched:
- `gm/gmByteCode.h` -- two new opcodes, `BC_EXISTS_BEGIN`/`BC_EXISTS_END`,
  appended at the very end of the enum (NOT inserted into the
  `BC_GETDOT..BC_NOP` prefix, which must stay positionally aligned with the
  parallel `gmOperator` enum).
- `gm/gmByteCode.cpp` -- disassembler names for the two opcodes.
- `gm/gmByteCodeGen.cpp` -- `AdjustStack()` cases (both stack-neutral).
- `gm/gmCodeTree.h` -- new `CTNOT_UNARY_EXISTS` node subtype.
- `gm/gmParser.y` -- `'?'` added to `unary_operator`, same precedence tier
  as `'!'`.
- `gm/gmScanner.l` -- `?` token.
- `gm/gmCodeGen.cpp` -- new `GenExprOpExists()`. Unlike every other unary
  operator (operand compiled first, then one opcode emitted), this wraps
  the operand in a `BC_EXISTS_BEGIN`/`BC_EXISTS_END` bracket, since
  suppressing null-related exceptions has to be active for the operand's
  *entire* evaluation, not applied after the fact.
- `gm/gmThread.h` -- two new `gmThread` fields, `m_existsDepth`/
  `m_existsHitNull`. Real scripts don't nest `?` (verified: no genuine `??`
  usage anywhere in game_data, only inside comments), so a single shared
  depth counter + hit flag is sufficient.
- `gm/gmThread.cpp` -- `BC_GETDOT`'s and `BC_GETIND`'s null-operand branches
  now check `m_existsDepth > 0` and suppress instead of throwing;
  `BC_EXISTS_BEGIN`/`END` handlers; `Sys_Reset()` resets both fields.

Not covered: calling a null value as a function (`PushStackFrame`'s
"attempt to call non function type" path) is NOT suppressed by `?`. No real
usage pattern needed it -- every `?(x.foo())` in the corpus has `foo`
guaranteed to exist; the `?` is checking the *return value*, not guarding
the call itself.

identifier, member chain, nested-null chain (must not crash -- the critical
case), `!?`, literal `null`, undefined global, parenthesized form, and use
in an `if` condition. All passed.

## 2. `->` as an alternate spelling of `.`

Confirmed real retail syntax (107 files: `v->x`, `pos->y`, `pb->getRadius()`,
etc.) Initially assumed to be assignment-only sugar for mutating vector2D
components in place; usage survey disproved that -- it's used for reads
(`v->x>0`), writes (`pos->x=0`), *and* method calls
(`pb->getMaxRotateSpeed()`) with no distinguishable semantic difference
from `.`. Simplest fix matching all observed usage: `gm/gmScanner.l` lexes
`"->"` as the same token as `"."`. No parser or codegen changes needed --
it becomes indistinguishable from `.` after lexing.

## 3. `switch`/`case`/`default` no longer reserved

`GM_USE_SWITCH` existed in `gmConfig.h` specifically to gate switch-statement
support, and `gmParser.y`/`gmCodeGen.cpp` already respect it -- but the
*scanner* reserved the three keywords unconditionally regardless of the
flag, an upstream inconsistency. Confirmed **zero** `switch(...)` statements
anywhere in the real retail corpus, but `default` is used as a plain
variable name 73 times across 48 files (`local default = ...`). Fix:
`GM_USE_SWITCH` set to `0` in `gm/gmConfig.h`, and the three keyword rules
removed from `gm/gmScanner.l` (flex's rules section here doesn't tolerate a
bare `#if`/`#endif` -- breaks its rule parser -- so they're omitted outright
rather than conditionally compiled). They fall through to the generic
`IDENTIFIER` rule.

## 4. Calling a null value is a no-op, not an exception (calling any OTHER non-function value still throws)

`gm/gmThread.cpp`'s `PushStackFrame()`: when the callee slot is specifically
`GM_NULL`, it collapses the call to a `null` return (same stack cleanup
stock GM already does when a native function returns nothing) instead of
raising `SYS_EXCEPTION` and killing the thread. Any other non-function type
(int, string, table, ...) still throws exactly like stock GM.

**Confirmed against the decompiled retail binary** (Ghidra, `EBOOT.elf`),
via the retail-only string `"\" is not a function."` (different from stock
GM's `"attempt to call non function type"` -- retail customized this
message, direct evidence they touched this code path). The function that
owns that string (`FUN_000f31f0 @ 0x000f31f0`, retail's `PushStackFrame`
equivalent) is a 3-way branch on the callee's type tag:
- type `0` (`GM_NULL`): sets the return slot to null, returns success
  (`uVar1 = 0`) -- no exception, thread keeps running.
- type `0x27` (`GM_FUNCTION`): normal call.
- anything else: builds an elaborate `"<name> is not a function. <name>
  is: <value>"` diagnostic, logs it, returns failure (`uVar1 = 8`) -- a
  real exception, unconditionally.

So the leniency is narrowly "a missing/never-assigned callback (null)
doesn't crash the game" -- matches Sound Shapes shipping a full in-game
level editor, where community-authored GM scripts leaving an optional
callback undefined is normal, not a bug -- NOT "calling anything
non-callable is fine". A genuine type mismatch (e.g. a misspelled global
that resolves to a string or table instead of a function) still throws,
so that class of real bug stays visible instead of being silently masked.

Originally written broader (any non-function type treated leniently),
found by hitting it live: real `game_data/State.gm`'s `changeState()` calls
`sd.onStateSteal(s,this.parent)` unconditionally (unlike every other
callback dispatch in the same file -- `call()`/`update()` both guard with
`?`), and real `game_data/Main.gm`'s very first
`this.saveState.changeState(...)` reaches exactly that call on a state
table that never defines `onStateSteal`. Narrowed to null-only after
checking the decompiled evidence above.

## Regenerating `gmParser.cpp`/`gmScanner.cpp` after further grammar changes

These are committed pre-generated output, not built from `.y`/`.l` at
compile time. To regenerate after editing the grammar:

```bash
cd vendor/gmscript/gmsrc/src/gm
bison -o gmParser.cpp --defines=gmParser.cpp.h -l -p gm gmParser.y
flex -ogmScanner.cpp -Pgm gmScanner.l
```

Notes:
- `--defines=gmParser.cpp.h` is required -- modern bison's default header
  name for a `.cpp` output is `gmParser.hpp`, but `gmScanner.cpp` expects to
  `#include "gmParser.cpp.h"`. Token numbering is stable across bison
  versions as long as no new *named* tokens are added (single-char literal
  tokens like `?`/`->` don't get `#define`d, so they never shift anything).
- The original build used ancient bison 1.x/flex custom skeletons
  (`bison.simple`/`bison.hairy`/`flex.skl`, still present in the tree for
  reference) that modern bison 3.8.2 / flex 2.6.4 don't need or use --
  their own default skeletons work fine for this grammar.
- A handful of harmless pre-existing warnings show up every regen (a
  duplicate `":"` rule in `gmScanner.l`, a duplicate `CONSTANT_FLOAT`
  `%token` in `gmParser.y`) -- both predate these patches, not introduced
  by them.

## Corpus verification

`CompileStringToFunction` (compile-only, no execution) run against every
`.gm` file in `game_data`, independently:

| Pass | Fix | Result |
|---|---|---|
| 0 | (none, stock grammar) | ~40% (estimate; not separately measured) |
| 1 | `?` operator | 643/728 (88%) |
| 2 | `->` alias | 724/728 (99.5%) |
| 3 | switch/case/default un-reserved | 725/728 (99.6%) |

The remaining 3 "failures" are not engine gaps:
- `Platformer/UIImageView.gm`, `Platformer/UITextView.gm` -- genuinely
  empty (0 bytes) and dead: the only references to them are commented-out
  `GameMonkeyScript.loadScript(...)` calls. Nothing ever loads them.
- `localization/ExtraStrings.gm` -- explicitly documented in its own header
  as "NOT a real gm file... this code is not used in game"; it's a
  string-extraction template read by tooling/the native engine directly,
  never compiled as GameMonkeyScript.

Effectively **100% of real, actually-loaded retail scripts compile clean.**
