#pragma once

#include <cmath>
#include <cstddef>

namespace axiom::math {

/**
 * @brief 4D vector class with x, y, z, and w components
 *
 * Represents a 4-dimensional vector with floating-point components.
 * Commonly used for homogeneous coordinates in graphics and physics.
 * Supports standard vector operations including arithmetic, comparison,
 * and utility functions.
 */
class Vec4 {
public:
    float x;
    float y;
    float z;
    float w;

    // Constructors

    /**
     * @brief Default constructor - initializes to zero vector
     */
    constexpr Vec4() noexcept : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}

    /**
     * @brief Construct from x, y, z, and w components
     * @param inX X component
     * @param inY Y component
     * @param inZ Z component
     * @param inW W component
     */
    constexpr Vec4(float inX, float inY, float inZ, float inW) noexcept
        : x(inX), y(inY), z(inZ), w(inW) {}

    /**
     * @brief Construct with same value for all components
     * @param scalar Value for x, y, z, and w
     */
    explicit constexpr Vec4(float scalar) noexcept : x(scalar), y(scalar), z(scalar), w(scalar) {}

    // Copy and move constructors/assignment (default)
    Vec4(const Vec4&) noexcept = default;
    Vec4(Vec4&&) noexcept = default;
    Vec4& operator=(const Vec4&) noexcept = default;
    Vec4& operator=(Vec4&&) noexcept = default;
    ~Vec4() = default;

    // Accessors

    /**
     * @brief Array-style access to components
     * @param index 0 for x, 1 for y, 2 for z, 3 for w
     * @return Reference to component
     */
    float& operator[](size_t index) noexcept { return (&x)[index]; }

    /**
     * @brief Const array-style access to components
     * @param index 0 for x, 1 for y, 2 for z, 3 for w
     * @return Const reference to component
     */
    const float& operator[](size_t index) const noexcept { return (&x)[index]; }

    // Arithmetic operators

    /**
     * @brief Vector addition
     */
    constexpr Vec4 operator+(const Vec4& other) const noexcept {
        return Vec4(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    /**
     * @brief Vector subtraction
     */
    constexpr Vec4 operator-(const Vec4& other) const noexcept {
        return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    /**
     * @brief Component-wise multiplication
     */
    constexpr Vec4 operator*(const Vec4& other) const noexcept {
        return Vec4(x * other.x, y * other.y, z * other.z, w * other.w);
    }

    /**
     * @brief Component-wise division
     */
    constexpr Vec4 operator/(const Vec4& other) const noexcept {
        return Vec4(x / other.x, y / other.y, z / other.z, w / other.w);
    }

    /**
     * @brief Scalar multiplication
     */
    constexpr Vec4 operator*(float scalar) const noexcept {
        return Vec4(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    /**
     * @brief Scalar division
     */
    constexpr Vec4 operator/(float scalar) const noexcept {
        return Vec4(x / scalar, y / scalar, z / scalar, w / scalar);
    }

    /**
     * @brief Unary negation
     */
    constexpr Vec4 operator-() const noexcept { return Vec4(-x, -y, -z, -w); }

    // Compound assignment operators

    Vec4& operator+=(const Vec4& other) noexcept {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    Vec4& operator-=(const Vec4& other) noexcept {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    Vec4& operator*=(const Vec4& other) noexcept {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;
        return *this;
    }

    Vec4& operator/=(const Vec4& other) noexcept {
        x /= other.x;
        y /= other.y;
        z /= other.z;
        w /= other.w;
        return *this;
    }

    Vec4& operator*=(float scalar) noexcept {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
    }

    Vec4& operator/=(float scalar) noexcept {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;
        return *this;
    }

    // Comparison operators

    /**
     * @brief Exact equality comparison
     */
    constexpr bool operator==(const Vec4& other) const noexcept {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    /**
     * @brief Inequality comparison
     */
    constexpr bool operator!=(const Vec4& other) const noexcept { return !(*this == other); }

    // Vector operations

    /**
     * @brief Dot product with another vector
     * @param other The other vector
     * @return Scalar dot product
     */
    constexpr float dot(const Vec4& other) const noexcept {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }

    /**
     * @brief Calculate squared length (magnitude squared)
     * @return Length squared
     */
    constexpr float lengthSquared() const noexcept { return x * x + y * y + z * z + w * w; }

    /**
     * @brief Calculate length (magnitude)
     * @return Length
     */
    float length() const noexcept { return std::sqrt(lengthSquared()); }

    /**
     * @brief Normalize the vector (return unit vector)
     * @return Normalized vector
     */
    Vec4 normalized() const noexcept {
        const float len = length();
        if (len > 0.0f) {
            return *this / len;
        }
        return Vec4::zero();
    }

    /**
     * @brief Normalize this vector in-place
     * @return Reference to this vector
     */
    Vec4& normalize() noexcept {
        const float len = length();
        if (len > 0.0f) {
            *this /= len;
        }
        return *this;
    }

    // Static utility functions

    /**
     * @brief Zero vector (0, 0, 0, 0)
     */
    static constexpr Vec4 zero() noexcept { return Vec4(0.0f, 0.0f, 0.0f, 0.0f); }

    /**
     * @brief One vector (1, 1, 1, 1)
     */
    static constexpr Vec4 one() noexcept { return Vec4(1.0f, 1.0f, 1.0f, 1.0f); }

    /**
     * @brief Unit X vector (1, 0, 0, 0)
     */
    static constexpr Vec4 unitX() noexcept { return Vec4(1.0f, 0.0f, 0.0f, 0.0f); }

    /**
     * @brief Unit Y vector (0, 1, 0, 0)
     */
    static constexpr Vec4 unitY() noexcept { return Vec4(0.0f, 1.0f, 0.0f, 0.0f); }

    /**
     * @brief Unit Z vector (0, 0, 1, 0)
     */
    static constexpr Vec4 unitZ() noexcept { return Vec4(0.0f, 0.0f, 1.0f, 0.0f); }

    /**
     * @brief Unit W vector (0, 0, 0, 1)
     */
    static constexpr Vec4 unitW() noexcept { return Vec4(0.0f, 0.0f, 0.0f, 1.0f); }
};

// Free functions for scalar * vector

/**
 * @brief Scalar multiplication (scalar * vector)
 */
constexpr Vec4 operator*(float scalar, const Vec4& vec) noexcept {
    return vec * scalar;
}

/**
 * @brief Dot product (free function)
 */
constexpr float dot(const Vec4& a, const Vec4& b) noexcept {
    return a.dot(b);
}

}  // namespace axiom::math
