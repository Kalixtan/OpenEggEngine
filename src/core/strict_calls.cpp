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

} // namespace StrictCalls

namespace {

bool IsNativeTable(gmMachine* a_machine, const gmVariable& a_var) {
    if (!a_machine || a_var.m_type != GM_TABLE) return false;
    gmTableObject* table = (gmTableObject*)a_machine->GetObject(a_var.m_value.m_ref);
    return table && table->Get(a_machine, kNativeMarkerKey).m_type == GM_INT;
}

// ---------------------------------------------------------------------------
// Retail reference lookup
// ---------------------------------------------------------------------------

// Tries the fully-qualified name first ("Video.getDimensions"), then the bare
// method name ("getDimensions"). The qualified form matters: getDimensions,
// isValid, getPos and friends exist on several unrelated classes at different
// addresses, and pointing at the wrong one is worse than pointing at nothing.
const RetailSymbol* FindRetailSymbol(const std::string& a_name) {
    for (const RetailSymbol& sym : kRetailSymbols) {
        if (a_name == sym.name) return &sym;
    }
    // Bare-name fallback. BUG FIX: this used to only run when a_name itself
    // contained a dot (i.e. was already qualified, "Video.getDimensions").
    // OnNullCall's unbound path never has that -- it only ever recovers the
    // bare member name from the failed BC_GETDOT/BC_GETTHIS lookup (it does
    // not know what table the call was made through), so a real symbol like
    // "Video.setAsPauseProcess" was unreachable from a report of the bare
    // "setAsPauseProcess" call, even though the index had it. Now always
    // computes the bare form (a_name itself if it has no dot) and matches it
    // against every symbol's own bare name.
    size_t dot = a_name.find('.');
    std::string bare = (dot != std::string::npos) ? a_name.substr(dot + 1) : a_name;
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
        // EntityRegistration.c:130 -- forwards only its first bool arg into
        // one opaque call (FUN_001ceb88), body not further decompiled, but
        // the very next function in the same file is
        // setIsRefreshDrawListOnChange -- strongly suggesting this is a
        // "mark my cached drawList dirty on this beat" hint for retail's
        // real deferred-command-buffer optimization, not a visual effect.
        // This engine already rebuilds every drawList from scratch every
        // frame (no persistent cache to invalidate), so the hint has
        // nothing to act on here regardless -- less certain than
        // EntityBase.update's literal empty body, but the architecture gap
        // it would bridge doesn't exist in this engine to begin with.
        "onMusicTrigger",
        "traceTick",
        // classes/Input.c 0x0007442c, a genuine native vtable-slot call),
        // and IS bound as a library function (Input.enableBackPanel is a
        // real no-op registered in input_bindings.cpp). This specific
        // report is a DIFFERENT call shape: Main.gm:10575's
        // `this.TouchEditProcess.enableBackPanel()` on the PS3 edit-
        // process's own returned table, which never defines this method at
        // all (confirmed by reading TouchEditProcessBase.gm and
        // TouchEditProcessPS3.gm in full -- TouchEditProcessPS3.gm defines
        // enableShaking as a real delegate to Input.enableShaking() right
        // next to where enableBackPanel would be, but never defines
        // enableBackPanel itself). PS3 hardware has no rear touch panel
        // (unlike NGP/Vita, whose TouchEditProcessNGP.gm DOES define this
        // delegate) -- this call resolving to nothing is confirmed correct
        // real PS3 behavior, not an unimplemented gap.
        "enableBackPanel",
        // generic native vtable-slot forwarder, `(*vtable[0xf])();` in C++
        // terms) -- but that's a NATIVE C++ object's draw() dispatch, and
        // this specific report's receiver (EntityMenuTopBase.gm:898's
        // `this.widgets[i].draw(drawList)` loop, hitting
        // EntityMenuTopPS3.gm's `addInputHandler()`-built inputHandler) is
        // a PLAIN script `table()` literal with no native backing and no
        // vtable of any kind -- confirmed by reading its full definition:
        // it only ever assigns `.update`, never `.draw`, an intentional
        // input-only overlay widget. The Misc.c body above is retail_
        // symbols.inc's closest bare-name match (this survey mechanism
        // isn't receiver-aware, see FindRetailSymbol's own header comment),
        // not evidence this exact call site needs it. Real retail's own
        // widgets[i] loop would hit the identical "no draw() on this
        // table" miss for the identical reason. Narrower risk than it
        // looks: entity/process draw hooks dispatch through a DIFFERENT
        // path (DrawEntitiesWithNativeDrawHook/ProcessManager's
        // BeginTableFunction calls, both of which check `Get("draw").
        // m_type==GM_FUNCTION` before ever calling) that never reaches
        // OnNullCall at all, so this exemption can only affect bespoke
        // script-table widgets like this one, not entities or processes.
        "draw",
    };
    for (const char* n : kNoOps) if (a_name == n) return true;
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

    // Primary signal: was the failed lookup performed ON an object this
    // engine created (a RegisterLibrary global, an entity/process table, a
    // factory-returned object)? If so this is unambiguous -- a real API
    // surface this engine owns is missing a method -- and gets reported
    // unconditionally, with no need for the name to already be catalogued in
    // retail_symbols.inc. This is what catches a genuinely-missing native
    // the first time the game reaches it, instead of only ones already known
    // about.
    bool receiverIsNative = IsNativeTable(a_thread->GetMachine(), a_thread->GetLastNullReceiver());

    // Fallback signal, for calls with no attributable native receiver (bare
    // globals, or a lookup on a plain script table): fall back to the retail
    // symbol index. See the long-form rationale below -- this is still needed
    // for BC_GETGLOBAL, whose "receiver" is the shared globals table that
    // mixes real engine globals with script-defined ones, so table identity
    // alone can't tell them apart there.
    // (and the one GMSCRIPT_PATCHES.md #4 documents by name): a script-side
    // dynamic-dispatch CONVENTION, not a missing engine API. State.gm's own
    // changeState() is patch #4's literal source example --
    // `sd.onStateSteal(s,this.parent)`, called unconditionally on `sd`, a
    // plain `table(...)` literal that most states never define this key on.
    // `sd`/`Video`/`GameSystem` are indistinguishable by GM_TABLE type tag
    // alone (RegisterLibrary's tables are plain gmTableObjects too), so the
    // signal used here is the retail symbol index instead (retail_symbols.inc,
    // ~1300 names extracted from the real EBOOT.elf): if the failed lookup
    // isn't a name retail's own binary ever exposed as a native function,
    // it's overwhelmingly more likely a script-authored callback hook than
    // an engine gap, and reporting it would misdirect the next porting step
    // at a script convention instead of a real missing binding. This does
    // mean a genuinely-missing native whose name isn't in the index yet
    // (like VMSHAPE.setAsFilledCircle was, before it got implemented --
    // corpus-confirmed real but never decompiled/clustered) goes quiet here
    // too; GE3_STRICT=survey plus a corpus grep for the symbol is the way to
    // catch those, same as setAsFilledCircle was found.
    if (!receiverIsNative && !FindRetailSymbol(a_symbol)) return;
    if (IsConfirmedNoOp(a_symbol)) return;

    std::string name = a_symbol;
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
