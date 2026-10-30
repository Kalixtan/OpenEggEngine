#include "system_config.h"

#include <fstream>
#include <iterator>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) return 2;

    SystemConfig& config = SystemConfig::Get();
    const std::string path = argv[1];
    if (config.LoadFromIni(path)) return 3;

    const std::string mode = argv[2];
    if (mode == "unreadable")
        return config.SaveReplacementSession("session-id", "user-id") ? 4 : 0;

    if (mode != "missing" || !config.SaveReplacementSession("session-id", "user-id"))
        return 5;

    std::ifstream input(path, std::ios::binary);
    const std::string contents(std::istreambuf_iterator<char>(input), {});
    return contents.find("server_session_id = session-id") != std::string::npos &&
           contents.find("server_user_id = user-id") != std::string::npos ? 0 : 6;
}
