#pragma once

#include "axiom/math/mat4.hpp"
#include "axiom/math/vec3.hpp"

#include <limits>

namespace axiom::math {

/**
 * @brief Axis-Aligned Bounding Box (AABB) for collision detection and spatial queries
 *
 * Represents an axis-aligned bounding box defined by minimum and maximum points.
 * AABBs are used extensively in collision detection for broad-phase culling,
 * BVH construction, and spatial partitioning. They provide fast overlap tests
 * and are cache-friendly for performance-critical operations.
 */
struct AABB {
    Vec3 min;  ///< Minimum point of the bounding box
    Vec3 max;  ///< Maximum point of the bounding box

    // Constructors

    /**
     * @brief Default constructor - creates an invalid AABB
     * @note The default AABB has min > max, which is considered invalid.
     *       Use expand() or merge() to make it valid.
     */
    constexpr AABB() noexcept
        : min(Vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max())),
          max(Vec3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::lowest())) {}

    /**
     * @brief Construct from minimum and maximum points
     * @param minPoint Minimum corner of the box
     * @param maxPoint Maximum corner of the box
     */
    constexpr AABB(const Vec3& minPoint, const Vec3& maxPoint) noexcept
        : min(minPoint), max(maxPoint) {}

    /**
     * @brief Construct from a single point (zero-volume AABB)
     * @param point The point to create AABB around
     */
    explicit constexpr AABB(const Vec3& point) noexcept : min(point), max(point) {}

    // Copy and move constructors/assignment (default)
    AABB(const AABB&) noexcept = default;
    AABB(AABB&&) noexcept = default;
    AABB& operator=(const AABB&) noexcept = default;
    AABB& operator=(AABB&&) noexcept = default;
    ~AABB() = default;

    // Query methods

    /**
     * @brief Get the center point of the AABB
     * @return Center point
     */
    constexpr Vec3 center() const noexcept { return (min + max) * 0.5f; }

    /**
     * @brief Get the half-extents (half-size) of the AABB
     * @return Half-size vector
     */
    constexpr Vec3 extents() const noexcept { return (max - min) * 0.5f; }

    /**
     * @brief Get the full size of the AABB
     * @return Size vector
     */
    constexpr Vec3 size() const noexcept { return max - min; }

    /**
     * @brief Get the surface area of the AABB
     * @return Surface area
     * @note Useful for BVH heuristics (Surface Area Heuristic)
     */
    constexpr float surfaceArea() const noexcept {
        Vec3 s = size();
        return 2.0f * (s.x * s.y + s.y * s.z + s.z * s.x);
    }

    /**
     * @brief Get the volume of the AABB
     * @return Volume
     */
    constexpr float volume() const noexcept {
        Vec3 s = size();
        return s.x * s.y * s.z;
    }

    /**
     * @brief Check if the AABB is valid (min <= max in all dimensions)
     * @return True if valid
     */
    constexpr bool isValid() const noexcept {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    // Containment tests

    /**
     * @brief Test if a point is contained within the AABB
     * @param point Point to test
     * @return True if point is inside or on the boundary
     */
    constexpr bool contains(const Vec3& point) const noexcept {
        return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }

    /**
     * @brief Test if another AABB is fully contained within this AABB
     * @param other AABB to test
     * @return True if other is fully inside
     */
    constexpr bool contains(const AABB& other) const noexcept {
        return other.min.x >= min.x && other.max.x <= max.x && other.min.y >= min.y &&
               other.max.y <= max.y && other.min.z >= min.z && other.max.z <= max.z;
    }

    // Intersection tests

    /**
     * @brief Test if this AABB intersects with another AABB
     * @param other AABB to test against
     * @return True if AABBs overlap
     */
    constexpr bool intersects(const AABB& other) const noexcept {
        return min.x <= other.max.x && max.x >= other.min.x && min.y <= other.max.y &&
               max.y >= other.min.y && min.z <= other.max.z && max.z >= other.min.z;
    }

    // Modification methods

    /**
     * @brief Expand the AABB to include a point
     * @param point Point to include
     */
    void expand(const Vec3& point) noexcept {
        min.x = (point.x < min.x) ? point.x : min.x;
        min.y = (point.y < min.y) ? point.y : min.y;
        min.z = (point.z < min.z) ? point.z : min.z;
        max.x = (point.x > max.x) ? point.x : max.x;
        max.y = (point.y > max.y) ? point.y : max.y;
        max.z = (point.z > max.z) ? point.z : max.z;
    }

    /**
     * @brief Expand the AABB by a margin in all directions
     * @param margin Amount to expand
     */
    void expand(float margin) noexcept {
        Vec3 offset(margin, margin, margin);
        min = min - offset;
        max = max + offset;
    }

    /**
     * @brief Merge this AABB with another AABB
     * @param other AABB to merge with
     */
    void merge(const AABB& other) noexcept {
        min.x = (other.min.x < min.x) ? other.min.x : min.x;
        min.y = (other.min.y < min.y) ? other.min.y : min.y;
        min.z = (other.min.z < min.z) ? other.min.z : min.z;
        max.x = (other.max.x > max.x) ? other.max.x : max.x;
        max.y = (other.max.y > max.y) ? other.max.y : max.y;
        max.z = (other.max.z > max.z) ? other.max.z : max.z;
    }

    // Transformation

    /**
     * @brief Transform the AABB by a 4x4 matrix
     * @param m Transformation matrix
     * @return Transformed AABB
     * @note This computes the AABB of all 8 corners transformed by the matrix
     */
    AABB transform(const Mat4& m) const noexcept;

    // Comparison operators

    /**
     * @brief Exact equality comparison
     */
    constexpr bool operator==(const AABB& other) const noexcept {
        return min == other.min && max == other.max;
    }

    /**
     * @brief Inequality comparison
     */
    constexpr bool operator!=(const AABB& other) const noexcept { return !(*this == other); }

    // Static factory methods

    /**
     * @brief Create an empty (invalid) AABB
     * @return Empty AABB
     */
    static constexpr AABB empty() noexcept { return AABB(); }

    /**
     * @brief Create an AABB from a point and half-extents
     * @param center Center point
     * @param halfExtents Half-size in each dimension
     * @return AABB centered at center with given extents
     */
    static constexpr AABB fromCenterExtents(const Vec3& center, const Vec3& halfExtents) noexcept {
        return AABB(center - halfExtents, center + halfExtents);
    }

    /**
     * @brief Merge two AABBs
     * @param a First AABB
     * @param b Second AABB
     * @return Merged AABB containing both inputs
     */
    static constexpr AABB merge(const AABB& a, const AABB& b) noexcept {
        return AABB(
            Vec3((a.min.x < b.min.x) ? a.min.x : b.min.x, (a.min.y < b.min.y) ? a.min.y : b.min.y,
                 (a.min.z < b.min.z) ? a.min.z : b.min.z),
            Vec3((a.max.x > b.max.x) ? a.max.x : b.max.x, (a.max.y > b.max.y) ? a.max.y : b.max.y,
                 (a.max.z > b.max.z) ? a.max.z : b.max.z));
    }
};

}  // namespace axiom::math
