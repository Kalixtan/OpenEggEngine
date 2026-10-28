#include "network_level_data.h"
#include "gmMachine.h"
#include "gmTableObject.h"
#include "tinyxml2.h"
#include "rapidjson/document.h"
#include <zlib.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace {
// Limit temporary output until the retail buffer size has been confirmed.
constexpr size_t kMaxLevelBytes = 64 * 1024 * 1024;

bool Inflate(const std::string& input, std::string& output) {
    z_stream stream{};
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
    stream.avail_in = static_cast<uInt>(std::min<size_t>(input.size(), UINT32_MAX));
    if (input.size() > UINT32_MAX || inflateInit(&stream) != Z_OK) return false;
    int status = Z_OK;
    char buffer[16384];
    while (status == Z_OK) {
        stream.next_out = reinterpret_cast<Bytef*>(buffer);
        stream.avail_out = sizeof(buffer);
        status = inflate(&stream, Z_NO_FLUSH);
        const size_t produced = sizeof(buffer) - stream.avail_out;
        if (produced > kMaxLevelBytes - output.size()) { status = Z_MEM_ERROR; break; }
        output.append(buffer, produced);
    }
    inflateEnd(&stream);
    return status == Z_STREAM_END;
}

gmVariable JsonValue(gmMachine& machine, const rapidjson::Value& value) {
    if (value.IsNull()) return gmVariable::s_null;
    if (value.IsBool()) return gmVariable(static_cast<gmint>(value.GetBool()));
    if (value.IsInt()) return gmVariable(static_cast<gmint>(value.GetInt()));
    if (value.IsNumber()) return gmVariable(static_cast<gmfloat>(value.GetDouble()));
    if (value.IsString()) return gmVariable(machine.AllocStringObject(value.GetString(), value.GetStringLength()));
    gmTableObject* table = machine.AllocTableObject();
    if (value.IsArray()) {
        int index = 0;
        for (const auto& item : value.GetArray()) table->Set(&machine, index++, JsonValue(machine, item));
    } else if (value.IsObject()) {
        for (auto member = value.MemberBegin(); member != value.MemberEnd(); ++member) {
            gmVariable key(machine.AllocStringObject(member->name.GetString(), member->name.GetStringLength()));
            table->Set(&machine, key, JsonValue(machine, member->value));
        }
    }
    return gmVariable(table);
}

gmTableObject* XmlChildren(gmMachine& machine, const tinyxml2::XMLNode& parent) {
    gmTableObject* table = machine.AllocTableObject();
    int nextIndex = 0;
    for (auto* node = parent.FirstChildElement(); node; node = node->NextSiblingElement()) {
        const char* name = node->Name();
        if (std::strcmp(name, "indexSize") == 0) {
            table->Set(&machine, "size", gmVariable(static_cast<gmint>(nextIndex)));
            continue;
        }
        if (std::strcmp(name, "element") != 0 && std::strcmp(name, "tableElement") != 0) continue;
        const char* keyText = node->Attribute("key");
        gmVariable key;
        int keyInt = 0;
        if (!keyText) key = gmVariable(static_cast<gmint>(nextIndex++));
        else if (std::sscanf(keyText, "%d", &keyInt) == 1) {
            key = gmVariable(static_cast<gmint>(keyInt));
            if (keyInt >= nextIndex && keyInt < INT32_MAX) nextIndex = keyInt + 1;
        } else key = gmVariable(machine.AllocStringObject(keyText));
        gmVariable value = gmVariable::s_null;
        if (std::strcmp(name, "tableElement") == 0) value = gmVariable(XmlChildren(machine, *node));
        else if (const char* text = node->Attribute("value")) {
            int integer = 0;
            double real = 0;
            const bool isInt = std::sscanf(text, "%d", &integer) == 1;
            const bool isReal = std::sscanf(text, "%lf", &real) == 1;
            if (isInt && (!isReal || static_cast<double>(integer) == static_cast<double>(static_cast<float>(real))))
                value = gmVariable(static_cast<gmint>(integer));
            else if (isReal) value = gmVariable(static_cast<gmfloat>(real));
            else value = gmVariable(machine.AllocStringObject(text));
        }
        table->Set(&machine, key, value);
    }
    return table;
}
}

gmVariable ParseNetworkLevelData(gmMachine& machine, const std::string& compressed) {
    std::string text;
    if (!Inflate(compressed, text) || text.empty()) return gmVariable::s_null;
    if (text.front() == '<') {
        tinyxml2::XMLDocument document;
        if (document.Parse(text.data(), text.size()) != tinyxml2::XML_SUCCESS) return gmVariable::s_null;
        return gmVariable(XmlChildren(machine, document));
    }
    rapidjson::Document document;
    document.Parse(text.data(), text.size());
    if (document.HasParseError()) return gmVariable::s_null;
    gmVariable result = JsonValue(machine, document);
    if (result.m_type == GM_TABLE) {
        auto* root = static_cast<gmTableObject*>(machine.GetObject(result.m_value.m_ref));
        for (const char* name : {"screenData", "entities"}) {
            const gmVariable child = root->Get(&machine, name);
            if (child.m_type == GM_TABLE) {
                auto* table = static_cast<gmTableObject*>(machine.GetObject(child.m_value.m_ref));
                table->Set(&machine, "size", gmVariable(static_cast<gmint>(table->Count())));
            }
        }
    }
    return result;
}
