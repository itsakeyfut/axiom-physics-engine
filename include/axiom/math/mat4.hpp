#pragma once

#include <cstddef>

namespace axiom::math {

// Forward declarations
class Vec3;
class Vec4;
class Quat;

/**
 * @brief 4x4 matrix class for 3D transformations
 *
 * Represents a 4x4 matrix with column-major storage order, matching
 * OpenGL and Vulkan conventions. Provides standard matrix operations
 * including multiplication, transposition, inversion, and factory
 * methods for creating transformation matrices.
 *
 * Memory layout (column-major):
 * m[0] m[4] m[ 8] m[12]
 * m[1] m[5] m[ 9] m[13]
 * m[2] m[6] m[10] m[14]
 * m[3] m[7] m[11] m[15]
 */
class Mat4 {
public:
    // Column-major storage: m[column * 4 + row]
    float m[16];

    // Constructors

    /**
     * @brief Default constructor - initializes to identity matrix
     */
    Mat4() noexcept;

    /**
     * @brief Construct from 16 values in column-major order
     * @param m00-m33 Matrix elements (column-major)
     */
    Mat4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13,
         float m20, float m21, float m22, float m23, float m30, float m31, float m32,
         float m33) noexcept;

    /**
     * @brief Construct from array of 16 floats (column-major)
     * @param data Array of 16 floats
     */
    explicit Mat4(const float* data) noexcept;

    // Copy and move constructors/assignment (default)
    Mat4(const Mat4&) noexcept = default;
    Mat4(Mat4&&) noexcept = default;
    Mat4& operator=(const Mat4&) noexcept = default;
    Mat4& operator=(Mat4&&) noexcept = default;
    ~Mat4() = default;

    // Accessors

    /**
     * @brief Array-style access to matrix elements
     * @param index Index into column-major array [0-15]
     * @return Reference to element
     */
    float& operator[](size_t index) noexcept { return m[index]; }

    /**
     * @brief Const array-style access to matrix elements
     * @param index Index into column-major array [0-15]
     * @return Const reference to element
     */
    const float& operator[](size_t index) const noexcept { return m[index]; }

    /**
     * @brief Access element at row and column
     * @param row Row index [0-3]
     * @param col Column index [0-3]
     * @return Reference to element
     */
    float& at(size_t row, size_t col) noexcept { return m[col * 4 + row]; }

    /**
     * @brief Const access element at row and column
     * @param row Row index [0-3]
     * @param col Column index [0-3]
     * @return Const reference to element
     */
    const float& at(size_t row, size_t col) const noexcept { return m[col * 4 + row]; }

    // Matrix operations

    /**
     * @brief Matrix multiplication
     * @param other The other matrix
     * @return Result of this * other
     */
    Mat4 operator*(const Mat4& other) const noexcept;

    /**
     * @brief Transform a Vec4 by this matrix
     * @param v The vector to transform
     * @return Transformed vector (this * v)
     */
    Vec4 operator*(const Vec4& v) const noexcept;

    /**
     * @brief Transform a Vec3 as a point (w=1)
     * @param v The point to transform
     * @return Transformed point
     */
    Vec3 transformPoint(const Vec3& v) const noexcept;

    /**
     * @brief Transform a Vec3 as a direction (w=0)
     * @param v The direction to transform
     * @return Transformed direction
     */
    Vec3 transformVector(const Vec3& v) const noexcept;

    /**
     * @brief Transpose this matrix
     * @return Transposed matrix
     */
    Mat4 transpose() const noexcept;

    /**
     * @brief Calculate inverse of this matrix
     * @return Inverse matrix (returns identity if non-invertible)
     */
    Mat4 inverse() const noexcept;

    /**
     * @brief Calculate determinant of this matrix
     * @return Determinant value
     */
    float determinant() const noexcept;

    // Comparison operators

    /**
     * @brief Exact equality comparison
     */
    bool operator==(const Mat4& other) const noexcept;

    /**
     * @brief Inequality comparison
     */
    bool operator!=(const Mat4& other) const noexcept { return !(*this == other); }

    // Static factory methods

    /**
     * @brief Create identity matrix
     * @return Identity matrix
     */
    static Mat4 identity() noexcept;

    /**
     * @brief Create zero matrix
     * @return Zero matrix
     */
    static Mat4 zero() noexcept;

    /**
     * @brief Create translation matrix
     * @param t Translation vector
     * @return Translation matrix
     */
    static Mat4 translation(const Vec3& t) noexcept;

    /**
     * @brief Create rotation matrix from quaternion
     * @param q Rotation quaternion (must be normalized)
     * @return Rotation matrix
     */
    static Mat4 rotation(const Quat& q) noexcept;

    /**
     * @brief Create rotation matrix around X axis
     * @param angleRadians Rotation angle in radians
     * @return Rotation matrix
     */
    static Mat4 rotationX(float angleRadians) noexcept;

    /**
     * @brief Create rotation matrix around Y axis
     * @param angleRadians Rotation angle in radians
     * @return Rotation matrix
     */
    static Mat4 rotationY(float angleRadians) noexcept;

    /**
     * @brief Create rotation matrix around Z axis
     * @param angleRadians Rotation angle in radians
     * @return Rotation matrix
     */
    static Mat4 rotationZ(float angleRadians) noexcept;

    /**
     * @brief Create rotation matrix from axis and angle
     * @param axis Rotation axis (must be normalized)
     * @param angleRadians Rotation angle in radians
     * @return Rotation matrix
     */
    static Mat4 rotationAxis(const Vec3& axis, float angleRadians) noexcept;

    /**
     * @brief Create scaling matrix
     * @param s Scaling factors
     * @return Scaling matrix
     */
    static Mat4 scaling(const Vec3& s) noexcept;

    /**
     * @brief Create uniform scaling matrix
     * @param s Uniform scaling factor
     * @return Scaling matrix
     */
    static Mat4 scaling(float s) noexcept;

    /**
     * @brief Create perspective projection matrix
     * @param fovYRadians Vertical field of view in radians
     * @param aspectRatio Width / height ratio
     * @param nearPlane Near clipping plane distance
     * @param farPlane Far clipping plane distance
     * @return Perspective projection matrix
     */
    static Mat4 perspective(float fovYRadians, float aspectRatio, float nearPlane,
                            float farPlane) noexcept;

    /**
     * @brief Create orthographic projection matrix
     * @param left Left plane coordinate
     * @param right Right plane coordinate
     * @param bottom Bottom plane coordinate
     * @param top Top plane coordinate
     * @param nearPlane Near clipping plane distance
     * @param farPlane Far clipping plane distance
     * @return Orthographic projection matrix
     */
    static Mat4 orthographic(float left, float right, float bottom, float top, float nearPlane,
                             float farPlane) noexcept;

    /**
     * @brief Create look-at view matrix
     * @param eye Camera position
     * @param target Look-at target position
     * @param up Up direction vector (will be normalized)
     * @return View matrix
     */
    static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) noexcept;
};

}  // namespace axiom::math
