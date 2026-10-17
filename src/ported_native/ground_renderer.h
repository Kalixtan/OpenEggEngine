#pragma once


struct DrawListData;

// Called by EntityRegistration.drawGrounds while Main.gm is recording its
// preDraw list. Appends native ground rendering at retail's list position.
void RecordGroundsNative(DrawListData* a_drawList);
