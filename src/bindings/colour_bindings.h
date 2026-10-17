#pragma once
#include "gmMachine.h"
#include <string>

void RegisterColourBindings(gmMachine& a_machine);
extern gmType g_colourType;

// Native drawList method: same named-colour interpolation as the global
// COLOURSCHEME helper, recorded on the receiver's command list.
int GM_CDECL DrawList_SetInterpolatedColour(gmThread* a_thread);
int GM_CDECL DrawList_SetColourHSL(gmThread* a_thread);
int GM_CDECL DrawList_SetColourWithLightnessShift(gmThread* a_thread);

bool ResolveColourRGB(gmThread* a_thread, const gmVariable& a_var, float& a_r, float& a_g, float& a_b);

// Live reference readers used by DrawList.setColourRef. Manager lookups
// resolve once when opcode 0x0c is recorded; replay reads the same RGB record
// pointer, as retail does. COLOUR user objects are retained by their list.
bool GetColourObjectRGB(gmObject* a_object, float& a_r, float& a_g, float& a_b);
const void* GetColourManagerReferenceByID(int a_id);
const void* GetColourManagerReferenceByName(const std::string& a_name);
bool ReadColourManagerReference(const void* a_reference,
                                float& a_r, float& a_g, float& a_b);

// Same by-name resolution as ResolveColourRGB (ColourManager registry
// first, falling back to the known-name table) but callable from plain C++
// with no gmThread/gmVariable in hand -- for native (non-script) draw code
// that just wants e.g. "player_fill"'s RGB, such as the native player body
// renderer (entity_bindings.cpp's DrawPlayerNative).
bool GetNamedColourRGB(const std::string& a_name, float& a_r, float& a_g, float& a_b);
// Retail HSL interpolation between two currently selected named-scheme
// colours, shared with native render passes that animate palette entries.
bool GetInterpolatedNamedColourRGB(float a_amount, const std::string& a_from,
                                   const std::string& a_to,
                                   float& a_r, float& a_g, float& a_b);

// Active colour-scheme ID (COLOURSCHEME_SetColourScheme); diagnostics.
int GetCurrentColourSchemeID();
