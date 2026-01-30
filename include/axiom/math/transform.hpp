#pragma once

#include "axiom/math/mat4.hpp"
#include "axiom/math/quat.hpp"
#include "axiom/math/vec3.hpp"

namespace axiom::math {

/**
 * @brief Transform class for representing position, rotation, and scale
 *
 * Represents a complete 3D transformation with position (translation),
 * rotation (quaternion), and scale components. Provides methods for
 * converting to/from matrices, composing transforms, and transforming
 * points, directions, and normals. Useful for rigid body poses,
 * hierarchical scene graphs, and coordinate space conversions.
 */
struct Transform {
    Vec3 position;  ///< Position/translation component
    Quat rotation;  ///< Rotation component (quaternion)
    Vec3 scale;     ///< Scale component (per-axis)

    // Constructors

    /**
     * @brief Default constructor - initializes to identity transform
     */
    constexpr Transform() noexcept
        : position(0.0f, 0.0f, 0.0f), rotation(), scale(1.0f, 1.0f, 1.0f) {}

    /**
     * @brief Construct from position, rotation, and scale
     * @param pos Position vector
     * @param rot Rotation quaternion
     * @param scl Scale vector
     */
    constexpr Transform(const Vec3& pos, const Quat& rot, const Vec3& scl) noexcept
        : position(pos), rotation(rot), scale(scl) {}

    /**
     * @brief Construct from position and rotation (uniform scale of 1)
     * @param pos Position vector
     * @param rot Rotation quaternion
     */
    constexpr Transform(const Vec3& pos, const Quat& rot) noexcept
        : position(pos), rotation(rot), scale(1.0f, 1.0f, 1.0f) {}

    /**
     * @brief Construct from position only (identity rotation, uniform scale of 1)
     * @param pos Position vector
     */
    explicit constexpr Transform(const Vec3& pos) noexcept
        : position(pos), rotation(), scale(1.0f, 1.0f, 1.0f) {}

    // Copy and move constructors/assignment (default)
    Transform(const Transform&) noexcept = default;
    Transform(Transform&&) noexcept = default;
    Transform& operator=(const Transform&) noexcept = default;
    Transform& operator=(Transform&&) noexcept = default;
    ~Transform() = default;

    // Static factory methods

    /**
     * @brief Create identity transform (no translation, rotation, or scale)
     * @return Identity transform
     */
    static constexpr Transform identity() noexcept {
        return Transform(Vec3(0.0f, 0.0f, 0.0f), Quat::identity(), Vec3(1.0f, 1.0f, 1.0f));
    }

    // Conversion methods

    /**
     * @brief Convert transform to 4x4 transformation matrix
     * @return 4x4 matrix representing this transform (TRS order: translate * rotate * scale)
     */
    Mat4 toMatrix() const noexcept;

    /**
     * @brief Create transform from 4x4 transformation matrix
     * @param m Transformation matrix
     * @return Transform decomposed from matrix
     * @note Assumes matrix is composed of translation, rotation, and scale (no shear/skew)
     */
    static Transform fromMatrix(const Mat4& m) noexcept;

    // Inverse transform

    /**
     * @brief Calculate inverse of this transform
     * @return Inverse transform
     * @note For transforms with non-zero scale components only
     */
    Transform inverse() const noexcept;

    // Point and vector transformation

    /**
     * @brief Transform a point (applies scale, rotation, and translation)
     * @param point Point in local space
     * @return Point in world space
     */
    Vec3 transformPoint(const Vec3& point) const noexcept;

    /**
     * @brief Transform a direction vector (applies scale and rotation, no translation)
     * @param direction Direction vector in local space
     * @return Direction vector in world space
     */
    Vec3 transformDirection(const Vec3& direction) const noexcept;

    /**
     * @brief Transform a normal vector (applies inverse transpose of scale and rotation)
     * @param normal Normal vector in local space
     * @return Normal vector in world space (normalized)
     * @note Normal vectors require special handling to remain perpendicular under non-uniform
     * scaling
     */
    Vec3 transformNormal(const Vec3& normal) const noexcept;

    /**
     * @brief Inverse transform a point (from world space to local space)
     * @param point Point in world space
     * @return Point in local space
     */
    Vec3 inverseTransformPoint(const Vec3& point) const noexcept;

    /**
     * @brief Inverse transform a direction vector (from world space to local space)
     * @param direction Direction vector in world space
     * @return Direction vector in local space
     */
    Vec3 inverseTransformDirection(const Vec3& direction) const noexcept;

    /**
     * @brief Inverse transform a normal vector (from world space to local space)
     * @param normal Normal vector in world space
     * @return Normal vector in local space (normalized)
     */
    Vec3 inverseTransformNormal(const Vec3& normal) const noexcept;

    // Transform composition

    /**
     * @brief Compose this transform with another (parent * child relationship)
     * @param other Child transform
     * @return Combined transform (applies other first, then this)
     * @note For parent-child hierarchies: worldTransform = parent * child
     */
    Transform operator*(const Transform& other) const noexcept;

    // Comparison operators

    /**
     * @brief Exact equality comparison
     */
    constexpr bool operator==(const Transform& other) const noexcept {
        return position == other.position && rotation == other.rotation && scale == other.scale;
    }

    /**
     * @brief Inequality comparison
     */
    constexpr bool operator!=(const Transform& other) const noexcept { return !(*this == other); }
};

}  // namespace axiom::math
