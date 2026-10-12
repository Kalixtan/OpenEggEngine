#pragma once
#include <cstdint>
#include <string>
#include <utility>
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
    // Palette-indexed BCI pixels (4-bit) and indexed PNG pixels (8-bit
    // container, palette indices 0-15 in the shipped animation corpus) use
    // the Animation palette bank at draw time. Index 0 is transparent.
    std::vector<uint8_t> indices;
    // Non-indexed PNG pixels are stored as straight RGBA, row-major. These
    // are uploaded directly and do not use the generated animation palette.
    std::vector<uint8_t> rgba;
};

struct FsaAsset {
    std::vector<BciFrame> frames;
    // FUN_000b6920 stores custom (image index, normalized weight) entries on
    // the selected record in the shared retail FSA resource. Keep that state
    // here so Animation instances sharing this cached asset see the same
    // sequence, while their playback cursor and record duration stay local.
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
