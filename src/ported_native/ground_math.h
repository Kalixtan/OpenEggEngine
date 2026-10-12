#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

namespace GroundRenderMath {

// Verification: the ground PPC helpers form x*x first, then use fmadds(y, y,
// x*x), followed by fsqrts (for example 0x196fc4..0x196fd4 and
// 0x197a90..0x197aa8). Keeping that fused operand order matters at binary32
// precision; fma(x, x, y*y) is not equivalent for the recovered sequence.
inline float RetailGroundLength(float a_x, float a_y) {
    const float xSquared = a_x * a_x;
    const float squaredLength = std::fma(a_y, a_y, xSquared);
    return std::sqrt(squaredLength);
}

inline float FloatFromBits(uint32_t a_bits) {
    float value;
    std::memcpy(&value, &a_bits, sizeof(value));
    return value;
}

inline uint32_t FloatBits(float a_value) {
    uint32_t bits;
    std::memcpy(&bits, &a_value, sizeof(bits));
    return bits;
}

// Verification: the large-angle path in retail FUN_001dd1ac calls
// FUN_001dbf8c, whose normal-finite reduction loop is traced in Ghidra. It
// scales the divisor by the difference in binary exponents, backs off until
// the scaled divisor fits, subtracts once in float, and repeats. Ground
// angles and 2*pi are ordinary finite float values, so this covers their
// reachable reduction path without host fmod's different rounding sequence.
inline float RetailGroundRemainder(float a_value, float a_divisor) {
    if (!std::isfinite(a_value) || !std::isfinite(a_divisor) ||
        a_divisor == 0.0f || std::fpclassify(a_value) == FP_SUBNORMAL ||
        std::fpclassify(a_divisor) == FP_SUBNORMAL) {
        return std::fmod(a_value, a_divisor);
    }
    if (std::fabs(a_value) < std::fabs(a_divisor)) return a_value;

    const bool negative = std::signbit(a_value);
    float remainder = std::fabs(a_value);
    const float divisor = std::fabs(a_divisor);
    int divisorExponent = 0;
    std::frexp(divisor, &divisorExponent);
    while (remainder >= divisor) {
        int remainderExponent = 0;
        std::frexp(remainder, &remainderExponent);
        int scale = remainderExponent - divisorExponent;
        float scaledDivisor = std::scalbn(divisor, scale);
        if (scaledDivisor > remainder) {
            --scale;
            scaledDivisor = std::scalbn(divisor, scale);
        }
        remainder = static_cast<float>(
            static_cast<double>(remainder) - static_cast<double>(scaledDivisor));
    }
    return negative ? -remainder : remainder;
}

// Verification: ordinary finite-positive MATH_POW (`0x1dc1c0`) is traced
// through PPC instructions and constants in
// ground domain only: 0 <= base <= 1 and 0 <= exponent <= 1. Intermediate
// float operations round to binary32; the reduced exponential's rational
// expression is binary64 before `frsp`, matching the PPC sequence. Values
// outside this domain still use the host library and are not parity-verified.
inline float RetailGroundPow(float a_base, float a_exponent) {
    if (!(a_base >= 0.0f && a_base <= 1.0f &&
          a_exponent >= 0.0f && a_exponent <= 1.0f) ||
        !std::isfinite(a_base) || !std::isfinite(a_exponent)) {
        return std::pow(a_base, a_exponent);
    }
    if (a_exponent == 0.0f) return 1.0f;
    if (a_base == 0.0f) return 0.0f;
    if (a_base == 1.0f || a_exponent == 1.0f) return a_base;

    const float kSqrtHalf = FloatFromBits(0x3f3504f3u);
    const float kLn2 = FloatFromBits(0x3f317218u);
    const float kLn2High = FloatFromBits(0x3f317216u);
    const float kLn2Low = FloatFromBits(0xb3fbe8e8u);
    const float kLog2E = FloatFromBits(0x3fb8aa3bu);

    int baseExponent = 0;
    float significand = static_cast<float>(std::frexp(a_base, &baseExponent));
    // Retail's frexp-style helper yields m in [0.5, 1); this normalization
    // keeps the logarithm's argument in [sqrt(0.5), 1).
    if (significand < kSqrtHalf) {
        significand = significand + significand;
        --baseExponent;
    }

    // The exponent enters MATH_POW as a widened GM_FLOAT. PPC `fmuls`
    // multiplies that double input by the f32 base exponent and rounds once.
    const float exponentProduct = static_cast<float>(
        static_cast<double>(a_exponent) * static_cast<double>(
            static_cast<float>(baseExponent)));
    const int integerScale = static_cast<int>(exponentProduct); // PPC fctiwz
    float logResidual;
    if (integerScale == 0) {
        logResidual = exponentProduct * kLn2;
    } else {
        // FUN_001dcc10(..., 16) clears the low `134 - exponentField` bits
        // of the positive f32 exponent, producing the high part used by the
        // compensated product in 0x1dc1c0.
        const uint32_t exponentBits = FloatBits(a_exponent);
        const int exponentField = static_cast<int>((exponentBits >> 23) & 0xffu);
        const int splitBits = 134 - exponentField;
        float exponentHigh = a_exponent;
        if (splitBits > 23) {
            // FUN_001dcc10 clears both fraction words and preserves only the
            // sign bit in this branch. Ground callers provide a nonnegative
            // exponent, so the retained high part is positive zero.
            exponentHigh = 0.0f;
        } else if (splitBits > 0) {
            exponentHigh = FloatFromBits(exponentBits &
                ~((uint32_t{1} << splitBits) - 1u));
        }
        const float scaleF = static_cast<float>(baseExponent);
        const float integerScaleF = static_cast<float>(integerScale);
        const float error = std::fma(scaleF, exponentHigh, -integerScaleF);
        const float exponentLow = static_cast<float>(
            static_cast<double>(a_exponent) - static_cast<double>(exponentHigh));
        const float compensatedProduct = std::fma(scaleF, exponentLow, error);
        logResidual = compensatedProduct * kLn2;
    }

    // GE3_Internal_MathLogCore(0x1dcfb0), mode 0. Its operations are PPC
    // single-precision divisions, multiplies, and fused multiply-adds.
    float logSignificand = 0.0f;
    if (significand != 1.0f) {
        const float numerator = significand - 1.0f;
        const float z = numerator / (significand + 1.0f);
        const float z2 = z * z;
        float polynomial = std::fma(z2, FloatFromBits(0x3e990215u),
                                    FloatFromBits(0x3eccae11u));
        polynomial = std::fma(z2, polynomial, FloatFromBits(0x3f2aaac0u));
        const float logCorrection = z * (numerator - z2 * polynomial);
        const float exponentF = static_cast<float>(baseExponent);
        const float splitCorrection = std::fma(exponentF, kLn2Low,
                                               logCorrection);
        logSignificand = std::fma(exponentF, kLn2High,
                                  numerator - splitCorrection);
    }
    logResidual = std::fma(logSignificand, a_exponent, logResidual);

    // GE3_Internal_MathExpReduceScale(0x1dce40): nearest-integer reduction
    // uses sign-dependent +/-0.5 followed by PPC truncation toward zero.
    const float scaledResidual = logResidual * kLog2E;
    const float roundingBias = scaledResidual < 0.0f ? -0.5f : 0.5f;
    const int expScale = static_cast<int>(scaledResidual + roundingBias);
    float reduced = std::fma(static_cast<float>(expScale), -kLn2High,
                             logResidual);
    reduced = std::fma(static_cast<float>(expScale), -kLn2Low, reduced);

    float result = 1.0f;
    const float kExpTiny = FloatFromBits(0x33000000u); // 2^-25
    if (reduced <= -kExpTiny || reduced >= kExpTiny) {
        const float square = reduced * reduced;
        const float numeratorTerm = reduced * std::fma(
            square, FloatFromBits(0x3c085413u), 0.5f);
        const float denominatorTerm = square * FloatFromBits(0x3dccbf9eu);
        const double numerator = static_cast<double>(denominatorTerm) +
                                 static_cast<double>(numeratorTerm) + 1.0;
        const double denominator = static_cast<double>(denominatorTerm) -
                                   static_cast<double>(numeratorTerm) + 1.0;
        result = static_cast<float>(numerator / denominator);
    }

    // FUN_002b75a0 changes the binary exponent directly. Its denormal path
    // right-shifts the implicit leading bit without rounding, so ldexp() is
    // not equivalent for subnormal outputs.
    uint32_t resultBits = FloatBits(result);
    const int resultExponent = static_cast<int>((resultBits >> 23) & 0xffu);
    const int adjustedExponent = resultExponent + expScale + integerScale;
    if (adjustedExponent >= 255) return FloatFromBits(0x7f800000u);
    if (adjustedExponent >= 1) {
        resultBits = (resultBits & 0x807fffffu) |
                     (static_cast<uint32_t>(adjustedExponent) << 23);
        return FloatFromBits(resultBits);
    }
    const int rightShift = 1 - adjustedExponent;
    if (rightShift >= 24) return 0.0f;
    const uint32_t significandBits = (resultBits & 0x7fffffu) | 0x800000u;
    return FloatFromBits(significandBits >> rightShift);
}

// Verification: retail FUN_001dd1ac's PPC range reduction, quadrant selection,
// and degree-7 sine/cosine polynomials are instruction-traced. This shared
// float implementation is used by both ground vertex construction and the
// entity-facing ground fill helper.
inline float RetailGroundSinCos(float a_angle, bool a_cosine) {
    constexpr float kInvHalfPi = 0.6366197466850281f;
    constexpr float kTau = 6.2831854820251465f;
    constexpr float kQuarterPiReductionHi = -1.570796251296997f;
    constexpr float kQuarterPiReductionLo = -7.549790126404332e-8f;
    constexpr float kHalf = 0.5f;
    constexpr float kSmallAngle = 0.000244140625f;
    constexpr float kCosC0 = -0.0013602249091491103f;
    constexpr float kCosC1 = 0.04165669530630112f;
    constexpr float kCosC2 = 0.4999990165233612f;
    constexpr float kSinC0 = -0.0001950727018993348f;
    constexpr float kSinC1 = 0.00833207555115223f;
    constexpr float kSinC2 = 0.16666652262210846f;

    float angle = a_angle;
    if (angle < -3.0679686069488525f || angle > 3.0679686069488525f)
        angle = RetailGroundRemainder(angle, kTau);
    float quadrantFloat = static_cast<float>(angle * kInvHalfPi);
    quadrantFloat = quadrantFloat <= kHalf
        ? quadrantFloat - kHalf : quadrantFloat + kHalf;
    const int quadrant = static_cast<int>(quadrantFloat);
    const float quadrantAsFloat = static_cast<float>(quadrant);
    float reduced = std::fma(quadrantAsFloat, kQuarterPiReductionHi, angle);
    reduced = std::fma(quadrantAsFloat, kQuarterPiReductionLo, reduced);
    const int selector = (quadrant & 3) + (a_cosine ? 1 : 0);

    float result = reduced;
    if (reduced <= -kSmallAngle || reduced >= kSmallAngle) {
        const float square = reduced * reduced;
        if ((selector & 1) != 0) {
            float polynomial = std::fma(square, kCosC0, kCosC1);
            polynomial = std::fma(square, polynomial, -kCosC2);
            result = std::fma(square, polynomial, 1.0f);
        } else {
            float polynomial = std::fma(square, kSinC0, kSinC1);
            polynomial = std::fma(square, polynomial, -kSinC2);
            result = std::fma(reduced * square, polynomial, reduced);
        }
    } else if ((selector & 1) != 0) {
        result = 1.0f;
    }
    return (selector & 2) != 0 ? -result : result;
}

} // namespace GroundRenderMath
