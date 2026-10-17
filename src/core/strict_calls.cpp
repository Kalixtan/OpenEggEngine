#include "strict_calls.h"
#include "gmFunctionObject.h"
#include "gmStringObject.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct RetailSymbol {
    const char* name;   // "Video.setTexture", or a bare global name
    const char* source;
    const char* address;
};

#include "retail_symbols.inc"

gmMachine* g_machine = nullptr;
StrictCalls::HaltDialogHandler g_haltDialogHandler = nullptr;
void* g_haltDialogContext = nullptr;

// ---------------------------------------------------------------------------
// Native-table marking
// ---------------------------------------------------------------------------
//
// A hidden key set directly on tables the engine itself builds (RegisterLibrary
// globals, entity/process handle & behavior tables, factory-returned objects
// like SHAPE/vector2D/...). Same trick as entity_bindings.cpp's
// __behaviorOwner: a plain script table never has this key, so its presence
// is a reliable "this object is ours" signal without needing a name registry.
const char* const kNativeMarkerKey = "__ge3_native";

} // namespace

namespace StrictCalls {

void MarkNative(gmMachine& a_machine, gmTableObject* a_table) {
    if (!a_table) return;
    if (a_table->Get(&a_machine, kNativeMarkerKey).m_type == GM_INT) return; // already marked
    a_table->Set(&a_machine, kNativeMarkerKey, gmVariable(1));
}

void MarkNative(gmMachine& a_machine, const gmVariable& a_tableVar) {
    if (a_tableVar.m_type != GM_TABLE) return;
    MarkNative(a_machine, (gmTableObject*)a_machine.GetObject(a_tableVar.m_value.m_ref));
}

void MarkScriptExtensibleNative(gmMachine& a_machine, gmTableObject* a_table) {
    if (a_table) a_table->Set(&a_machine, kNativeMarkerKey, gmVariable(2));
}

void MarkScriptExtensibleNative(gmMachine& a_machine, const gmVariable& a_tableVar) {
    if (a_tableVar.m_type != GM_TABLE) return;
    MarkScriptExtensibleNative(a_machine,
        (gmTableObject*)a_machine.GetObject(a_tableVar.m_value.m_ref));
}

} // namespace StrictCalls

