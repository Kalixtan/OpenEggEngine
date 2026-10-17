#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>


struct BciFrame {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> indices;
    // Non-indexed PNG pixels are stored as straight RGBA, row-major. These
    // are uploaded directly and do not use the generated animation palette.
    std::vector<uint8_t> rgba;
};

struct FsaAsset {
    std::vector<BciFrame> frames;
    std::vector<std::pair<int, float>> customSequence;

    bool LoadFromFile(const std::string& a_path);
};
