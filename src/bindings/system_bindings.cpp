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

int GM_CDECL gmf_FILE_IsExists(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    bool exists = GetEngineContext().fileHelper && GetEngineContext().fileHelper->Resolve(path).has_value();
    a_thread->PushInt(exists ? 1 : 0);
    return GM_OK;
}

namespace fs = std::filesystem;

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
    std::ifstream f(std::filesystem::u8path(full), std::ios::binary);
    if (!f) return false;
    std::ostringstream ss; ss << f.rdbuf();
    a_out = ss.str();
    return true;
}
int GM_CDECL gmf_FILEHELPER_VirtualSpaceLeft(gmThread* a_thread) {
    a_thread->PushInt(0x400);
    return GM_OK;
}

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
            for (auto& entry : fs::directory_iterator(fs::u8path(full), ec)) {
                if (ec) break;
                const std::string name = entry.path().filename().u8string();
                table->Set(&m, i++, gmVariable(m.AllocStringObject(name.c_str())));
            }
        }
        table->Set(&m, "size", gmVariable((gmint)i));
    } else {
        table->Set(&m, "size", gmVariable((gmint)0));
    }
    a_thread->PushTable(table);
    return GM_OK;
}

int GM_CDECL gmf_FILEHELPER_DeleteFile(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    int area = 1;
    if (a_thread->GetNumParams() > 1) { GM_INT_PARAM(a, 1, 1); area = a; }
    if (area == 0) SaveData::DeleteFile(path);
    return GM_OK;
}

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

    std::ifstream src(std::filesystem::u8path(srcFull), std::ios::binary);
    if (!src) return GM_OK;
    std::ofstream dst(std::filesystem::u8path(dstFull), std::ios::binary | std::ios::trunc);
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

int GM_CDECL gmf_FILEHELPER_EnsurePathExists(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(path, 0, "");
    GM_INT_PARAM(area, 1, 1);
    std::string full;
    if (FileHelperResolvePath(area, path, full)) {
        fs::path parent = fs::u8path(full).parent_path();
        if (!parent.empty()) {
            std::error_code ec;
            fs::create_directories(parent, ec);
        }
    }
    a_thread->PushInt(1);
    return GM_OK;
}


struct FileData {
    std::fstream stream;
    bool isOpen = false;
};

FileData* ThisFile(gmThread* a_thread) {
    return (FileData*)a_thread->ThisUserCheckType(g_fileType);
}

std::string FileResolvePath(const std::string& a_relPath) {
    if (fs::u8path(a_relPath).is_absolute()) return a_relPath;
    FileHelper* fh = GetEngineContext().fileHelper;
    if (fh && !fh->PrimaryRoot().empty()) return fh->PrimaryRoot() + "/" + a_relPath;
    return a_relPath;
}

int GM_CDECL gmf_File_ctor(gmThread* a_thread) {
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
    fs::create_directories(fs::u8path(full).parent_path(), ec);
    f->stream.close(); f->stream.clear();
    f->stream.open(fs::u8path(full), std::ios::out | std::ios::trunc);
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
    f->stream.open(fs::u8path(full), textMode ? std::ios::in : (std::ios::in | std::ios::binary));
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
    fs::create_directories(fs::u8path(full).parent_path(), ec);
    f->stream.close(); f->stream.clear();
    f->stream.open(fs::u8path(full), std::ios::out | std::ios::app);
    f->isOpen = f->stream.is_open();
    a_thread->PushInt(f->isOpen ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_File_write(gmThread* a_thread) {
    FileData* f = ThisFile(a_thread); if (!f) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(text, 0, "");
    if (!f->isOpen) { a_thread->PushInt(0); return GM_OK; }
    f->stream << text;
    a_thread->PushInt(1);
    return GM_OK;
}
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

int GM_CDECL gmf_FILE_Write(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(path, 0, "");
    GM_STRING_PARAM(content, 1, "");
    std::string full = FileResolvePath(path);
    std::error_code ec;
    fs::create_directories(fs::u8path(full).parent_path(), ec);
    std::ofstream f(std::filesystem::u8path(full), std::ios::out | std::ios::trunc);
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
    fs::create_directories(fs::u8path(full).parent_path(), ec);
    std::ofstream f(std::filesystem::u8path(full), std::ios::out | std::ios::app);
    bool ok = f.is_open();
    if (ok) { f << content << "\n"; ok = f.good(); }
    a_thread->PushInt(ok ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_SetIsOccludingUpdates(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}


int GM_CDECL gmf_GameMonkey2JSON(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    std::string json = GmVariableToJsonString(*a_thread->GetMachine(), a_thread->Param(0));
    a_thread->PushNewString(json.c_str());
    return GM_OK;
}

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

int GM_CDECL gmf_BreakPoint(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_assert(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_myprint(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_mybreak(gmThread* a_thread) {
    (void)a_thread;
    std::fprintf(stderr, "[mybreak]\n");
    return GM_OK;
}
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
float AsNumber(const gmVariable& a_v, bool& a_ok) {
    if (a_v.m_type == GM_INT) { a_ok = true; return (float)a_v.m_value.m_int; }
    if (a_v.m_type == GM_FLOAT) { a_ok = true; return a_v.m_value.m_float; }
    a_ok = false;
    return 0.0f;
}
int GM_CDECL gmf_PROFILER_WidgetCmd(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_PROFILE_MARKER_PUSH_PROCESS(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_PROFILE_MARKER_POP(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_Memory_StatsPrintAll(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_Memory_StatsPrintDelta(gmThread* a_thread) { (void)a_thread; return GM_OK; }
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

int GM_CDECL gmf_SYSTEM_GetOSDescription(gmThread* a_thread) {
    // The scripts compare these short names directly (notably "Win32" and
    // "OS X" for desktop helpers, plus "PS3" and "NGP" in platform UI code).
    const char* desc = "NGP";
    switch (SystemConfig::Get().osId) {
        case OsId::WINDOWS: desc = "Win32"; break;
        case OsId::OSX: desc = "OS X"; break;
        case OsId::PS3: desc = "PS3"; break;
        case OsId::PS4: desc = "PS4"; break;
        case OsId::NGP: desc = "NGP"; break;
        case OsId::VITATV: desc = "Vita TV"; break;
    }
    a_thread->PushNewString(desc);
    return GM_OK;
}

int GM_CDECL gmf_SYSTEM_IsBeta(gmThread* a_thread) {
    a_thread->PushInt(0);
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
    std::time_t t = std::time(nullptr);
    a_thread->PushNewString(std::to_string((long long)t).c_str());
    return GM_OK;
}

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
    a_machine.RegisterLibrary(s_systemLib, sizeof(s_systemLib) / sizeof(s_systemLib[0]), nullptr);

    // OsId enum
    gmTableObject* g = a_machine.GetGlobals();
    g->Set(&a_machine, "OSID_WIN32", gmVariable((gmint)OsId::WINDOWS));
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
