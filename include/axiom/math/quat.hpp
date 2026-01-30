#pragma once

#include <cstddef>

namespace axiom::math {

// Forward declarations
class Vec3;
class Mat4;

/**
 * @brief Quaternion class for representing 3D rotations
 *
 * Represents a quaternion with components (x, y, z, w) where w is the scalar part.
 * Quaternions provide a gimbal-lock-free representation of 3D rotations and support
 * smooth interpolation (SLERP). More memory-efficient than rotation matrices (4 floats
 * vs 9 floats) and avoid the singularities of Euler angles.
 *
 * Storage format: (x, y, z, w) where:
 * - (x, y, z) is the vector part
 * - w is the scalar part
 *
 * For a rotation of angle θ around axis (ax, ay, az):
 * - x = ax * sin(θ/2)
 * - y = ay * sin(θ/2)
 * - z = az * sin(θ/2)
 * - w = cos(θ/2)
 */
class Quat {
public:
    float x;  ///< X component (vector part)
    float y;  ///< Y component (vector part)
    float z;  ///< Z component (vector part)
    float w;  ///< W component (scalar part)

    // Constructors

    /**
     * @brief Default constructor - initializes to identity quaternion (0, 0, 0, 1)
     */
    constexpr Quat() noexcept : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}

    /**
     * @brief Construct from components
     * @param inX X component (vector part)
     * @param inY Y component (vector part)
     * @param inZ Z component (vector part)
     * @param inW W component (scalar part)
     */
    constexpr Quat(float inX, float inY, float inZ, float inW) noexcept
        : x(inX), y(inY), z(inZ), w(inW) {}

    // Copy and move constructors/assignment (default)
    Quat(const Quat&) noexcept = default;
    Quat(Quat&&) noexcept = default;
    Quat& operator=(const Quat&) noexcept = default;
    Quat& operator=(Quat&&) noexcept = default;
    ~Quat() = default;

    // Accessors

    /**
     * @brief Array-style access to components [x, y, z, w]
     * @param index 0 for x, 1 for y, 2 for z, 3 for w
     * @return Reference to component
     */
    float& operator[](size_t index) noexcept { return (&x)[index]; }

    /**
     * @brief Const array-style access to components [x, y, z, w]
     * @param index 0 for x, 1 for y, 2 for z, 3 for w
     * @return Const reference to component
     */
    const float& operator[](size_t index) const noexcept { return (&x)[index]; }

    // Quaternion operations

    /**
     * @brief Quaternion multiplication (composition of rotations)
     * @param other The other quaternion
     * @return Result of this * other (applies other rotation first, then this)
     */
    Quat operator*(const Quat& other) const noexcept;

    /**
     * @brief Rotate a vector by this quaternion
     * @param v The vector to rotate
     * @return Rotated vector (q * v * q^-1)
     */
    Vec3 operator*(const Vec3& v) const noexcept;

    /**
     * @brief Quaternion conjugate (negates vector part)
     * @return Conjugate quaternion
     */
    constexpr Quat conjugate() const noexcept { return Quat(-x, -y, -z, w); }

    /**
     * @brief Quaternion inverse (for unit quaternions, same as conjugate)
     * @return Inverse quaternion
     */
    Quat inverse() const noexcept;

    /**
     * @brief Calculate length (magnitude) of quaternion
     * @return Length
     */
    float length() const noexcept;

    /**
     * @brief Calculate squared length (magnitude squared)
     * @return Length squared
     */
    constexpr float lengthSquared() const noexcept { return x * x + y * y + z * z + w * w; }

    /**
     * @brief Normalize the quaternion (return unit quaternion)
     * @return Normalized quaternion
     */
    Quat normalized() const noexcept;

    /**
     * @brief Normalize this quaternion in-place
     * @return Reference to this quaternion
     */
    Quat& normalize() noexcept;

    /**
     * @brief Dot product with another quaternion
     * @param other The other quaternion
     * @return Scalar dot product
     */
    constexpr float dot(const Quat& other) const noexcept {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }

    // Comparison operators

    /**
     * @brief Exact equality comparison
     */
    constexpr bool operator==(const Quat& other) const noexcept {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    /**
     * @brief Inequality comparison
     */
    constexpr bool operator!=(const Quat& other) const noexcept { return !(*this == other); }

    // Static factory methods

    /**
     * @brief Create identity quaternion (no rotation)
     * @return Identity quaternion (0, 0, 0, 1)
     */
    static constexpr Quat identity() noexcept { return Quat(0.0f, 0.0f, 0.0f, 1.0f); }

    /**
     * @brief Create quaternion from axis and angle
     * @param axis Rotation axis (must be normalized)
     * @param angleRadians Rotation angle in radians
     * @return Quaternion representing rotation
     */
    static Quat fromAxisAngle(const Vec3& axis, float angleRadians) noexcept;

    /**
     * @brief Create quaternion from Euler angles (intrinsic XYZ order)
     * @param pitch Rotation around X axis in radians
     * @param yaw Rotation around Y axis in radians
     * @param roll Rotation around Z axis in radians
     * @return Quaternion representing combined rotation
     * @note Uses intrinsic XYZ rotation order (roll-pitch-yaw)
     */
    static Quat fromEuler(float pitch, float yaw, float roll) noexcept;

    /**
     * @brief Create quaternion from rotation matrix
     * @param m Rotation matrix (should be orthogonal)
     * @return Quaternion representing same rotation
     */
    static Quat fromMatrix(const Mat4& m) noexcept;

    /**
     * @brief Spherical linear interpolation between two quaternions
     * @param a Start quaternion
     * @param b End quaternion
     * @param t Interpolation parameter [0, 1]
     * @return Interpolated quaternion
     * @note Uses the shortest path and handles quaternion double-cover
     */
    static Quat slerp(const Quat& a, const Quat& b, float t) noexcept;

    /**
     * @brief Normalized linear interpolation between two quaternions
     * @param a Start quaternion
     * @param b End quaternion
     * @param t Interpolation parameter [0, 1]
     * @return Interpolated and normalized quaternion
     * @note Faster than slerp but non-constant angular velocity
     */
    static Quat nlerp(const Quat& a, const Quat& b, float t) noexcept;

    // Conversion methods

    /**
     * @brief Convert quaternion to rotation matrix
     * @return 4x4 rotation matrix
     */
    Mat4 toMatrix() const noexcept;

    /**
     * @brief Convert quaternion to axis-angle representation
     * @param outAxis Output rotation axis (normalized)
     * @param outAngle Output rotation angle in radians
     */
    void toAxisAngle(Vec3& outAxis, float& outAngle) const noexcept;

    /**
     * @brief Convert quaternion to Euler angles (intrinsic XYZ order)
     * @param outPitch Output rotation around X axis in radians
     * @param outYaw Output rotation around Y axis in radians
     * @param outRoll Output rotation around Z axis in radians
     * @note Uses intrinsic XYZ rotation order (roll-pitch-yaw)
     */
    void toEuler(float& outPitch, float& outYaw, float& outRoll) const noexcept;
};

// Free functions

/**
 * @brief Dot product between two quaternions (free function)
 */
constexpr float dot(const Quat& a, const Quat& b) noexcept {
    return a.dot(b);
}

/**
 * @brief Spherical linear interpolation (free function)
 */
inline Quat slerp(const Quat& a, const Quat& b, float t) noexcept {
    return Quat::slerp(a, b, t);
}

/**
 * @brief Normalized linear interpolation (free function)
 */
inline Quat nlerp(const Quat& a, const Quat& b, float t) noexcept {
    return Quat::nlerp(a, b, t);
}

}  // namespace axiom::math
