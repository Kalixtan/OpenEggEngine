#pragma once

#include "gmMachine.h"
#include "gmThread.h"

void RegisterVideoDrawBufferBindings(gmMachine& a_machine);

int GM_CDECL gmf_Video_createDrawBuffer(gmThread* a_thread);
int GM_CDECL gmf_Video_destroyDrawBuffer(gmThread* a_thread);
int GM_CDECL gmf_Video_getNumDrawBuffers(gmThread* a_thread);
