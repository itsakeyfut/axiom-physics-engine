#pragma once

#include <cmath>
#include <cstddef>

namespace axiom {
namespace math {

/**
 * @brief 3D vector class with x, y, and z components
 *
 * Represents a 3-dimensional vector with floating-point components.
 * Supports standard vector operations including arithmetic, comparison,
 * dot product, cross product, and utility functions.
 */
class Vec3 {
public:
    float x;
    float y;
    float z;

    // Constructors

    /**
     * @brief Default constructor - initializes to zero vector
     */
    constexpr Vec3() noexcept : x(0.0f), y(0.0f), z(0.0f) {}

    /**
     * @brief Construct from x, y, and z components
     * @param x X component
     * @param y Y component
     * @param z Z component
     */
    constexpr Vec3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}

    /**
     * @brief Construct with same value for all components
     * @param scalar Value for x, y, and z
     */
    explicit constexpr Vec3(float scalar) noexcept : x(scalar), y(scalar), z(scalar) {}

    // Copy and move constructors/assignment (default)
    Vec3(const Vec3&) noexcept = default;
    Vec3(Vec3&&) noexcept = default;
    Vec3& operator=(const Vec3&) noexcept = default;
    Vec3& operator=(Vec3&&) noexcept = default;
    ~Vec3() = default;

    // Accessors

    /**
     * @brief Array-style access to components
     * @param index 0 for x, 1 for y, 2 for z
     * @return Reference to component
     */
    float& operator[](size_t index) noexcept {
        return (&x)[index];
    }

    /**
     * @brief Const array-style access to components
     * @param index 0 for x, 1 for y, 2 for z
     * @return Const reference to component
     */
    const float& operator[](size_t index) const noexcept {
        return (&x)[index];
    }

    // Arithmetic operators

    /**
     * @brief Vector addition
     */
    constexpr Vec3 operator+(const Vec3& other) const noexcept {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    /**
     * @brief Vector subtraction
     */
    constexpr Vec3 operator-(const Vec3& other) const noexcept {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    /**
     * @brief Component-wise multiplication
     */
    constexpr Vec3 operator*(const Vec3& other) const noexcept {
        return Vec3(x * other.x, y * other.y, z * other.z);
    }

    /**
     * @brief Component-wise division
     */
    constexpr Vec3 operator/(const Vec3& other) const noexcept {
        return Vec3(x / other.x, y / other.y, z / other.z);
    }

    /**
     * @brief Scalar multiplication
     */
    constexpr Vec3 operator*(float scalar) const noexcept {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }

    /**
     * @brief Scalar division
     */
    constexpr Vec3 operator/(float scalar) const noexcept {
        return Vec3(x / scalar, y / scalar, z / scalar);
    }

    /**
     * @brief Unary negation
     */
    constexpr Vec3 operator-() const noexcept {
        return Vec3(-x, -y, -z);
    }

    // Compound assignment operators

    Vec3& operator+=(const Vec3& other) noexcept {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vec3& operator-=(const Vec3& other) noexcept {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vec3& operator*=(const Vec3& other) noexcept {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        return *this;
    }

    Vec3& operator/=(const Vec3& other) noexcept {
        x /= other.x;
        y /= other.y;
        z /= other.z;
        return *this;
    }

    Vec3& operator*=(float scalar) noexcept {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    Vec3& operator/=(float scalar) noexcept {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    // Comparison operators

    /**
     * @brief Exact equality comparison
     */
    constexpr bool operator==(const Vec3& other) const noexcept {
        return x == other.x && y == other.y && z == other.z;
    }

    /**
     * @brief Inequality comparison
     */
    constexpr bool operator!=(const Vec3& other) const noexcept {
        return !(*this == other);
    }

    // Vector operations

    /**
     * @brief Dot product with another vector
     * @param other The other vector
     * @return Scalar dot product
     */
    constexpr float dot(const Vec3& other) const noexcept {
        return x * other.x + y * other.y + z * other.z;
    }

    /**
     * @brief Cross product with another vector
     * @param other The other vector
     * @return Perpendicular vector
     */
    constexpr Vec3 cross(const Vec3& other) const noexcept {
        return Vec3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    /**
     * @brief Calculate squared length (magnitude squared)
     * @return Length squared
     */
    constexpr float lengthSquared() const noexcept {
        return x * x + y * y + z * z;
    }

    /**
     * @brief Calculate length (magnitude)
     * @return Length
     */
    float length() const noexcept {
        return std::sqrt(lengthSquared());
    }

    /**
     * @brief Normalize the vector (return unit vector)
     * @return Normalized vector
     */
    Vec3 normalized() const noexcept {
        const float len = length();
        if (len > 0.0f) {
            return *this / len;
        }
        return Vec3::zero();
    }

    /**
     * @brief Normalize this vector in-place
     * @return Reference to this vector
     */
    Vec3& normalize() noexcept {
        const float len = length();
        if (len > 0.0f) {
            *this /= len;
        }
        return *this;
    }

    // Static utility functions

    /**
     * @brief Zero vector (0, 0, 0)
     */
    static constexpr Vec3 zero() noexcept {
        return Vec3(0.0f, 0.0f, 0.0f);
    }

    /**
     * @brief One vector (1, 1, 1)
     */
    static constexpr Vec3 one() noexcept {
        return Vec3(1.0f, 1.0f, 1.0f);
    }

    /**
     * @brief Unit X vector (1, 0, 0)
     */
    static constexpr Vec3 unitX() noexcept {
        return Vec3(1.0f, 0.0f, 0.0f);
    }

    /**
     * @brief Unit Y vector (0, 1, 0)
     */
    static constexpr Vec3 unitY() noexcept {
        return Vec3(0.0f, 1.0f, 0.0f);
    }

    /**
     * @brief Unit Z vector (0, 0, 1)
     */
    static constexpr Vec3 unitZ() noexcept {
        return Vec3(0.0f, 0.0f, 1.0f);
    }
};

// Free functions for scalar * vector

/**
 * @brief Scalar multiplication (scalar * vector)
 */
constexpr Vec3 operator*(float scalar, const Vec3& vec) noexcept {
    return vec * scalar;
}

/**
 * @brief Dot product (free function)
 */
constexpr float dot(const Vec3& a, const Vec3& b) noexcept {
    return a.dot(b);
}

/**
 * @brief Cross product (free function)
 */
constexpr Vec3 cross(const Vec3& a, const Vec3& b) noexcept {
    return a.cross(b);
}

}  // namespace math
}  // namespace axiom
