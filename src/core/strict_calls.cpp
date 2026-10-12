#include "strict_calls.h"
#include "gmFunctionObject.h"
#include "gmStringObject.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct RetailSymbol {
    const char* name;   // "Video.setTexture", or a bare global name
    const char* source;
    const char* address; // EBOOT.elf VMA, e.g. "0x00074c28"
};

#include "retail_symbols.inc"

gmMachine* g_machine = nullptr;

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

// Resolves the .gm file and line that made the call, from the caller recorded
// at BC_CALL (see gmThread.h's m_strictCallerFn). Returns false when the call
// did not come from script -- a native bootstrap call, for instance.
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

void PrintReport(gmThread* a_thread, const std::string& a_name, bool a_unbound,
                 const std::string& a_file, int a_line, bool a_haveSite) {
    std::fprintf(stderr,
        "\n"
        "================================================================================\n"
        " GE3 STRICT: script called an engine function that is not implemented 1-1 yet\n"
        "================================================================================\n");
    std::fprintf(stderr, "  function     : %s\n", a_name.c_str());
    std::fprintf(stderr, "  status       : %s\n", a_unbound
        ? "never bound -- no implementation of any kind exists"
        : "fabricated implementation removed -- needs porting from the decompiled body");

    if (a_haveSite) {
        std::fprintf(stderr, "  called from  : %s:%d\n", a_file.c_str(), a_line);
    } else {
        std::fprintf(stderr, "  called from  : <native bootstrap, no script frame>\n");
    }

    // Arguments: the real signature is often the thing you most need to know
    // before writing the replacement, and the decompiled body only tells you
    // the arity, not what the values look like in practice.
    int numParams = a_thread->GetNumParams();
    std::fprintf(stderr, "  arguments    : %d\n", numParams);
    for (int i = 0; i < numParams; ++i) {
        std::fprintf(stderr, "      [%d] %s\n", i,
                     DescribeValue(a_thread->GetMachine(), a_thread->Param(i)).c_str());
    }

    if (const RetailSymbol* sym = FindRetailSymbol(a_name)) {
        std::fprintf(stderr, "  retail body  : %s\n",
                     sym->source[0] ? sym->source : "(no decompiled body clustered)");
        std::fprintf(stderr, "  EBOOT.elf    : %s\n", sym->address);
    } else {
        std::fprintf(stderr, "  retail body  : not in the reveng index -- no decompiled body found\n");
        std::fprintf(stderr, "                 for this name (regenerate with\n");
        std::fprintf(stderr, "                 reveng/generate_retail_symbol_index.py if reveng/ changed)\n");
    }

    // The GM-side call stack goes through the machine log rather than stderr,
    // so drain it here to keep the whole report in one place.
    a_thread->LogCallStack();
    if (g_machine) {
        bool first = true;
        const char* entry;
        std::fprintf(stderr, "  script stack :\n");
        while ((entry = g_machine->GetLog().GetEntry(first))) {
            std::fprintf(stderr, "      %s\n", entry);
        }
        g_machine->GetLog().Reset();
    }
    std::fprintf(stderr, "================================================================================\n");
}

// A small, hand-curated set of retail functions whose decompiled body is a
// confirmed, literal no-op (`return 0;`, nothing else) -- e.g.
// EntityBase.update (0x001c8728). These are real, they're just genuinely
// empty, so a call reaching this engine's null-call leniency for one of
// them ISN'T a gap to port: the current "do nothing" behavior already
// matches retail bit-for-bit. Reported anyway (matching a name in
// retail_symbols.inc) would repeatedly point the halt-mode workflow at a
// function with nothing left to write, blocking real progress every frame
// something like a menu widget's `this.widgets[i].update(msElapsed)` loop
// runs. Keep this list short and only add a name here after actually
// reading its decompiled body -- this is a targeted exception, not a
// general "trust the name" allowlist.
bool IsConfirmedNoOp(const std::string& a_name) {
    static const char* const kNoOps[] = {
        "update",
        "traceTick",
    };
    for (const char* n : kNoOps) if (a_name == n) return true;
    return false;
}

// EntityBase.gm calls EntityManager.setEntityUID(uid, null) in its
// destructor, but the retail EBOOT has no registered `setEntityUID` name
// (unlike getEntityFromUID/createEntityAsPlayer immediately beside it).
// This is therefore a retail null-call, not an omitted native binding.
bool IsKnownRetailNullCall(gmThread* a_thread, const std::string& a_name) {
    auto receiverIsLibrary = [a_thread](const char* a_library) {
        gmMachine* machine = a_thread->GetMachine();
        gmVariable library = machine->GetGlobals()->Get(machine, a_library);
        const gmVariable& receiver = a_thread->GetLastNullReceiver();
        return library.m_type == GM_TABLE && receiver.m_type == GM_TABLE &&
               library.m_value.m_ref == receiver.m_value.m_ref;
    };
    if (a_name == "setEntityUID") return receiverIsLibrary("EntityManager");

    // Retail's complete 68-entry AUDIO method table (EBOOT.elf 0x421418-
    // 0x421744) contains neither of these names. SwayingTree.gm calls
    // Audio.stopSound, while GoombaCritterGuitar.gm calls Audio.
    // playSoundWithMixer. The latter is registered on EntityBase instead
    // (EBOOT.elf method table entry 0x425d8c). Both are retail null calls
    // on Audio, not missing Audio bindings. Scope these exceptions to that
    // receiver so a real method gap on another object remains visible.
    if (a_name == "stopSound" || a_name == "playSoundWithMixer")
        return receiverIsLibrary("Audio");
    // PJInvaderBullet.gm calls this on its PhysicsBody during explosion,
    // but the retail EBOOT name pool and PhysicsBody registration expose
    // setRotateVel and setAngularVel, not setRotateSpeed. Preserve the VM's
    // retail null-call behavior for this one absent method.
    // MovingAsteroid.gm also calls vector2D.dot, but retail has no "dot"
    // method string or registration on that native vector type. Keep its
    // null comparison path rather than inventing a vector operation.
    // Several shipped entities spell "Categories" as "Catagories" on a
    // PhysicsBody. Retail registers only the correctly spelled setter, so
    // these calls also take the GameMonkey null-call path in the original.
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
    // An unattributable null callee is a script-level null (an optional
    // callback a level never defined, say), not a missing engine function --
    // that is real retail behavior (GMSCRIPT_PATCHES.md #4) and must stay
    // silent, or every '?'-guarded optional hook in the corpus trips the trap.
    if (!a_symbol) return;
    if (IsKnownRetailNullCall(a_thread, a_symbol)) return;

    // Primary signal: was the failed lookup performed ON an object this
    // engine created (a RegisterLibrary global, an entity/process table, a
    // factory-returned object)? If so this is unambiguous -- a real API
    // surface this engine owns is missing a method -- and gets reported
    // unconditionally, with no need for the name to already be catalogued in
    // retail_symbols.inc. This is what catches a genuinely-missing native
    // the first time the game reaches it, instead of only ones already known
    // about.
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
    PrintReport(a_thread, name, /*unbound=*/true, file, line, haveSite);
    Halt();
}

} // namespace

namespace StrictCalls {

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

    PrintReport(a_thread, name, /*unbound=*/false, file, line, haveSite);
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
            std::fprintf(stderr, "     retail: %s %s\n",
                         sym->source[0] ? sym->source : "(address only)", sym->address);
        } else {
            std::fprintf(stderr, "     retail: not in the reveng index\n");
        }
    }
    std::fprintf(stderr, "================================================================================\n");
    std::fflush(stderr);
}

} // namespace StrictCalls
