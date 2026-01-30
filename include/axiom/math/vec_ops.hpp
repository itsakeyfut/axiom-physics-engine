#pragma once

#include "axiom/math/vec2.hpp"
#include "axiom/math/vec3.hpp"
#include "axiom/math/vec4.hpp"

#include <algorithm>
#include <cmath>

namespace axiom {
namespace math {

// ============================================================================
// Dot Product
// ============================================================================

/**
 * @brief Calculate dot product of two 2D vectors
 * @param a First vector
 * @param b Second vector
 * @return Scalar dot product
 */
constexpr float dot(const Vec2& a, const Vec2& b) noexcept {
    return a.x * b.x + a.y * b.y;
}

// Note: dot for Vec3 and Vec4 are already defined in their respective headers

// ============================================================================
// Cross Product
// ============================================================================

// Note: cross for Vec3 is already defined in vec3.hpp

// ============================================================================
// Length Functions
// ============================================================================

/**
 * @brief Calculate squared length of a 2D vector
 * @param v Vector
 * @return Length squared
 */
constexpr float lengthSquared(const Vec2& v) noexcept {
    return v.x * v.x + v.y * v.y;
}

/**
 * @brief Calculate length of a 2D vector
 * @param v Vector
 * @return Length (magnitude)
 */
inline float length(const Vec2& v) noexcept {
    return std::sqrt(lengthSquared(v));
}

/**
 * @brief Calculate squared length of a 3D vector
 * @param v Vector
 * @return Length squared
 */
constexpr float lengthSquared(const Vec3& v) noexcept {
    return v.lengthSquared();
}

/**
 * @brief Calculate length of a 3D vector
 * @param v Vector
 * @return Length (magnitude)
 */
inline float length(const Vec3& v) noexcept {
    return v.length();
}

/**
 * @brief Calculate squared length of a 4D vector
 * @param v Vector
 * @return Length squared
 */
constexpr float lengthSquared(const Vec4& v) noexcept {
    return v.lengthSquared();
}

/**
 * @brief Calculate length of a 4D vector
 * @param v Vector
 * @return Length (magnitude)
 */
inline float length(const Vec4& v) noexcept {
    return v.length();
}

// ============================================================================
// Distance Functions
// ============================================================================

/**
 * @brief Calculate distance between two 2D points
 * @param a First point
 * @param b Second point
 * @return Distance
 */
inline float distance(const Vec2& a, const Vec2& b) noexcept {
    return length(b - a);
}

/**
 * @brief Calculate squared distance between two 2D points
 * @param a First point
 * @param b Second point
 * @return Squared distance
 */
constexpr float distanceSquared(const Vec2& a, const Vec2& b) noexcept {
    return lengthSquared(b - a);
}

/**
 * @brief Calculate distance between two 3D points
 * @param a First point
 * @param b Second point
 * @return Distance
 */
inline float distance(const Vec3& a, const Vec3& b) noexcept {
    return length(b - a);
}

/**
 * @brief Calculate squared distance between two 3D points
 * @param a First point
 * @param b Second point
 * @return Squared distance
 */
constexpr float distanceSquared(const Vec3& a, const Vec3& b) noexcept {
    return lengthSquared(b - a);
}

/**
 * @brief Calculate distance between two 4D points
 * @param a First point
 * @param b Second point
 * @return Distance
 */
inline float distance(const Vec4& a, const Vec4& b) noexcept {
    return length(b - a);
}

/**
 * @brief Calculate squared distance between two 4D points
 * @param a First point
 * @param b Second point
 * @return Squared distance
 */
constexpr float distanceSquared(const Vec4& a, const Vec4& b) noexcept {
    return lengthSquared(b - a);
}

// ============================================================================
// Normalization Functions
// ============================================================================

/**
 * @brief Normalize a 2D vector
 * @param v Vector to normalize
 * @return Normalized vector (unit vector)
 */
inline Vec2 normalize(const Vec2& v) noexcept {
    const float len = length(v);
    if (len > 0.0f) {
        return v / len;
    }
    return Vec2::zero();
}

/**
 * @brief Safely normalize a 2D vector with fallback
 * @param v Vector to normalize
 * @param fallback Fallback vector if v is zero
 * @return Normalized vector or fallback
 */
inline Vec2 safeNormalize(const Vec2& v, const Vec2& fallback = Vec2::unitX()) noexcept {
    const float len = length(v);
    if (len > 0.0f) {
        return v / len;
    }
    return fallback;
}

/**
 * @brief Normalize a 3D vector
 * @param v Vector to normalize
 * @return Normalized vector (unit vector)
 */
inline Vec3 normalize(const Vec3& v) noexcept {
    return v.normalized();
}

/**
 * @brief Safely normalize a 3D vector with fallback
 * @param v Vector to normalize
 * @param fallback Fallback vector if v is zero
 * @return Normalized vector or fallback
 */
inline Vec3 safeNormalize(const Vec3& v, const Vec3& fallback = Vec3::unitX()) noexcept {
    const float len = length(v);
    if (len > 0.0f) {
        return v / len;
    }
    return fallback;
}

/**
 * @brief Normalize a 4D vector
 * @param v Vector to normalize
 * @return Normalized vector (unit vector)
 */
inline Vec4 normalize(const Vec4& v) noexcept {
    return v.normalized();
}

/**
 * @brief Safely normalize a 4D vector with fallback
 * @param v Vector to normalize
 * @param fallback Fallback vector if v is zero
 * @return Normalized vector or fallback
 */
inline Vec4 safeNormalize(const Vec4& v, const Vec4& fallback = Vec4::unitX()) noexcept {
    const float len = length(v);
    if (len > 0.0f) {
        return v / len;
    }
    return fallback;
}

// ============================================================================
// Reflection and Refraction
// ============================================================================

/**
 * @brief Reflect a vector around a normal
 * @param v Incident vector
 * @param n Normal vector (should be normalized)
 * @return Reflected vector
 */
inline Vec2 reflect(const Vec2& v, const Vec2& n) noexcept {
    return v - 2.0f * dot(v, n) * n;
}

/**
 * @brief Reflect a vector around a normal
 * @param v Incident vector
 * @param n Normal vector (should be normalized)
 * @return Reflected vector
 */
inline Vec3 reflect(const Vec3& v, const Vec3& n) noexcept {
    return v - 2.0f * dot(v, n) * n;
}

/**
 * @brief Reflect a vector around a normal
 * @param v Incident vector
 * @param n Normal vector (should be normalized)
 * @return Reflected vector
 */
inline Vec4 reflect(const Vec4& v, const Vec4& n) noexcept {
    return v - 2.0f * dot(v, n) * n;
}

/**
 * @brief Refract a vector through a surface
 * @param v Incident vector (should be normalized)
 * @param n Normal vector (should be normalized)
 * @param eta Ratio of indices of refraction
 * @return Refracted vector or zero if total internal reflection
 */
inline Vec2 refract(const Vec2& v, const Vec2& n, float eta) noexcept {
    const float dotNV = dot(n, v);
    const float k = 1.0f - eta * eta * (1.0f - dotNV * dotNV);
    if (k < 0.0f) {
        return Vec2::zero();  // Total internal reflection
    }
    return eta * v - (eta * dotNV + std::sqrt(k)) * n;
}

/**
 * @brief Refract a vector through a surface
 * @param v Incident vector (should be normalized)
 * @param n Normal vector (should be normalized)
 * @param eta Ratio of indices of refraction
 * @return Refracted vector or zero if total internal reflection
 */
inline Vec3 refract(const Vec3& v, const Vec3& n, float eta) noexcept {
    const float dotNV = dot(n, v);
    const float k = 1.0f - eta * eta * (1.0f - dotNV * dotNV);
    if (k < 0.0f) {
        return Vec3::zero();  // Total internal reflection
    }
    return eta * v - (eta * dotNV + std::sqrt(k)) * n;
}

/**
 * @brief Refract a vector through a surface
 * @param v Incident vector (should be normalized)
 * @param n Normal vector (should be normalized)
 * @param eta Ratio of indices of refraction
 * @return Refracted vector or zero if total internal reflection
 */
inline Vec4 refract(const Vec4& v, const Vec4& n, float eta) noexcept {
    const float dotNV = dot(n, v);
    const float k = 1.0f - eta * eta * (1.0f - dotNV * dotNV);
    if (k < 0.0f) {
        return Vec4::zero();  // Total internal reflection
    }
    return eta * v - (eta * dotNV + std::sqrt(k)) * n;
}

// ============================================================================
// Linear Interpolation
// ============================================================================

/**
 * @brief Linear interpolation between two 2D vectors
 * @param a Start vector
 * @param b End vector
 * @param t Interpolation factor [0, 1]
 * @return Interpolated vector
 */
constexpr Vec2 lerp(const Vec2& a, const Vec2& b, float t) noexcept {
    return a + (b - a) * t;
}

/**
 * @brief Linear interpolation between two 3D vectors
 * @param a Start vector
 * @param b End vector
 * @param t Interpolation factor [0, 1]
 * @return Interpolated vector
 */
constexpr Vec3 lerp(const Vec3& a, const Vec3& b, float t) noexcept {
    return a + (b - a) * t;
}

/**
 * @brief Linear interpolation between two 4D vectors
 * @param a Start vector
 * @param b End vector
 * @param t Interpolation factor [0, 1]
 * @return Interpolated vector
 */
constexpr Vec4 lerp(const Vec4& a, const Vec4& b, float t) noexcept {
    return a + (b - a) * t;
}

// ============================================================================
// Component-wise Min/Max/Clamp
// ============================================================================

/**
 * @brief Component-wise minimum of two 2D vectors
 */
constexpr Vec2 min(const Vec2& a, const Vec2& b) noexcept {
    return Vec2((a.x < b.x) ? a.x : b.x, (a.y < b.y) ? a.y : b.y);
}

/**
 * @brief Component-wise maximum of two 2D vectors
 */
constexpr Vec2 max(const Vec2& a, const Vec2& b) noexcept {
    return Vec2((a.x > b.x) ? a.x : b.x, (a.y > b.y) ? a.y : b.y);
}

/**
 * @brief Component-wise clamp of a 2D vector
 */
constexpr Vec2 clamp(const Vec2& v, const Vec2& minVec, const Vec2& maxVec) noexcept {
    return min(max(v, minVec), maxVec);
}

/**
 * @brief Component-wise minimum of two 3D vectors
 */
constexpr Vec3 min(const Vec3& a, const Vec3& b) noexcept {
    return Vec3((a.x < b.x) ? a.x : b.x, (a.y < b.y) ? a.y : b.y, (a.z < b.z) ? a.z : b.z);
}

/**
 * @brief Component-wise maximum of two 3D vectors
 */
constexpr Vec3 max(const Vec3& a, const Vec3& b) noexcept {
    return Vec3((a.x > b.x) ? a.x : b.x, (a.y > b.y) ? a.y : b.y, (a.z > b.z) ? a.z : b.z);
}

/**
 * @brief Component-wise clamp of a 3D vector
 */
constexpr Vec3 clamp(const Vec3& v, const Vec3& minVec, const Vec3& maxVec) noexcept {
    return min(max(v, minVec), maxVec);
}

/**
 * @brief Component-wise minimum of two 4D vectors
 */
constexpr Vec4 min(const Vec4& a, const Vec4& b) noexcept {
    return Vec4((a.x < b.x) ? a.x : b.x, (a.y < b.y) ? a.y : b.y, (a.z < b.z) ? a.z : b.z,
                (a.w < b.w) ? a.w : b.w);
}

/**
 * @brief Component-wise maximum of two 4D vectors
 */
constexpr Vec4 max(const Vec4& a, const Vec4& b) noexcept {
    return Vec4((a.x > b.x) ? a.x : b.x, (a.y > b.y) ? a.y : b.y, (a.z > b.z) ? a.z : b.z,
                (a.w > b.w) ? a.w : b.w);
}

/**
 * @brief Component-wise clamp of a 4D vector
 */
constexpr Vec4 clamp(const Vec4& v, const Vec4& minVec, const Vec4& maxVec) noexcept {
    return min(max(v, minVec), maxVec);
}

// ============================================================================
// Component-wise Math Functions
// ============================================================================

/**
 * @brief Component-wise absolute value of a 2D vector
 */
inline Vec2 abs(const Vec2& v) noexcept {
    return Vec2(std::abs(v.x), std::abs(v.y));
}

/**
 * @brief Component-wise floor of a 2D vector
 */
inline Vec2 floor(const Vec2& v) noexcept {
    return Vec2(std::floor(v.x), std::floor(v.y));
}

/**
 * @brief Component-wise ceiling of a 2D vector
 */
inline Vec2 ceil(const Vec2& v) noexcept {
    return Vec2(std::ceil(v.x), std::ceil(v.y));
}

/**
 * @brief Component-wise round of a 2D vector
 */
inline Vec2 round(const Vec2& v) noexcept {
    return Vec2(std::round(v.x), std::round(v.y));
}

/**
 * @brief Component-wise absolute value of a 3D vector
 */
inline Vec3 abs(const Vec3& v) noexcept {
    return Vec3(std::abs(v.x), std::abs(v.y), std::abs(v.z));
}

/**
 * @brief Component-wise floor of a 3D vector
 */
inline Vec3 floor(const Vec3& v) noexcept {
    return Vec3(std::floor(v.x), std::floor(v.y), std::floor(v.z));
}

/**
 * @brief Component-wise ceiling of a 3D vector
 */
inline Vec3 ceil(const Vec3& v) noexcept {
    return Vec3(std::ceil(v.x), std::ceil(v.y), std::ceil(v.z));
}

/**
 * @brief Component-wise round of a 3D vector
 */
inline Vec3 round(const Vec3& v) noexcept {
    return Vec3(std::round(v.x), std::round(v.y), std::round(v.z));
}

/**
 * @brief Component-wise absolute value of a 4D vector
 */
inline Vec4 abs(const Vec4& v) noexcept {
    return Vec4(std::abs(v.x), std::abs(v.y), std::abs(v.z), std::abs(v.w));
}

/**
 * @brief Component-wise floor of a 4D vector
 */
inline Vec4 floor(const Vec4& v) noexcept {
    return Vec4(std::floor(v.x), std::floor(v.y), std::floor(v.z), std::floor(v.w));
}

/**
 * @brief Component-wise ceiling of a 4D vector
 */
inline Vec4 ceil(const Vec4& v) noexcept {
    return Vec4(std::ceil(v.x), std::ceil(v.y), std::ceil(v.z), std::ceil(v.w));
}

/**
 * @brief Component-wise round of a 4D vector
 */
inline Vec4 round(const Vec4& v) noexcept {
    return Vec4(std::round(v.x), std::round(v.y), std::round(v.z), std::round(v.w));
}

}  // namespace math
}  // namespace axiom
