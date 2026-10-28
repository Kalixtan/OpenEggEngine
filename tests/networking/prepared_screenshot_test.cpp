#include "../../src/render/prepared_screenshot.h"
#include <cassert>

int main() {
    constexpr int width = 9, height = 10;
    std::vector<std::uint8_t> rgba(width * height * 4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto offset = (y * width + x) * 4;
            rgba[offset] = static_cast<std::uint8_t>(x);
            rgba[offset + 1] = static_cast<std::uint8_t>(y);
            rgba[offset + 2] = 73;
            rgba[offset + 3] = 255;
        }
    }
    PreparedScreenshot screenshot;
    screenshot.Sample(rgba.data(), width, height);
    assert(screenshot.width == 2 && screenshot.height == 2);
    assert((screenshot.pixels == std::vector<std::uint8_t>{0, 9, 73, 4, 9, 73,
                                                           0, 5, 73, 4, 5, 73}));
    screenshot.Sample(rgba.data(), 3, 3);
    assert(screenshot.width == 0 && screenshot.height == 0 && screenshot.pixels.empty());
    screenshot.Sample(rgba.data(), width, height);
    screenshot.Clear();
    assert(screenshot.width == 0 && screenshot.height == 0 && screenshot.pixels.empty());
}
