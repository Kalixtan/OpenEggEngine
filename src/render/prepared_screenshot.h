#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

struct PreparedScreenshot {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;

    void Clear() {
        std::vector<std::uint8_t>().swap(pixels);
        width = height = 0;
    }

    // OpenGL rows run bottom to top; retail samples from the top row downward.
    void Sample(const std::uint8_t* rgba, int sourceWidth, int sourceHeight) {
        Clear();
        if (!rgba || sourceWidth < 4 || sourceHeight < 4) return;
        width = sourceWidth / 4;
        height = sourceHeight / 4;
        pixels.resize(static_cast<std::size_t>(width) * height * 3);
        for (int y = 0; y < height; ++y) {
            const int sourceY = sourceHeight - 1 - y * 4;
            for (int x = 0; x < width; ++x) {
                const auto source = (static_cast<std::size_t>(sourceY) * sourceWidth + x * 4) * 4;
                const auto target = (static_cast<std::size_t>(y) * width + x) * 3;
                pixels[target] = rgba[source];
                pixels[target + 1] = rgba[source + 1];
                pixels[target + 2] = rgba[source + 2];
            }
        }
    }
};

inline PreparedScreenshot& GetPreparedScreenshot() {
    static PreparedScreenshot screenshot;
    return screenshot;
}
