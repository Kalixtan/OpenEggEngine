#pragma once
#include "gmMachine.h"

// constants confirmed" section: TEXTALIGN_*, FILEHELPER_AREA_*) but whose
// real retail integer VALUES aren't recovered yet -- these are OUR
// self-consistent placeholder values (also used internally by
// render/bitmap_font.cpp's alignment enum, keep in sync if you change them).
// SW is likewise a placeholder "world units" scale, chosen so
// game_data_min/Main.gm's `this.size = SW*16` comfortably covers
// Renderer::GetWorldHalfHeight().
void RegisterConstantsBindings(gmMachine& a_machine);
