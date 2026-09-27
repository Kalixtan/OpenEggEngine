#include "gmThread.h"
#include "printinfo_bindings.h"
#include "gmTableObject.h"

namespace {

int GM_CDECL gmf_PRINTINFO(gmThread* a_thread) {
    a_thread->PushTable(a_thread->GetMachine()->AllocTableObject());
    return GM_OK;
}

} // namespace

void RegisterPrintInfoBindings(gmMachine& a_machine) {
    a_machine.RegisterLibraryFunction("PRINTINFO", gmf_PRINTINFO);
}
