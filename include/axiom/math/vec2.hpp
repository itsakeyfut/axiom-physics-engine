#pragma once

#include <cmath>
#include <cstddef>

namespace axiom {
namespace math {

/**
 * @brief 2D vector class with x and y components
 *
 * Represents a 2-dimensional vector with floating-point components.
 * Supports standard vector operations including arithmetic, comparison,
 * and utility functions.
 */
class Vec2 {
public:
    float x;
    float y;

    // Constructors

    /**
     * @brief Default constructor - initializes to zero vector
     */
    constexpr Vec2() noexcept : x(0.0f), y(0.0f) {}

    /**
     * @brief Construct from x and y components
     * @param inX X component
     * @param inY Y component
     */
    constexpr Vec2(float inX, float inY) noexcept : x(inX), y(inY) {}

    /**
     * @brief Construct with same value for all components
     * @param scalar Value for both x and y
     */
    explicit constexpr Vec2(float scalar) noexcept : x(scalar), y(scalar) {}

    // Copy and move constructors/assignment (default)
    Vec2(const Vec2&) noexcept = default;
    Vec2(Vec2&&) noexcept = default;
    Vec2& operator=(const Vec2&) noexcept = default;
    Vec2& operator=(Vec2&&) noexcept = default;
    ~Vec2() = default;

    // Accessors

    /**
     * @brief Array-style access to components
     * @param index 0 for x, 1 for y
     * @return Reference to component
     */
    float& operator[](size_t index) noexcept { return (&x)[index]; }

    /**
     * @brief Const array-style access to components
     * @param index 0 for x, 1 for y
     * @return Const reference to component
     */
    const float& operator[](size_t index) const noexcept { return (&x)[index]; }

    // Arithmetic operators

    /**
     * @brief Vector addition
     */
    constexpr Vec2 operator+(const Vec2& other) const noexcept {
        return Vec2(x + other.x, y + other.y);
    }

    /**
     * @brief Vector subtraction
     */
    constexpr Vec2 operator-(const Vec2& other) const noexcept {
        return Vec2(x - other.x, y - other.y);
    }

    /**
     * @brief Component-wise multiplication
     */
    constexpr Vec2 operator*(const Vec2& other) const noexcept {
        return Vec2(x * other.x, y * other.y);
    }

    /**
     * @brief Component-wise division
     */
    constexpr Vec2 operator/(const Vec2& other) const noexcept {
        return Vec2(x / other.x, y / other.y);
    }

    /**
     * @brief Scalar multiplication
     */
    constexpr Vec2 operator*(float scalar) const noexcept { return Vec2(x * scalar, y * scalar); }

    /**
     * @brief Scalar division
     */
    constexpr Vec2 operator/(float scalar) const noexcept { return Vec2(x / scalar, y / scalar); }

    /**
     * @brief Unary negation
     */
    constexpr Vec2 operator-() const noexcept { return Vec2(-x, -y); }

    // Compound assignment operators

    Vec2& operator+=(const Vec2& other) noexcept {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vec2& operator-=(const Vec2& other) noexcept {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vec2& operator*=(const Vec2& other) noexcept {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    Vec2& operator/=(const Vec2& other) noexcept {
        x /= other.x;
        y /= other.y;
        return *this;
    }

    Vec2& operator*=(float scalar) noexcept {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    Vec2& operator/=(float scalar) noexcept {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    // Comparison operators

    /**
     * @brief Exact equality comparison
     */
    constexpr bool operator==(const Vec2& other) const noexcept {
        return x == other.x && y == other.y;
    }

    /**
     * @brief Inequality comparison
     */
    constexpr bool operator!=(const Vec2& other) const noexcept { return !(*this == other); }

    // Static utility functions

    /**
     * @brief Zero vector (0, 0)
     */
    static constexpr Vec2 zero() noexcept { return Vec2(0.0f, 0.0f); }

    /**
     * @brief One vector (1, 1)
     */
    static constexpr Vec2 one() noexcept { return Vec2(1.0f, 1.0f); }

    /**
     * @brief Unit X vector (1, 0)
     */
    static constexpr Vec2 unitX() noexcept { return Vec2(1.0f, 0.0f); }

    /**
     * @brief Unit Y vector (0, 1)
     */
    static constexpr Vec2 unitY() noexcept { return Vec2(0.0f, 1.0f); }
};

// Free functions for scalar * vector

/**
 * @brief Scalar multiplication (scalar * vector)
 */
constexpr Vec2 operator*(float scalar, const Vec2& vec) noexcept {
    return vec * scalar;
}

}  // namespace math
}  // namespace axiom
