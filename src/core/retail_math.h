#pragma once

#include <cmath>
#include <cstdint>

namespace RetailMath {

// PowerPC fctiwz truncates in-range values, saturates overflow, and maps NaN
// to the integer-indefinite word.
inline int32_t Fctiwz(float value) {
    constexpr float kIntMin = -2147483648.0f;
    constexpr float kIntMaxExclusive = 2147483648.0f;
    if (std::isnan(value) || value < kIntMin) return INT32_MIN;
    if (value >= kIntMaxExclusive) return INT32_MAX;
    return static_cast<int32_t>(value);
}

} // namespace RetailMath
