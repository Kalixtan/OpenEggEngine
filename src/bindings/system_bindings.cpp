#include "gmThread.h"
#include "system_bindings.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include "../core/system_config.h"
#include "../core/engine_context.h"
#include "../core/strict_calls.h"
#include "../core/save_data.h"
#include "../core/file_helper.h"
#include "gamemonkeyscript_bindings.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <climits>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <zlib.h>

gmType g_fileType = -1;

namespace {

// 0x000aa2dc: 1-or-2-arg, returns bool). Real call site (ListCellEntity.gm
// :36: `if(!FILE_IsExists(imagePath+imageFile)){...fallback image...}`)
// gates a real fallback-image path, so returning the wrong answer here
// silently either always shows the fallback or always tries (and fails to
// load) the constructed path. The `area` 2nd arg (FILEHELPER_AREA_* enum,
// default 1) has no distinct meaning in this engine's single-rooted
// FileHelper (see file_helper.h) -- accepted and ignored, same as every
// other FILEHELPER_AREA_* param elsewhere in this codebase.
int GM_CDECL gmf_FILE_IsExists(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    bool exists = GetEngineContext().fileHelper && GetEngineContext().fileHelper->Resolve(path).has_value();
    a_thread->PushInt(exists ? 1 : 0);
    return GM_OK;
}

namespace fs = std::filesystem;

// ---------------------------------------------------------------------
// functions are registered as unqualified globals in s_systemLib below.
// Retail names/descriptors occur in the candidate mixed native-record run
// at 0x423300..0x423360; its registration caller is not yet traced. The
// corpus has live (non-commented-out) call sites for GetFileListing,
// VirtualSpaceLeft, DeleteFile, CopyFile, WriteString, LoadString, and
// EnsurePathExists (game_data/Platformer/{Main,LoginProcess,Instrument,
// FileSelectorProcess,DebugMenu,MainMenu5/EditScreenSidePanel}.gm), unlike
// FILE_IsExists above (File.c, a different native type) each of these
// takes a FILEHELPER_AREA_* 2nd/4th arg that real call sites actually vary
// (LoginProcess.gm explicitly sets `loginFileArea=FILEHELPER_AREA_SAVEDATA`;
// EditScreenSidePanel.gm's CopyFile calls always pass AREA_SAVEDATA to copy
// between myLevels save slots -- files that only exist under
// SaveData::Dir(), never in the read-only game_data tree), so unlike
// FILE_IsExists's existing "area ignored" stance, area is honored here.
// AREA_SAVEDATA(0) routes to SaveData::Dir(); AREA_GAMEDATA(1, the default
// whenever a real call site omits the arg) routes to FileHelper's primary
// root; AREA_HOST(2) is used as a literal, already-absolute host path (real
// call sites are dev-tool-only for this one); AREA_NULL(3) or anything else
// resolves to nothing, matching its name.
std::optional<std::string> FileHelperAreaRoot(int a_area) {
    switch (a_area) {
        case 0: return SaveData::Dir();
        case 1: {
            FileHelper* fh = GetEngineContext().fileHelper;
            if (fh && !fh->PrimaryRoot().empty()) return fh->PrimaryRoot();
            return std::nullopt;
        }
        default: return std::nullopt;
    }
}
bool FileHelperResolvePath(int a_area, const std::string& a_relPath, std::string& a_out) {
    // Desktop namespace translation. Retail FUN_0016f4e8 formats area 0 as
    // /savedata/<path> and areas 1/2 as /<path>; whether the PS3 path service
    // maps those namespaces to these host roots is separate from the bytecode
    // behavior. This AREA_HOST branch is verbatim, while retail prepends a
    // slash; an absolute input may normalize equivalently, but that behavior
    // is unverified. See
    if (a_area == 2) { a_out = a_relPath; return true; } // desktop AREA_HOST mapping
    if (a_area == 1) {
        FileHelper* fh = GetEngineContext().fileHelper;
        if (!fh) return false;
        if (fh->PrimaryRoot().empty()) return false;
        a_out = fh->PrimaryRoot() + "/" + a_relPath;
        return true;
    }
    auto root = FileHelperAreaRoot(a_area);
    if (!root) return false;
    a_out = *root + "/" + a_relPath;
    return true;
}
bool FileHelperAreaReadFile(int a_area, const std::string& a_relPath, std::string& a_out) {
    if (a_area == 1) {
        FileHelper* fh = GetEngineContext().fileHelper;
        return fh && fh->ReadTextFile(a_relPath, a_out);
    }
    std::string full;
    if (!FileHelperResolvePath(a_area, a_relPath, full)) return false;
    std::ifstream f(full, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss; ss << f.rdbuf();
    a_out = ss.str();
    return true;
}
// FILEHELPER_VirtualSpaceLeft() -- bytecode-confirmed constant 0x400
// (FUN_001702f0 @ 0x1702f0), returned as an integer by the native wrapper.
int GM_CDECL gmf_FILEHELPER_VirtualSpaceLeft(gmThread* a_thread) {
    a_thread->PushInt(0x400);
    return GM_OK;
}

// FILEHELPER_GetFileListing(dirPath, area=AREA_GAMEDATA) -- bytecode-
// Retail assigns entry strings under sequential integer keys and adds an
// integer `size` field; live scripts iterate to listing.size. Exact OS entry
// ordering and record filtering remain platform-dependent.
int GM_CDECL gmf_FILEHELPER_GetFileListing(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(dir, 0, "");
    int area = 1;
    if (a_thread->GetNumParams() > 1) { GM_INT_PARAM(a, 1, 1); area = a; }
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* table = m.AllocTableObject();
    std::string full;
    if (FileHelperResolvePath(area, dir, full)) {
        int i = 0;
        if (area == 1) {
            if (FileHelper* fh = GetEngineContext().fileHelper) {
                for (const auto& name : fh->ListFiles(dir))
                    table->Set(&m, i++, gmVariable(m.AllocStringObject(name.c_str())));
            }
        } else {
            std::error_code ec;
            for (auto& entry : fs::directory_iterator(full, ec)) {
                if (ec) break;
                table->Set(&m, i++, gmVariable(m.AllocStringObject(entry.path().filename().string().c_str())));
            }
        }
        table->Set(&m, "size", gmVariable((gmint)i));
    } else {
        table->Set(&m, "size", gmVariable((gmint)0));
    }
    a_thread->PushTable(table);
    return GM_OK;
}

// FILEHELPER_DeleteFile(path, area=AREA_GAMEDATA) -- bytecode-confirmed
// 1/2-arg, no script value returned. Retail only dispatches deletion through
// its SaveData service for area 0; all other areas are no-ops. The desktop
// SaveData delete approximates that service's file removal.
int GM_CDECL gmf_FILEHELPER_DeleteFile(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    int area = 1;
    if (a_thread->GetNumParams() > 1) { GM_INT_PARAM(a, 1, 1); area = a; }
    if (area == 0) SaveData::DeleteFile(path);
    return GM_OK;
}

// FILEHELPER_CopyFile(srcPath, srcArea, dstPath, dstArea) -- bytecode-
// confirmed 4-arg, no return value pushed. Retail opens rb/wb, queries source
// length, allocates min(length, 1 MiB), then asks fread/fwrite for one full
// chunk on every iteration and ignores both results. This preserves retail's
// rounded-up final chunk behavior for files larger than 1 MiB.
int GM_CDECL gmf_FILEHELPER_CopyFile(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(4);
    GM_STRING_PARAM(srcPath, 0, "");
    GM_INT_PARAM(srcArea, 1, 1);
    GM_STRING_PARAM(dstPath, 2, "");
    GM_INT_PARAM(dstArea, 3, 1);
    std::string srcFull, dstFull;
    if (srcArea == 1) {
        FileHelper* fh = GetEngineContext().fileHelper;
        auto resolved = fh ? fh->Resolve(srcPath) : std::nullopt;
        if (!resolved) return GM_OK;
        srcFull = *resolved;
    } else if (!FileHelperResolvePath(srcArea, srcPath, srcFull)) return GM_OK;
    if (!FileHelperResolvePath(dstArea, dstPath, dstFull)) return GM_OK;

    std::ifstream src(srcFull, std::ios::binary);
    if (!src) return GM_OK;
    std::ofstream dst(dstFull, std::ios::binary | std::ios::trunc);
    if (!dst) return GM_OK;

    src.seekg(0, std::ios::end);
    std::streamoff length = src.tellg();
    src.seekg(0, std::ios::beg);
    if (length <= 0) return GM_OK;

    // The retail helper stores the queried length in a 32-bit unsigned value,
    // then tests it as signed in the loop condition.
    uint32_t retailLength = static_cast<uint32_t>(length);
    size_t chunkSize = static_cast<size_t>(std::min<uint32_t>(retailLength, 0x100000));
    std::vector<char> buffer(chunkSize);
    uint32_t remaining = retailLength;
    while (static_cast<int32_t>(remaining) > 0) {
        // Retail requests one item of chunkSize bytes for both calls and does
        // not inspect fread's item count. A short final read therefore still
        // writes the entire buffer; its unwritten tail retains prior bytes.
        src.read(buffer.data(), static_cast<std::streamsize>(chunkSize));
        dst.write(buffer.data(), static_cast<std::streamsize>(chunkSize));
        remaining -= static_cast<uint32_t>(chunkSize);
    }
    return GM_OK;
}

// FILEHELPER_WriteString(path, content, area=AREA_GAMEDATA) and
// WriteCompressedString -- bytecode-confirmed bool return, but retail's
// FUN_00170148 is only a permission gate: nonzero areas log that file writing
// is not permitted and return false; area 0 returns true without writing.
// Both APIs use this gate, so do not create host files here.
int GM_CDECL gmf_FILEHELPER_WriteString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(path, 0, "");
    GM_STRING_PARAM(content, 1, "");
    int area = 1;
    if (a_thread->GetNumParams() > 2) { GM_INT_PARAM(a, 2, 1); area = a; }
    (void)path;
    (void)content;
    if (area != 0) {
        std::fprintf(stderr, "FILEHELPER[ln 457] file writing in retail is not permitted!\n");
    }
    a_thread->PushInt(area == 0 ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_FILEHELPER_WriteCompressedString(gmThread* a_thread) { return gmf_FILEHELPER_WriteString(a_thread); }

// FILEHELPER_LoadString(path, area=AREA_GAMEDATA) -- bytecode-confirmed raw
// read into a NUL-terminated temporary, then returned as a GM string.
int GM_CDECL gmf_FILEHELPER_LoadString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    int area = 1;
    if (a_thread->GetNumParams() > 1) { GM_INT_PARAM(a, 1, 1); area = a; }
    std::string content;
    FileHelperAreaReadFile(area, path, content);
    a_thread->PushNewString(content.c_str());
    return GM_OK;
}

// FILEHELPER_LoadCompressedString uses FUN_00010330, confirmed from PPC as
// zlib inflateInit_ / inflate(Z_FINISH) / inflateEnd. Retail caps decoded
// output at 0x4fffff bytes and returns it as a NUL-terminated GM string.
int GM_CDECL gmf_FILEHELPER_LoadCompressedString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    int area = 1;
    if (a_thread->GetNumParams() > 1) { GM_INT_PARAM(a, 1, 1); area = a; }

    std::string compressed;
    std::string decoded;
    if (FileHelperAreaReadFile(area, path, compressed) &&
        !compressed.empty() && compressed.size() <= UINT_MAX) {
        constexpr size_t kRetailOutputCapacity = 0x4fffff;
        std::vector<unsigned char> output(kRetailOutputCapacity);
        z_stream stream{};
        stream.next_in = reinterpret_cast<Bytef*>(compressed.data());
        stream.avail_in = static_cast<uInt>(compressed.size());
        stream.next_out = output.data();
        stream.avail_out = static_cast<uInt>(output.size());
        if (inflateInit_(&stream, ZLIB_VERSION, sizeof(stream)) == Z_OK) {
            int inflateResult = inflate(&stream, Z_FINISH);
            uLong decodedSize = stream.total_out;
            int endResult = inflateEnd(&stream);
            if (inflateResult == Z_STREAM_END && endResult == Z_OK &&
                decodedSize > 0 && decodedSize <= output.size()) {
                decoded.assign(reinterpret_cast<const char*>(output.data()),
                               static_cast<size_t>(decodedSize));
            }
        }
    }
    a_thread->PushNewString(decoded.c_str());
    return GM_OK;
}

// FILEHELPER_EnsurePathExists(path, area) -- bytecode-confirmed two-argument
// call. Its native wrapper converts the helper's zero return to true; the
// helper ignores cellFsMkdir results, so retail reports success regardless
// of individual directory-creation failures.
int GM_CDECL gmf_FILEHELPER_EnsurePathExists(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(path, 0, "");
    GM_INT_PARAM(area, 1, 1);
    std::string full;
    if (FileHelperResolvePath(area, path, full)) {
        fs::path parent = fs::path(full).parent_path();
        if (!parent.empty()) {
            std::error_code ec;
            fs::create_directories(parent, ec);
        }
    }
    a_thread->PushInt(1);
    return GM_OK;
}

// ---------------------------------------------------------------------
// constructor and 6 instance methods (openForWrite/openForRead/
// openForAppend/write/readAll/close), plus 3 bare-global convenience
// functions (FILE_IsExists -- already registered above, a different
// function despite the shared name; FILE_Write/FILE_Append). Was entirely
// unimplemented (not even a stub) -- real corpus call sites (Main.gm,
// Mixer.gm, User.gm, EntityMenuTopBaseOld.gm, PresetHelper.gm,
// SavePresetProcess.gm, SystemHelper.gm, all editor/dev-tool code writing
// generated .gm/preset/backup files back into the Platformer/ tree) always
// construct `local file=FILE();` then call `.openForWrite(filename)` --
// with FILE() unregistered, that call returned null via GM's null-call
// leniency, and `file.openForWrite(...)` on a null `file` likewise no-op'd
// to null, so `!file.openForWrite(filename)` was always true and every one
// of these editor save/export paths always hit its own "Can't open file!"
// failure branch and never wrote anything.
//
// Exact fopen mode strings and write format strings recovered directly
// from the retail EBOOT.elf at the DAT_ addresses Ghidra's decompile left
// opaque: openForWrite/FILE_Write use "w"; openForRead defaults to "r",
// or "rb" when explicitly called with a falsy 2nd arg; openForAppend/
// FILE_Append use "a"; write()/FILE_Write's format is bare "%s" (no
// trailing newline), while FILE_Append's format is "%s\n" (WITH a
// trailing newline) -- a real, confirmed asymmetry between write() and
// FILE_Append, not a guess.
//
// File has no FILEHELPER_AREA_* concept of its own (unlike FileHelper.c's
// functions) -- real call sites pass plain Platformer/-rooted paths and
// write straight into the game_data tree (an editor/dev-tool workflow, not
// a shipped-game save path), so this resolves relative to FileHelper's
// primary game_data root (same area FILEHELPER_AREA_GAMEDATA would use),
// falling back to the path as-is if no FileHelper root is registered.
// ---------------------------------------------------------------------

struct FileData {
    std::fstream stream;
    bool isOpen = false;
};

FileData* ThisFile(gmThread* a_thread) {
    return (FileData*)a_thread->ThisUserCheckType(g_fileType);
}

std::string FileResolvePath(const std::string& a_relPath) {
    if (!a_relPath.empty() && a_relPath[0] == '/') return a_relPath; // already absolute
    FileHelper* fh = GetEngineContext().fileHelper;
    if (fh && !fh->PrimaryRoot().empty()) return fh->PrimaryRoot() + "/" + a_relPath;
    return a_relPath;
}

int GM_CDECL gmf_File_ctor(gmThread* a_thread) {
    // arg0 (default 1), if present, isn't read by any of the 6 confirmed
    // instance methods below -- accepted for signature-completeness only.
    FileData* data = new FileData();
    gmUserObject* obj = a_thread->GetMachine()->AllocUserObject(data, g_fileType);
    a_thread->Push(gmVariable(obj));
    return GM_OK;
}

int GM_CDECL gmf_File_openForWrite(gmThread* a_thread) {
    FileData* f = ThisFile(a_thread); if (!f) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    std::string full = FileResolvePath(path);
    std::error_code ec;
    fs::create_directories(fs::path(full).parent_path(), ec);
    f->stream.close(); f->stream.clear();
    f->stream.open(full, std::ios::out | std::ios::trunc);
    f->isOpen = f->stream.is_open();
    a_thread->PushInt(f->isOpen ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_File_openForRead(gmThread* a_thread) {
    FileData* f = ThisFile(a_thread); if (!f) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    bool textMode = true;
    if (a_thread->GetNumParams() > 1) { GM_INT_PARAM(t, 1, 1); textMode = (t != 0); }
    std::string full = FileResolvePath(path);
    f->stream.close(); f->stream.clear();
    f->stream.open(full, textMode ? std::ios::in : (std::ios::in | std::ios::binary));
    f->isOpen = f->stream.is_open();
    a_thread->PushInt(f->isOpen ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_File_openForAppend(gmThread* a_thread) {
    FileData* f = ThisFile(a_thread); if (!f) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    std::string full = FileResolvePath(path);
    std::error_code ec;
    fs::create_directories(fs::path(full).parent_path(), ec);
    f->stream.close(); f->stream.clear();
    f->stream.open(full, std::ios::out | std::ios::app);
    f->isOpen = f->stream.is_open();
    a_thread->PushInt(f->isOpen ? 1 : 0);
    return GM_OK;
}
// write(str) -- confirmed real "%s" format (no newline appended).
int GM_CDECL gmf_File_write(gmThread* a_thread) {
    FileData* f = ThisFile(a_thread); if (!f) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(text, 0, "");
    if (!f->isOpen) { a_thread->PushInt(0); return GM_OK; }
    f->stream << text;
    a_thread->PushInt(1);
    return GM_OK;
}
// readAll() -- confirmed real: "" (not null) when no file is open.
int GM_CDECL gmf_File_readAll(gmThread* a_thread) {
    FileData* f = ThisFile(a_thread); if (!f) return GM_EXCEPTION;
    if (!f->isOpen) { a_thread->PushNewString(""); return GM_OK; }
    std::ostringstream ss; ss << f->stream.rdbuf();
    a_thread->PushNewString(ss.str().c_str());
    return GM_OK;
}
int GM_CDECL gmf_File_close(gmThread* a_thread) {
    FileData* f = ThisFile(a_thread); if (!f) return GM_EXCEPTION;
    if (f->isOpen) { f->stream.close(); f->isOpen = false; }
    return GM_OK;
}

gmFunctionEntry s_fileLib[] = {
    {"openForWrite", gmf_File_openForWrite, nullptr},
    {"openForRead", gmf_File_openForRead, nullptr},
    {"openForAppend", gmf_File_openForAppend, nullptr},
    {"write", gmf_File_write, nullptr},
    {"readAll", gmf_File_readAll, nullptr},
    {"close", gmf_File_close, nullptr},
};

void GM_CDECL FileGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    (void)a_machine;
    delete (FileData*)a_object->m_user;
}

// FILE_Write(path, content) / FILE_Append(path, content) -- confirmed real
// close in one call, bool return. Confirmed real format-string asymmetry
// (see header comment above): Write uses "%s" (no newline), Append uses
// "%s\n" (WITH a trailing newline).
int GM_CDECL gmf_FILE_Write(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(path, 0, "");
    GM_STRING_PARAM(content, 1, "");
    std::string full = FileResolvePath(path);
    std::error_code ec;
    fs::create_directories(fs::path(full).parent_path(), ec);
    std::ofstream f(full, std::ios::out | std::ios::trunc);
    bool ok = f.is_open();
    if (ok) { f << content; ok = f.good(); }
    a_thread->PushInt(ok ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_FILE_Append(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(path, 0, "");
    GM_STRING_PARAM(content, 1, "");
    std::string full = FileResolvePath(path);
    std::error_code ec;
    fs::create_directories(fs::path(full).parent_path(), ec);
    std::ofstream f(full, std::ios::out | std::ios::app);
    bool ok = f.is_open();
    if (ok) { f << content << "\n"; ok = f.good(); }
    a_thread->PushInt(ok ? 1 : 0);
    return GM_OK;
}

// 0x000a2a18: a plain global debug flag setter, bare-global call site
// Main.gm:1453 `SetIsOccludingUpdates(true);`). An occlusion-culling debug
// toggle with no consumer this reimplementation models -- genuine no-op,
// consumed rather than left as a null-call.
int GM_CDECL gmf_SetIsOccludingUpdates(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// ---------------------------------------------------------------------
// DumpObject are already implemented elsewhere (gamesystem_bindings.cpp).
// GameMonkey2JSON/JSON2GameMonkey now share the real JSON<->gmVariable
// mapping gamemonkeyscript_bindings.cpp already has internally
// (saveTableToJsonSaveRequest/loadTableFromJsonSaveRequest use the same
// code), exposed via JsonStringToGmVariable/GmVariableToJsonString
// (gamemonkeyscript_bindings.h) rather than duplicated here -- see
// GitHub issue #7.
// ---------------------------------------------------------------------

// @ 0x001afb58. Real usage (Platformer/Main.gm's level-save flow:
// SAVEDATA_AddString(filename, GameMonkey2JSON(jsonTable), ...)) always
// passes a table and uses the string result directly.
int GM_CDECL gmf_GameMonkey2JSON(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    std::string json = GmVariableToJsonString(*a_thread->GetMachine(), a_thread->Param(0));
    a_thread->PushNewString(json.c_str());
    return GM_OK;
}

// JSON2GameMonkey(jsonString[, extraTable]) -- confirmed real,
// checks for exactly 2 args with the second one type-tagged 0x26 (GM_TABLE)
// before treating it as a merge target -- a 1-arg call always allocates a
// fresh table.
int GM_CDECL gmf_JSON2GameMonkey(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(json, 0, "");
    const gmVariable* extra = nullptr;
    if (a_thread->GetNumParams() >= 2 && a_thread->Param(1).m_type == GM_TABLE) {
        extra = &a_thread->Param(1);
    }
    a_thread->Push(JsonStringToGmVariable(*a_thread->GetMachine(), json, extra));
    return GM_OK;
}

// BreakPoint(msg) -- confirmed real: retail's own decompiled body is
// `return 0;` with ZERO argument reads, a genuine unconditional no-op even
// on retail hardware (not something this reimplementation is choosing to
// skip) -- 10 real call sites all just expect it to do nothing observable.
int GM_CDECL gmf_BreakPoint(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
// Debug.assert()/myprint() -- confirmed real: both are `return 0;` with no
// body at all in retail (a stripped/disabled assert, matching real usage
// treating it as a statement with no observable effect).
int GM_CDECL gmf_assert(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_myprint(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
// Debug.c @ 0x1af584); real call site (ColourEditorProcess.gm:569) doesn't
// check a return value, so a debug log line satisfies the contract.
int GM_CDECL gmf_mybreak(gmThread* a_thread) {
    (void)a_thread;
    std::fprintf(stderr, "[mybreak]\n");
    return GM_OK;
}
// @ 0xa2978): prints msg (only if an arg was actually passed). No corpus
// call sites use it, but it's a trivial, safe stderr log.
int GM_CDECL gmf_DebugPrintf(gmThread* a_thread) {
    if (a_thread->GetNumParams() > 0) {
        GM_STRING_PARAM(msg, 0, "");
        std::fprintf(stderr, "[DebugPrintf] %s\n", msg);
    }
    return GM_OK;
}
int GM_CDECL gmf_SetDebuggingSomething(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_TraceCallStack(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_PrintTableStats(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_DumpTextures(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_DumpSounds(gmThread* a_thread) { (void)a_thread; return GM_OK; }

// Debug.GetTableSize(table)/GetTableActualUsageSize(table) -- confirmed
// two distinct internal fields off the real retail table object (a used-
// slot count and a separate "actual usage" byte count) that this engine's
// gmTableObject doesn't expose two different numbers for -- Count() (used
// key/value slots) is the only metric available, so both use it. Real call
// site (Main.gm:52-53) just sums both into running debug-stat totals, so
// reporting the same number for both is a safe approximation.
int GM_CDECL gmf_GetTableSize(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_CHECK_TABLE_PARAM(t, 0);
    a_thread->PushInt((gmint)t->Count());
    return GM_OK;
}
int GM_CDECL gmf_GetTableActualUsageSize(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_CHECK_TABLE_PARAM(t, 0);
    a_thread->PushInt((gmint)t->Count());
    return GM_OK;
}

// Debug.c @ 0xa2bc8): copies every key/value pair from each additional
// table arg into dest, later tables' keys overwriting earlier ones on
// collision. Throws if any 2nd+ arg isn't a table (confirmed: retail
// returns the exception sentinel on the first non-table).
int GM_CDECL gmf_MergeTables(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_CHECK_TABLE_PARAM(dest, 0);
    gmMachine& m = *a_thread->GetMachine();
    int n = a_thread->GetNumParams();
    for (int i = 1; i < n; ++i) {
        gmVariable v = a_thread->Param(i);
        if (v.m_type != GM_TABLE) { GM_EXCEPTION_MSG("MergeTables: expecting a table"); return GM_EXCEPTION; }
        gmTableObject* src = (gmTableObject*)m.GetObject(v.m_value.m_ref);
        gmTableIterator it;
        for (gmTableNode* node = src->GetFirst(it); node; node = src->GetNext(it)) {
            dest->Set(&m, node->m_key, node->m_value);
        }
    }
    return GM_OK;
}
// Debug.MergeTablesIntoFirstTakingMax(dest, ...tables) -- confirmed real
// on a key collision keeps whichever of the two values is numerically
// larger instead of blindly overwriting -- confirmed by its one real,
// live call site (CloudSaveHelper.gm:294's
// `MergeTablesIntoFirstTakingMax(SAVE_DATA, this.saveDataFromServer)`, a
// classic "keep the higher score/stat between local and cloud save"
// conflict resolution). Non-numeric values (or a key not already present
// in dest) are just set directly, matching MergeTables.
float AsNumber(const gmVariable& a_v, bool& a_ok) {
    if (a_v.m_type == GM_INT) { a_ok = true; return (float)a_v.m_value.m_int; }
    if (a_v.m_type == GM_FLOAT) { a_ok = true; return a_v.m_value.m_float; }
    a_ok = false;
    return 0.0f;
}
// ---------------------------------------------------------------------
// Memory.c), entirely unregistered. All are debug/profiling-only
// utilities with no gameplay-observable effect to model: PROFILE_MARKER_
// PUSH_PROCESS/POP are literally no-op stubs even in retail (empty
// decompiled bodies), PROFILER_WidgetCmd drives an on-screen profiler HUD
// this engine doesn't have, and the Memory_* functions probe/print real
// PS3 heap statistics this engine doesn't share (a different allocator on
// a different platform) -- safe no-ops / stderr logs, matching this
// codebase's existing stance on other unmodeled debug-only subsystems.
// ---------------------------------------------------------------------
int GM_CDECL gmf_PROFILER_WidgetCmd(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_PROFILE_MARKER_PUSH_PROCESS(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_PROFILE_MARKER_POP(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_Memory_StatsPrintAll(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_Memory_StatsPrintDelta(gmThread* a_thread) { (void)a_thread; return GM_OK; }
// Memory_ProbeLargestBlock/ProbeCapacity -- confirmed real int-returning
// (byte counts on retail's own heap); zero corpus call sites. A large,
// clearly-arbitrary constant matches this codebase's existing "always
// plenty of [resource] available" stance for other unmodeled capacity
// queries (SAVEDATA_getSystemFreeSpaceQuota/Media, FILEHELPER_
// VirtualSpaceLeft above).
int GM_CDECL gmf_Memory_ProbeLargestBlock(gmThread* a_thread) {
    (void)a_thread;
    a_thread->PushInt(0x7fffffff);
    return GM_OK;
}
int GM_CDECL gmf_Memory_ProbeCapacity(gmThread* a_thread) {
    a_thread->PushInt(0x7fffffff);
    return GM_OK;
}

int GM_CDECL gmf_MergeTablesIntoFirstTakingMax(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_CHECK_TABLE_PARAM(dest, 0);
    gmMachine& m = *a_thread->GetMachine();
    int n = a_thread->GetNumParams();
    for (int i = 1; i < n; ++i) {
        gmVariable v = a_thread->Param(i);
        if (v.m_type != GM_TABLE) { GM_EXCEPTION_MSG("MergeTablesIntoFirstTakingMax: expecting a table"); return GM_EXCEPTION; }
        gmTableObject* src = (gmTableObject*)m.GetObject(v.m_value.m_ref);
        gmTableIterator it;
        for (gmTableNode* node = src->GetFirst(it); node; node = src->GetNext(it)) {
            gmVariable existing = dest->Get(node->m_key);
            bool existingIsNum = false, newIsNum = false;
            float existingNum = AsNumber(existing, existingIsNum);
            float newNum = AsNumber(node->m_value, newIsNum);
            if (existing.m_type != GM_NULL && existingIsNum && newIsNum) {
                if (newNum > existingNum) dest->Set(&m, node->m_key, node->m_value);
            } else {
                dest->Set(&m, node->m_key, node->m_value);
            }
        }
    }
    return GM_OK;
}

int GM_CDECL gmf_SYSTEM_GetOSID(gmThread* a_thread) {
    a_thread->PushInt((gmint)SystemConfig::Get().osId);
    return GM_OK;
}

// Confirmed real short-code strings (NOT a human-readable description
// despite the name) -- every real call site does an EXACT string compare:
// PlayMenu.gm:44 `if (SYSTEM_GetOSDescription() == "NGP")`; Main.gm:1595-96
// `if (os=="NGP") {...} else if (os=="PS3") {...} else {...PC...}`
// (selects which TouchEditProcess/EntityMenuTop variant the editor loads);
// Main.gm:10258 `os != "NGP" && os != "PS3"`; Main.gm:10397-99
// `if (desc=="Win32") {...} else if (desc=="OS X") {...}`. The previous
// "PlayStation 3 (SCEA)"/"PlayStation Vita (NGP, SCEA)" strings never
// matched any of these comparisons, so Main.gm's `os=="PS3"` branch never
// fired on PS3 -- it silently fell all the way through to the PC branch
// (Platformer_TouchEditProcessPC/Platformer_EntityMenuTopPC), loading the
// wrong platform's editor input handler entirely. That's why the editor's
// PS3-specific enableShaking/enableBackPanel calls (TouchEditProcessPS3.gm)
// never ran, and why editor navigation/menu input generally didn't match
// this project's PS3 target: the PC variant expects a completely different
// (mouse/keyboard-driven) input scheme.
int GM_CDECL gmf_SYSTEM_GetOSDescription(gmThread* a_thread) {
    const char* desc = (SystemConfig::Get().osId == OsId::PS3) ? "PS3" : "NGP";
    a_thread->PushNewString(desc);
    return GM_OK;
}

int GM_CDECL gmf_SYSTEM_IsBeta(gmThread* a_thread) {
    a_thread->PushInt(0); // confirmed real decompiled body: always false, not config-driven
    return GM_OK;
}

int GM_CDECL gmf_SYSTEM_IsRetailBuild(gmThread* a_thread) {
    a_thread->PushInt(SystemConfig::Get().isRetailBuild ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_SYSTEM_GetLanguage(gmThread* a_thread) {
    a_thread->PushInt(SystemConfig::Get().languageCode);
    return GM_OK;
}

// classes/System.c @ 0xab58c): parses the string as a 64-bit integer
// (same "%lld" format as STRINGHELPER_ConvertMSStringToSecondInt, and
// same divide-by-1000 fixed-point constant -- so a server timestamp in
// milliseconds), null on a failed/empty parse (confirmed, same 0/-1
// sscanf-failure branch as every other STRINGHELPER_ConvertXxx-shaped
// function in this codebase), else a table -- confirmed return-slot tag
// 0x26 (table). Field names "day"/"month"/"year" are directly confirmed by
// the only real call site (CommunityLevelInfoPopupTableBase.gm:278/1035:
// `this.publishDate = SYSTEM_ServerTimeStampToDayMonthYear(publishDate);
// ... "%1/%2/%3", this.parent.publishDate.day, .month, .year`). The exact
// calendar-breakdown routine (FUN_00016b34) isn't independently
// decompiled, so this uses the standard UTC gmtime breakdown of the
// timestamp (in whole seconds after the confirmed /1000).
int GM_CDECL gmf_SYSTEM_ServerTimeStampToDayMonthYear(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(str, 0, "");
    char* end = nullptr;
    long long ms = std::strtoll(str, &end, 10);
    if (end == str) { a_thread->PushNull(); return GM_OK; }
    std::time_t seconds = (std::time_t)(ms / 1000);
    std::tm tmVal{};
#if defined(_WIN32)
    gmtime_s(&tmVal, &seconds);
#else
    gmtime_r(&seconds, &tmVal);
#endif
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* out = m.AllocTableObject();
    out->Set(&m, "day", gmVariable((gmint)tmVal.tm_mday));
    out->Set(&m, "month", gmVariable((gmint)(tmVal.tm_mon + 1)));
    out->Set(&m, "year", gmVariable((gmint)(tmVal.tm_year + 1900)));
    a_thread->PushTable(out);
    return GM_OK;
}

int GM_CDECL gmf_SYSTEM_getUNIXTimestamp(gmThread* a_thread) {
    // Confirmed real signature returns a STRING (type tag 0x25), not an int
    std::time_t t = std::time(nullptr);
    a_thread->PushNewString(std::to_string((long long)t).c_str());
    return GM_OK;
}

// SYSTEM_subtractUNIXTimestringFromServerTimestamp(unixSeconds, serverMs) --
// matched to retail wrapper 0x000ab3f0 and helper 0x002b69f4. Retail parses
// both strings with "%lld", returns null if either parse yields 0/-1, then
// returns the low 32 bits of unixSeconds - trunc(serverMs / 1000). The
// 0x20c49ba5e353f7cf multiplier in the PPC sequence is the fixed-point
// divide-by-1000; `system_subtract_timestamp_assembly_20261009.txt` records
// the operand order, truncation correction, and integer result tag.
int GM_CDECL gmf_SYSTEM_subtractUNIXTimestringFromServerTimestamp(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(unixText, 0, "");
    GM_STRING_PARAM(serverText, 1, "");
    long long unixSeconds = 0;
    long long serverMilliseconds = 0;
    if (std::sscanf(unixText, "%lld", &unixSeconds) != 1 ||
        std::sscanf(serverText, "%lld", &serverMilliseconds) != 1) {
        a_thread->PushNull();
        return GM_OK;
    }
    const std::uint32_t resultBits = static_cast<std::uint32_t>(unixSeconds) -
        static_cast<std::uint32_t>(serverMilliseconds / 1000);
    a_thread->PushInt(static_cast<gmint>(static_cast<std::int32_t>(resultBits)));
    return GM_OK;
}

int GM_CDECL gmf_SYSTEM_Execute(gmThread* a_thread) {
    // Confirmed real signature: takes a command string, matches the stock
    // gmfSystem()/C system() pattern. Deliberately NOT executed -- running
    // an arbitrary OS command from script content is a real command-
    // injection risk we don't need to take on for a script-compatibility
    // milestone. Logged instead so callers can see what would have run.
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(cmd, 0, "");
    std::fprintf(stderr, "[SYSTEM_Execute] suppressed (not executed): %s\n", cmd);
    return GM_OK;
}

// Real global `_(str)` localization function -- see core/localization.h.
int GM_CDECL gmf_Translate(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(text, 0, "");
    Localization* loc = GetEngineContext().localization;
    if (!loc) {
        a_thread->PushNewString(text);
        return GM_OK;
    }
    a_thread->PushNewString(loc->Translate(text).c_str());
    return GM_OK;
}

// Confirmed real by usage only (Platformer/MainMenu5/PreTutorialScreen.gm,
// HomeScreenBase.gm: `local path=Localization_LocalizeFilename("Platformer/
// images/MainMenu/logo.png"); this.logoTexture=Video.getTexture(path,...);`)
// only 2 real call sites. Was previously entirely unregistered, so
// calling it silently returned nothing via the null-call leniency patch,
// leaving `path` undefined and crashing the very next line's
// Video.getTexture(path,...).
//
// Confirmed real behavior from the actual game_data asset layout (not
// just usage): Platformer/images/MainMenu/ has one subdirectory per
// locale (en/, fr/, de/, ja/, ...) each containing a same-named
// preTutorial.png/logo.png/winTitle.png, matching
// Localization::LocaleDirForLanguageCode's folder names exactly -- so
// this inserts the current locale directory right before the filename.
// Localization.c @ 0x1aeacc). Only real call site (LoginPopup.gm:20) gates
// a PSN chat-restriction dialog behind it; that dialog is already a no-op
// in this engine (SAVEDATA_showOsMsgDialog, system_bindings.cpp), so the
// practical effect of this value is nil either way. This engine has no
// SCEE/territory concept at all (only OsId and a language code, see
// SystemConfig/Localization above) -- not SCEE by default, since inventing
// a region flag with no real basis to gate a dialog this engine doesn't
// render anyway isn't worth the false confidence.
int GM_CDECL gmf_Localization_IsSCEE(gmThread* a_thread) {
    a_thread->PushInt(0);
    return GM_OK;
}

int GM_CDECL gmf_Localization_LocalizeFilename(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    std::string p(path);
    std::string localeDir = Localization::LocaleDirForLanguageCode(SystemConfig::Get().languageCode);
    size_t slash = p.find_last_of('/');
    std::string localized = (slash == std::string::npos)
        ? (localeDir + "/" + p)
        : (p.substr(0, slash + 1) + localeDir + "/" + p.substr(slash + 1));
    a_thread->PushNewString(localized.c_str());
    return GM_OK;
}

// game_data/Main.gm's InitGame() (~line 1039-1061) uses the returned
// table, which fully determines the real contract: it reads
// `results.savedataResult`, compares it against SAVEDATARESULT_NOSAVE/
// NEWSAVE/NOTOC/OK/OK_TOCXML (stored ON the result table itself -- an
// unusual but confirmed-by-usage pattern, `results.SAVEDATARESULT_NOSAVE`
// not a bare global), and on SAVEDATARESULT_OK/NOTOC reads
// `results.saveFileTicket`/`.tocFileTicket` and passes them straight to
// GameMonkeyScript.loadTableFromJsonSaveRequest()/GetMyLevelsTOC().
//
// Main.gm's own comment right above this call ("we can assume the engine
// has already loaded the files for us so we can just jump directly into
// setting up the toc and save data") plus its dead `else` branch (manual
// SAVEDATA_reqGetFileSize/SAVEDATA_reqLoadFile preloading, commented
// "this should never happen anymore!") together confirm retail's real
// savedataResult is unconditionally >=0 by the time this game shipped --
// the async-preload fallback path is dead code, not something this needs
// to replicate.
//
// This engine's save data is real JSON files under SaveData::Dir() (see
// core/save_data.h -- "savedata/" next to the built executable, plain text,
// the splash screen's save-data handshake" becomes simply: check what's
// already on disk. Ticket design matches GameMonkeyScript.
// {save,load}TableFromJsonSaveRequest's (gamemonkeyscript_bindings.cpp): a
// ticket is just the relative filename, since no real call site does
// anything with one beyond handing it straight to a loader.
int GM_CDECL gmf_SPLASHSCREEN_FinalizeSplashScreenAndGetResults(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* results = m.AllocTableObject();
    results->Set(&m, "SAVEDATARESULT_NOSAVE", gmVariable((gmint)0));
    results->Set(&m, "SAVEDATARESULT_NEWSAVE", gmVariable((gmint)1));
    results->Set(&m, "SAVEDATARESULT_NOTOC", gmVariable((gmint)2));
    results->Set(&m, "SAVEDATARESULT_OK", gmVariable((gmint)3));
    results->Set(&m, "SAVEDATARESULT_OK_TOCXML", gmVariable((gmint)4));

    bool haveSave = SaveData::FileExists("save.json");
    bool haveToc = SaveData::FileExists("myLevels/toc.json");

    int savedataResult;
    gmVariable saveTicket = gmVariable::s_null;
    gmVariable tocTicket = gmVariable::s_null;
    if (!haveSave) {
        // First-ever boot (or GE3_CLEAR_SAVE just wiped it, see main.cpp) --
        // NEWSAVE takes the "start from scratch" branch in Main.gm, which
        // builds a fresh SAVE_DATA and writes save.json itself, so no ticket
        // is needed here.
        savedataResult = 1; // SAVEDATARESULT_NEWSAVE
    } else if (!haveToc) {
        savedataResult = 2; // SAVEDATARESULT_NOTOC
        saveTicket = gmVariable(m.AllocStringObject("save.json"));
    } else {
        savedataResult = 3; // SAVEDATARESULT_OK
        saveTicket = gmVariable(m.AllocStringObject("save.json"));
        tocTicket = gmVariable(m.AllocStringObject("myLevels/toc.json"));
    }

    results->Set(&m, "savedataResult", gmVariable((gmint)savedataResult));
    results->Set(&m, "saveFileTicket", saveTicket);
    results->Set(&m, "tocFileTicket", tocTicket);
    a_thread->PushTable(results);
    return GM_OK;
}

gmFunctionEntry s_systemLib[] = {
    {"SYSTEM_GetOSID", gmf_SYSTEM_GetOSID, nullptr},
    {"SYSTEM_GetOSDescription", gmf_SYSTEM_GetOSDescription, nullptr},
    {"SYSTEM_IsBeta", gmf_SYSTEM_IsBeta, nullptr},
    {"SYSTEM_IsRetailBuild", gmf_SYSTEM_IsRetailBuild, nullptr},
    {"SYSTEM_GetLanguage", gmf_SYSTEM_GetLanguage, nullptr},
    {"SYSTEM_getUNIXTimestamp", gmf_SYSTEM_getUNIXTimestamp, nullptr},
    {"SYSTEM_subtractUNIXTimestringFromServerTimestamp", gmf_SYSTEM_subtractUNIXTimestringFromServerTimestamp, nullptr},
    {"SYSTEM_ServerTimeStampToDayMonthYear", gmf_SYSTEM_ServerTimeStampToDayMonthYear, nullptr},
    {"SYSTEM_Execute", gmf_SYSTEM_Execute, nullptr},
    {"SPLASHSCREEN_FinalizeSplashScreenAndGetResults", gmf_SPLASHSCREEN_FinalizeSplashScreenAndGetResults, nullptr},
    {"_", gmf_Translate, nullptr},
    {"Localization_LocalizeFilename", gmf_Localization_LocalizeFilename, nullptr},
    {"Localization_IsSCEE", gmf_Localization_IsSCEE, nullptr},
    {"FILE_IsExists", gmf_FILE_IsExists, nullptr},
    {"SetIsOccludingUpdates", gmf_SetIsOccludingUpdates, nullptr},
    {"FILEHELPER_VirtualSpaceLeft", gmf_FILEHELPER_VirtualSpaceLeft, nullptr},
    {"FILEHELPER_GetFileListing", gmf_FILEHELPER_GetFileListing, nullptr},
    {"FILEHELPER_DeleteFile", gmf_FILEHELPER_DeleteFile, nullptr},
    {"FILEHELPER_CopyFile", gmf_FILEHELPER_CopyFile, nullptr},
    {"FILEHELPER_WriteString", gmf_FILEHELPER_WriteString, nullptr},
    {"FILEHELPER_WriteCompressedString", gmf_FILEHELPER_WriteCompressedString, nullptr},
    {"FILEHELPER_LoadString", gmf_FILEHELPER_LoadString, nullptr},
    {"FILEHELPER_LoadCompressedString", gmf_FILEHELPER_LoadCompressedString, nullptr},
    {"FILEHELPER_EnsurePathExists", gmf_FILEHELPER_EnsurePathExists, nullptr},
    {"FILE_Write", gmf_FILE_Write, nullptr},
    {"FILE_Append", gmf_FILE_Append, nullptr},
    {"BreakPoint", gmf_BreakPoint, nullptr},
    {"assert", gmf_assert, nullptr},
    {"myprint", gmf_myprint, nullptr},
    {"mybreak", gmf_mybreak, nullptr},
    {"DebugPrintf", gmf_DebugPrintf, nullptr},
    {"SetDebuggingSomething", gmf_SetDebuggingSomething, nullptr},
    {"TraceCallStack", gmf_TraceCallStack, nullptr},
    {"PrintTableStats", gmf_PrintTableStats, nullptr},
    {"DumpTextures", gmf_DumpTextures, nullptr},
    {"DumpSounds", gmf_DumpSounds, nullptr},
    {"GetTableSize", gmf_GetTableSize, nullptr},
    {"GetTableActualUsageSize", gmf_GetTableActualUsageSize, nullptr},
    {"MergeTables", gmf_MergeTables, nullptr},
    {"MergeTablesIntoFirstTakingMax", gmf_MergeTablesIntoFirstTakingMax, nullptr},
    {"PROFILER_WidgetCmd", gmf_PROFILER_WidgetCmd, nullptr},
    {"PROFILE_MARKER_PUSH_PROCESS", gmf_PROFILE_MARKER_PUSH_PROCESS, nullptr},
    {"PROFILE_MARKER_POP", gmf_PROFILE_MARKER_POP, nullptr},
    {"Memory_StatsPrintAll", gmf_Memory_StatsPrintAll, nullptr},
    {"Memory_StatsPrintDelta", gmf_Memory_StatsPrintDelta, nullptr},
    {"Memory_ProbeLargestBlock", gmf_Memory_ProbeLargestBlock, nullptr},
    {"Memory_ProbeCapacity", gmf_Memory_ProbeCapacity, nullptr},
    {"GameMonkey2JSON", gmf_GameMonkey2JSON, nullptr},
    {"JSON2GameMonkey", gmf_JSON2GameMonkey, nullptr},
};

} // namespace

void RegisterSystemBindings(gmMachine& a_machine) {
    // Confirmed real: SYSTEM_* are bare globals (SYSTEM_GetOSID(...), not
    // in Platformer/EntityMenuContents.gm. `_` is likewise a bare global,
    // confirmed by its 2500+ real call sites across game_data/*.gm.
    a_machine.RegisterLibrary(s_systemLib, sizeof(s_systemLib) / sizeof(s_systemLib[0]), nullptr);

    // OsId enum
    gmTableObject* g = a_machine.GetGlobals();
    g->Set(&a_machine, "OSID_WIN32", gmVariable((gmint)OsId::WIN32));
    g->Set(&a_machine, "OSID_OSX", gmVariable((gmint)OsId::OSX));
    g->Set(&a_machine, "OSID_PS3", gmVariable((gmint)OsId::PS3));
    g->Set(&a_machine, "OSID_PS4", gmVariable((gmint)OsId::PS4));
    g->Set(&a_machine, "OSID_NGP", gmVariable((gmint)OsId::NGP));
    g->Set(&a_machine, "OSID_VITATV", gmVariable((gmint)OsId::VITATV));

    // File native type (see header comment above s_fileLib).
    g_fileType = a_machine.CreateUserType("File");
    a_machine.RegisterTypeLibrary(g_fileType, s_fileLib, sizeof(s_fileLib) / sizeof(s_fileLib[0]));
    a_machine.RegisterUserCallbacks(g_fileType, nullptr, FileGCDestruct, nullptr);
    a_machine.RegisterLibraryFunction("FILE", gmf_File_ctor);
}
