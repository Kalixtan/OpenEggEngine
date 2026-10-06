#pragma once
#include "gmMachine.h"
#include <string>

// value type, no alpha channel at all per the confirmed decompiled struct
// layout: 3 contiguous floats) and ColourManager (the real global that
// owns the named-colour palette loaded from Platformer/colours.dat --
// confirmed real via corpus usage, e.g. game_data/Main.gm:973
// `ColourManager.load("Platformer/colours.dat")`, but with zero
// Built entirely from usage patterns: getColour/setColour/
// makeInterpolatedColour all accept a colour name (string), a colour ID
// (int, from getColourID), or an existing COLOUR object interchangeably
// (confirmed by real call sites mixing all three), so ColourManager
// resolves any of the three at each call site rather than fixing one
// argument type.
void RegisterColourBindings(gmMachine& a_machine);

// Native drawList method: same named-colour interpolation as the global
// COLOURSCHEME helper, recorded on the receiver's command list.
int GM_CDECL DrawList_SetInterpolatedColour(gmThread* a_thread);

// Resolves a_var (a colour name string, a ColourManager colour ID int, or
// an existing COLOUR object) to plain floats. Shared with
// draw_context.cpp's setColour, which is confirmed real to accept a
// colour name/object as its first argument in ~1000 of the ~1500 real
// setColour(...) call sites across the corpus (`drawList.setColour(
// "cory_white", alpha)` is the single most common form, more common than
// the plain (r,g,b,[a]) form) -- ColourManager's own by-name/by-object
// resolution logic is the same lookup this needs, so it's exposed here
// rather than duplicated.
bool ResolveColourRGB(gmThread* a_thread, const gmVariable& a_var, float& a_r, float& a_g, float& a_b);

// Same by-name resolution as ResolveColourRGB (ColourManager registry
// first, falling back to the known-name table) but callable from plain C++
// with no gmThread/gmVariable in hand -- for native (non-script) draw code
// that just wants e.g. "player_fill"'s RGB, such as the native player body
// renderer (entity_bindings.cpp's DrawPlayerNative).
bool GetNamedColourRGB(const std::string& a_name, float& a_r, float& a_g, float& a_b);

// Active colour-scheme ID (COLOURSCHEME_SetColourScheme); diagnostics.
int GetCurrentColourSchemeID();
