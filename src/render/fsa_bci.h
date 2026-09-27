#pragma once
#include <cstdint>
#include <string>
#include <vector>

// .fsa/.bci sprite-animation format -- container + pixel decode fully
// Every real animated entity in the corpus uses this (2738 .fsa files,
// 31818 embedded .bci frames, tested to decode cleanly). The open question
// documented there -- whether the 4-bit index is a fixed palette or an
// intensity mask tinted at runtime -- is resolved in practice by
// SpawnPoint.gm's actual usage (COLOURSCHEME_SetAnimationPaletteColour(anim,
// outColour, fillColour) called right before every draw): visually
// inspecting a decoded frame (spawnRecordPlayerIdle0001.bci) shows a
// dominant single-value body (index 1, the record-player "cup") plus a
// handful of much rarer higher-index pixels tracing the tonearm/feet
// detail lines -- exactly the shape you'd expect from a 2-colour
// fill+outline mask, not an arbitrary fixed palette.

struct BciFrame {
    int width = 0;
    int height = 0;
    // width*height entries, one 4-bit palette index (0-15) per pixel,
    // row-major. 0 is confirmed transparent background.
    std::vector<uint8_t> indices;
};

struct FsaAsset {
    std::vector<BciFrame> frames;

    // Parses the whole container and decodes every embedded .bci frame in
    // directory order (confirmed real: entries are already named/ordered
    // sequentially, e.g. spawnRecordPlayerIdle0001.bci..0015.bci). Returns
    // false (frames left empty) if the file can't be read or isn't a valid
    // FSA container -- callers treat a frameless asset as "nothing to draw"
    // rather than crashing, same leniency as every other asset-load gap in
    // this engine.
    bool LoadFromFile(const std::string& a_path);
};