namespace {

bool IsNativeTable(gmMachine* a_machine, const gmVariable& a_var) {
    if (!a_machine || a_var.m_type != GM_TABLE) return false;
    gmTableObject* table = (gmTableObject*)a_machine->GetObject(a_var.m_value.m_ref);
    return table && table->Get(a_machine, kNativeMarkerKey).m_type == GM_INT;
}

bool IsScriptExtensibleNativeTable(gmMachine* a_machine, const gmVariable& a_var) {
    if (!a_machine || a_var.m_type != GM_TABLE) return false;
    gmTableObject* table = (gmTableObject*)a_machine->GetObject(a_var.m_value.m_ref);
    gmVariable marker = table ? table->Get(a_machine, kNativeMarkerKey) : gmVariable::s_null;
    return marker.m_type == GM_INT && marker.m_value.m_int == 2;
}

// ---------------------------------------------------------------------------
// Retail reference lookup
// ---------------------------------------------------------------------------

// A receiver-qualified name must match exactly: the same bare method name
// can belong to an unrelated native class. Only unattributed bare calls use
// the fallback index lookup, where the result is necessarily less certain.
const RetailSymbol* FindRetailSymbol(const std::string& a_name) {
    for (const RetailSymbol& sym : kRetailSymbols) {
        if (a_name == sym.name) return &sym;
    }
    if (a_name.find('.') != std::string::npos) return nullptr;
    // BC_GETGLOBAL and mixed native/script tables only give us the bare
    // member name. Match its symbol suffix when no receiver class is known.
    const std::string& bare = a_name;
    for (const RetailSymbol& sym : kRetailSymbols) {
        const char* symDot = std::strchr(sym.name, '.');
        if (symDot ? (bare == symDot + 1) : (bare == sym.name)) return &sym;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Call-site recovery
// ---------------------------------------------------------------------------

bool CallSite(gmThread* a_thread, std::string& a_file, int& a_line) {
    if (!g_machine || !a_thread->GetStrictCallerFunction()) return false;
    gmFunctionObject* fn = (gmFunctionObject*)g_machine->GetObject(a_thread->GetStrictCallerFunction());
    if (!fn) return false;

    a_line = fn->GetLine(a_thread->GetStrictCallerInstruction());

    const char* source = nullptr;
    const char* filename = nullptr;
    if (g_machine->GetSourceCode(fn->GetSourceId(), source, filename) && filename) {
        a_file = filename;
    } else {
        a_file = fn->GetDebugName() ? fn->GetDebugName() : "<unknown>";
    }
    return true;
}

std::string DescribeValue(gmMachine* a_machine, const gmVariable& a_var) {
    char buf[192];
    const char* text = a_var.AsStringWithType(a_machine, buf, sizeof(buf));
    return text ? text : "?";
}

// ---------------------------------------------------------------------------
// Survey bookkeeping
// ---------------------------------------------------------------------------

struct SurveyEntry {
    std::string name;
    std::string firstCallSite;
    long long count = 0;
};
std::vector<SurveyEntry>& Survey() {
    static std::vector<SurveyEntry> entries;
    return entries;
}

// Returns true the first time this name is seen (i.e. worth printing).
bool RecordSurvey(const std::string& a_name, const std::string& a_callSite) {
    for (SurveyEntry& e : Survey()) {
        if (e.name == a_name) { ++e.count; return false; }
    }
    Survey().push_back({a_name, a_callSite, 1});
    return true;
}

// ---------------------------------------------------------------------------
// Reporting
// ---------------------------------------------------------------------------

std::string FormatReport(gmThread* a_thread, const std::string& a_name, bool a_unbound,
                         const std::string& a_file, int a_line, bool a_haveSite) {
    std::ostringstream report;
    report <<
        "\n"
        "================================================================================\n"
        " GE3 STRICT: script called an engine function that is not implemented 1-1 yet\n"
        "================================================================================\n"
        << "  function     : " << a_name << '\n'
        << "  status       : " << (a_unbound
            ? "never bound -- no implementation of any kind exists"
            : "implementation removed -- behavior still needs to be reproduced")
        << '\n';

    if (a_haveSite) {
        report << "  called from  : " << a_file << ':' << a_line << '\n';
    } else {
        report << "  called from  : <native bootstrap, no script frame>\n";
    }

    int numParams = a_thread->GetNumParams();
    report << "  arguments    : " << numParams << '\n';
    for (int i = 0; i < numParams; ++i) {
        report << "      [" << i << "] "
               << DescribeValue(a_thread->GetMachine(), a_thread->Param(i)) << '\n';
    }

    if (const RetailSymbol* sym = FindRetailSymbol(a_name)) {
        report << "  entry point  : " << sym->address << '\n';
    } else {
        report << "  entry point  : no address recorded\n";
    }

    // The GM-side call stack goes through the machine log rather than stderr,
    // so drain it here to keep the whole report in one place.
    a_thread->LogCallStack();
    if (g_machine) {
        bool first = true;
        const char* entry;
        report << "  script stack :\n";
        while ((entry = g_machine->GetLog().GetEntry(first))) {
            report << "      " << entry << '\n';
        }
        g_machine->GetLog().Reset();
    }
    report << "================================================================================\n";
    return report.str();
}

void PrintReport(const std::string& report) {
    std::fwrite(report.data(), 1, report.size(), stderr);
    std::fflush(stderr);
}

bool AskToContinue(const std::string& report) {
    return g_haltDialogHandler &&
           g_haltDialogHandler(g_haltDialogContext, report.c_str());
}

bool IsConfirmedNoOp(const std::string& a_name) {
    static const char* const kNoOps[] = {
        "update",
        "traceTick",
    };
    for (const char* n : kNoOps) if (a_name == n) return true;
    return false;
}

bool IsKnownRetailNullCall(gmThread* a_thread, const std::string& a_name) {
    auto receiverIsLibrary = [a_thread](const char* a_library) {
        gmMachine* machine = a_thread->GetMachine();
        gmVariable library = machine->GetGlobals()->Get(machine, a_library);
        const gmVariable& receiver = a_thread->GetLastNullReceiver();
        return library.m_type == GM_TABLE && receiver.m_type == GM_TABLE &&
               library.m_value.m_ref == receiver.m_value.m_ref;
    };
    if (a_name == "setEntityUID") return receiverIsLibrary("EntityManager");

    if (a_name == "stopSound" || a_name == "playSoundWithMixer")
        return receiverIsLibrary("Audio");
    if (a_name == "setRotateSpeed" || a_name == "dot" ||
        a_name == "setCatagoriesToCollideWith") {
        const gmVariable& receiver = a_thread->GetLastNullReceiver();
        if (receiver.m_type < GM_USER) return false;
        const char* typeName = a_thread->GetMachine()->GetTypeName(receiver.m_type);
        if (!typeName) return false;
        return (a_name == "setRotateSpeed" && std::strcmp(typeName, "PhysicsBody") == 0) ||
               (a_name == "setCatagoriesToCollideWith" && std::strcmp(typeName, "PhysicsBody") == 0) ||
               (a_name == "dot" && std::strcmp(typeName, "vector2D") == 0);
    }
    return false;
}

[[noreturn]] void Halt() {
    std::fprintf(stderr,
        "\nStopping here: this is the next function to reimplement.\n"
        "  Run again with GE3_STRICT=survey to list every unimplemented function the\n"
        "  game reaches, in call order, instead of stopping at the first one.\n\n");
    std::fflush(stderr);
    std::fflush(stdout);
    std::_Exit(70);
}

// The VM-side hook: a call on a value that is null because nothing ever bound
// it. See gmThread.cpp's PushStackFrame.
void OnNullCall(gmThread* a_thread, const char* a_symbol) {
    if (!a_symbol) return;
    if (IsKnownRetailNullCall(a_thread, a_symbol)) return;

    const gmVariable& receiver = a_thread->GetLastNullReceiver();
    bool receiverIsNative = IsNativeTable(a_thread->GetMachine(), receiver);
    bool receiverIsScriptExtensibleNative =
        IsScriptExtensibleNativeTable(a_thread->GetMachine(), receiver);
    // GM_USER and every type above it are registered by native code. Their
    // method lookup uses the type library rather than a marked table, so a
    // missing method must be reported even if the retail symbol index has
    // never seen its name. Otherwise precisely the newly discovered gaps
    // this trap is intended to expose pass through silently.
    bool receiverIsNativeUser = receiver.m_type >= GM_USER;

    // A plain script table may use the same method name as an unrelated
    // native class. CampaignScreenBase's widgets, for example, call an
    // optional onMusicTrigger callback even though EntityRegistration also
    // exports a native onMusicTrigger. The receiver decides which it is.
    if (receiver.m_type == GM_TABLE && !receiverIsNative) return;

    // A handle's script callbacks share this table with native methods.
    // The receiver marker alone cannot distinguish an absent callback from
    // a missing native binding, so require an indexed retail name here.
    if (receiverIsScriptExtensibleNative && !FindRetailSymbol(a_symbol)) return;

    // Bare globals fall back to the retail symbol index because BC_GETGLOBAL
    // has no receiver and its lookup uses the mixed script/native globals
    // table. Native tables and user objects have their own receiver signal.
    if (!receiverIsNative && !receiverIsNativeUser && !FindRetailSymbol(a_symbol)) return;
    if (IsConfirmedNoOp(a_symbol)) return;

    std::string name = a_symbol;
    if (receiverIsNativeUser) {
        const char* typeName = a_thread->GetMachine()->GetTypeName(receiver.m_type);
        if (typeName && *typeName) name = std::string(typeName) + "." + a_symbol;
    }
    std::string file;
    int line = 0;
    bool haveSite = CallSite(a_thread, file, line);
    std::string site = haveSite ? (file + ":" + std::to_string(line)) : std::string("<native>");

    if (StrictCalls::GetMode() == StrictCalls::Mode::Survey) {
        if (RecordSurvey(name, site)) {
            std::fprintf(stderr, "[strict] unbound: %-52s first called from %s\n",
                         name.c_str(), site.c_str());
        }
        return;
    }
    const std::string report = FormatReport(
        a_thread, name, /*unbound=*/true, file, line, haveSite);
    PrintReport(report);
    if (AskToContinue(report)) return;
    Halt();
}

} // namespace

namespace StrictCalls {

void SetHaltDialogHandler(HaltDialogHandler handler, void* context) {
    g_haltDialogHandler = handler;
    g_haltDialogContext = context;
}

Mode GetMode() {
    static Mode mode = [] {
        const char* env = std::getenv("GE3_STRICT");
        if (!env) return Mode::Halt;
        if (std::strcmp(env, "survey") == 0) return Mode::Survey;
        if (std::strcmp(env, "off") == 0 || std::strcmp(env, "0") == 0) return Mode::Off;
        return Mode::Halt;
    }();
    return mode;
}

void Install(gmMachine& a_machine) {
    g_machine = &a_machine;
    if (GetMode() == Mode::Off) return;
    // Also gates the BC_CALL call-site capture in gmThread.cpp, so leaving it
    // null in Off mode keeps the VM's hot path exactly as it was.
    g_gmNullCallHook = &OnNullCall;

    std::fprintf(stderr, "[strict] mode=%s (GE3_STRICT: halt | survey | off)\n",
                 GetMode() == Mode::Survey ? "survey" : "halt");
}

int Report(gmThread* a_thread, const char* a_name) {
    if (GetMode() == Mode::Off) {
        // Nothing to fall back on -- the fabricated bodies are gone. Null is
        // what the old lenient path produced for an unbound name anyway.
        a_thread->PushNull();
        return GM_OK;
    }

    std::string name = a_name;
    std::string file;
    int line = 0;
    bool haveSite = CallSite(a_thread, file, line);
    std::string site = haveSite ? (file + ":" + std::to_string(line)) : std::string("<native>");

    if (GetMode() == Mode::Survey) {
        if (RecordSurvey(name, site)) {
            std::fprintf(stderr, "[strict] removed: %-52s first called from %s\n",
                         name.c_str(), site.c_str());
        }
        a_thread->PushNull();
        return GM_OK;
    }

    const std::string report = FormatReport(
        a_thread, name, /*unbound=*/false, file, line, haveSite);
    PrintReport(report);
    if (AskToContinue(report)) {
        a_thread->PushNull();
        return GM_OK;
    }
    Halt();
}

void DumpSurvey() {
    if (GetMode() != Mode::Survey || Survey().empty()) return;

    // Call order is the useful order: it is the order the game needs these
    // functions to exist in, so working down the list keeps the boot moving.
    std::fprintf(stderr,
        "\n================================================================================\n"
        " GE3 STRICT survey: %zu unimplemented engine functions reached, in call order\n"
        "================================================================================\n",
        Survey().size());
    int index = 1;
    for (const SurveyEntry& e : Survey()) {
        const RetailSymbol* sym = FindRetailSymbol(e.name);
        std::fprintf(stderr, "%3d. %-46s calls=%-8lld %s\n", index++, e.name.c_str(), e.count,
                     e.firstCallSite.c_str());
        if (sym) {
            std::fprintf(stderr, "     entry point: %s\n", sym->address);
        } else {
            std::fprintf(stderr, "     entry point: no address recorded\n");
        }
    }
    std::fprintf(stderr, "================================================================================\n");
    std::fflush(stderr);
}

} // namespace StrictCalls
