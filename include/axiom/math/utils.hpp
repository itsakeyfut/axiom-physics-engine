#pragma once

#include "axiom/math/constants.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace axiom::math {

// ============================================================================
// Angle Conversion
// ============================================================================

/**
 * @brief Convert degrees to radians
 * @param degrees Angle in degrees
 * @return Angle in radians
 */
inline constexpr float radians(float degrees) noexcept {
    return degrees * DEG_TO_RAD<float>;
}

/**
 * @brief Convert radians to degrees
 * @param radians Angle in radians
 * @return Angle in degrees
 */
inline constexpr float degrees(float radians) noexcept {
    return radians * RAD_TO_DEG<float>;
}

// ============================================================================
// Scalar Clamping and Interpolation
// ============================================================================

/**
 * @brief Clamp a value between a minimum and maximum
 * @param value Value to clamp
 * @param min Minimum bound
 * @param max Maximum bound
 * @return Clamped value in range [min, max]
 */
inline constexpr float clamp(float value, float min, float max) noexcept {
    return (value < min) ? min : (value > max) ? max : value;
}

/**
 * @brief Linear interpolation between two values
 * @param a Start value
 * @param b End value
 * @param t Interpolation factor [0, 1]
 * @return Interpolated value
 */
inline constexpr float lerp(float a, float b, float t) noexcept {
    return a + (b - a) * t;
}

/**
 * @brief Smooth Hermite interpolation between 0 and 1
 * @param edge0 Lower edge value
 * @param edge1 Upper edge value
 * @param x Value to interpolate
 * @return Smoothly interpolated value between 0 and 1
 * @note Returns 0 if x <= edge0, 1 if x >= edge1, smooth transition otherwise
 */
inline constexpr float smoothstep(float edge0, float edge1, float x) noexcept {
    // Clamp x to [0, 1] range
    const float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    // Hermite interpolation: 3t^2 - 2t^3
    return t * t * (3.0f - 2.0f * t);
}

// ============================================================================
// Sign and Comparison
// ============================================================================

/**
 * @brief Get the sign of a value
 * @param x Input value
 * @return -1 if x < 0, +1 if x > 0, 0 if x == 0
 */
inline constexpr float sign(float x) noexcept {
    return (x > 0.0f) ? 1.0f : (x < 0.0f) ? -1.0f : 0.0f;
}

/**
 * @brief Check if two floating-point values are approximately equal
 * @param a First value
 * @param b Second value
 * @param epsilon Maximum allowed difference
 * @return True if |a - b| <= epsilon
 */
inline bool almostEqual(float a, float b, float epsilon = EPSILON_F) noexcept {
    return std::abs(a - b) <= epsilon;
}

// ============================================================================
// Power of Two Utilities
// ============================================================================

/**
 * @brief Check if a number is a power of two
 * @param n Number to check
 * @return True if n is a power of two (1, 2, 4, 8, 16, ...)
 * @note Returns false for n == 0
 */
inline constexpr bool isPowerOfTwo(uint32_t n) noexcept {
    return (n != 0) && ((n & (n - 1)) == 0);
}

/**
 * @brief Find the next power of two greater than or equal to n
 * @param n Input number
 * @return Next power of two >= n
 * @note Returns n if n is already a power of two
 * @note Returns 0 if n == 0
 */
inline uint32_t nextPowerOfTwo(uint32_t n) noexcept {
    if (n == 0) {
        return 0;
    }

    // If already power of two, return as-is
    if (isPowerOfTwo(n)) {
        return n;
    }

    // Subtract 1 and fill all bits below the highest set bit
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;

    return n;
}

}  // namespace axiom::math
