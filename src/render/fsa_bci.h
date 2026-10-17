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

    // Parses the whole container and decodes embedded .bci and .png frames
    // in their FSA directory order, matching the retail loader traversal.
    // Returns
    // false (frames left empty) if the file can't be read or isn't a valid
    // FSA container -- callers treat a frameless asset as "nothing to draw"
    // rather than crashing, same leniency as every other asset-load gap in
    // this engine.
    bool LoadFromFile(const std::string& a_path);
};
