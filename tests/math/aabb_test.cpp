#include "axiom/math/aabb.hpp"
#include "axiom/math/constants.hpp"
#include "axiom/math/mat4.hpp"
#include "axiom/math/vec3.hpp"

#include <gtest/gtest.h>

#include <cmath>

using namespace axiom::math;

// Helper function for floating point comparison
namespace {
constexpr float TEST_EPSILON = 1e-5f;

bool almostEqual(float a, float b, float epsilon = TEST_EPSILON) {
    return std::fabs(a - b) < epsilon;
}

bool almostEqual(const Vec3& a, const Vec3& b, float epsilon = TEST_EPSILON) {
    return almostEqual(a.x, b.x, epsilon) && almostEqual(a.y, b.y, epsilon) &&
           almostEqual(a.z, b.z, epsilon);
}

bool almostEqual(const AABB& a, const AABB& b, float epsilon = TEST_EPSILON) {
    return almostEqual(a.min, b.min, epsilon) && almostEqual(a.max, b.max, epsilon);
}
}  // namespace

// Constructor tests

TEST(AABBTest, DefaultConstructor) {
    AABB aabb;
    // Default AABB should be invalid (min > max)
    EXPECT_FALSE(aabb.isValid());
}

TEST(AABBTest, MinMaxConstructor) {
    Vec3 min(1.0f, 2.0f, 3.0f);
    Vec3 max(4.0f, 5.0f, 6.0f);
    AABB aabb(min, max);

    EXPECT_TRUE(almostEqual(aabb.min, min));
    EXPECT_TRUE(almostEqual(aabb.max, max));
    EXPECT_TRUE(aabb.isValid());
}

TEST(AABBTest, SinglePointConstructor) {
    Vec3 point(1.0f, 2.0f, 3.0f);
    AABB aabb(point);

    EXPECT_TRUE(almostEqual(aabb.min, point));
    EXPECT_TRUE(almostEqual(aabb.max, point));
    EXPECT_TRUE(aabb.isValid());
}

// Factory method tests

TEST(AABBTest, EmptyFactory) {
    AABB aabb = AABB::empty();
    EXPECT_FALSE(aabb.isValid());
}

TEST(AABBTest, FromCenterExtents) {
    Vec3 center(0.0f, 0.0f, 0.0f);
    Vec3 extents(1.0f, 2.0f, 3.0f);
    AABB aabb = AABB::fromCenterExtents(center, extents);

    EXPECT_TRUE(almostEqual(aabb.min, Vec3(-1.0f, -2.0f, -3.0f)));
    EXPECT_TRUE(almostEqual(aabb.max, Vec3(1.0f, 2.0f, 3.0f)));
    EXPECT_TRUE(almostEqual(aabb.center(), center));
    EXPECT_TRUE(almostEqual(aabb.extents(), extents));
}

// Query method tests

TEST(AABBTest, Center) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 4.0f, 6.0f));
    Vec3 center = aabb.center();

    EXPECT_TRUE(almostEqual(center, Vec3(1.0f, 2.0f, 3.0f)));
}

TEST(AABBTest, Extents) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(4.0f, 6.0f, 8.0f));
    Vec3 extents = aabb.extents();

    EXPECT_TRUE(almostEqual(extents, Vec3(2.0f, 3.0f, 4.0f)));
}

TEST(AABBTest, Size) {
    AABB aabb(Vec3(1.0f, 2.0f, 3.0f), Vec3(4.0f, 6.0f, 9.0f));
    Vec3 size = aabb.size();

    EXPECT_TRUE(almostEqual(size, Vec3(3.0f, 4.0f, 6.0f)));
}

TEST(AABBTest, SurfaceArea) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 2.0f, 3.0f));
    float area = aabb.surfaceArea();

    // Surface area = 2 * (1*2 + 2*3 + 3*1) = 2 * (2 + 6 + 3) = 22
    EXPECT_TRUE(almostEqual(area, 22.0f));
}

TEST(AABBTest, Volume) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 3.0f, 4.0f));
    float volume = aabb.volume();

    // Volume = 2 * 3 * 4 = 24
    EXPECT_TRUE(almostEqual(volume, 24.0f));
}

TEST(AABBTest, IsValid) {
    AABB valid(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));
    EXPECT_TRUE(valid.isValid());

    AABB invalid(Vec3(1.0f, 1.0f, 1.0f), Vec3(0.0f, 0.0f, 0.0f));
    EXPECT_FALSE(invalid.isValid());

    AABB point(Vec3(1.0f, 1.0f, 1.0f));
    EXPECT_TRUE(point.isValid());  // Zero-volume AABB is still valid
}

// Containment tests

TEST(AABBTest, ContainsPoint_Inside) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 2.0f, 2.0f));
    Vec3 point(1.0f, 1.0f, 1.0f);

    EXPECT_TRUE(aabb.contains(point));
}

TEST(AABBTest, ContainsPoint_OnBoundary) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 2.0f, 2.0f));
    Vec3 point(2.0f, 1.0f, 1.0f);

    EXPECT_TRUE(aabb.contains(point));
}

TEST(AABBTest, ContainsPoint_Outside) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 2.0f, 2.0f));
    Vec3 point(3.0f, 1.0f, 1.0f);

    EXPECT_FALSE(aabb.contains(point));
}

TEST(AABBTest, ContainsPoint_Corner) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 2.0f, 2.0f));

    EXPECT_TRUE(aabb.contains(Vec3(0.0f, 0.0f, 0.0f)));  // min corner
    EXPECT_TRUE(aabb.contains(Vec3(2.0f, 2.0f, 2.0f)));  // max corner
}

TEST(AABBTest, ContainsAABB_Fully) {
    AABB outer(Vec3(0.0f, 0.0f, 0.0f), Vec3(4.0f, 4.0f, 4.0f));
    AABB inner(Vec3(1.0f, 1.0f, 1.0f), Vec3(3.0f, 3.0f, 3.0f));

    EXPECT_TRUE(outer.contains(inner));
    EXPECT_FALSE(inner.contains(outer));
}

TEST(AABBTest, ContainsAABB_Partial) {
    AABB aabb1(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 2.0f, 2.0f));
    AABB aabb2(Vec3(1.0f, 1.0f, 1.0f), Vec3(3.0f, 3.0f, 3.0f));

    EXPECT_FALSE(aabb1.contains(aabb2));
    EXPECT_FALSE(aabb2.contains(aabb1));
}

TEST(AABBTest, ContainsAABB_Same) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 2.0f, 2.0f));

    EXPECT_TRUE(aabb.contains(aabb));
}

// Intersection tests

TEST(AABBTest, Intersects_Overlapping) {
    AABB aabb1(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 2.0f, 2.0f));
    AABB aabb2(Vec3(1.0f, 1.0f, 1.0f), Vec3(3.0f, 3.0f, 3.0f));

    EXPECT_TRUE(aabb1.intersects(aabb2));
    EXPECT_TRUE(aabb2.intersects(aabb1));
}

TEST(AABBTest, Intersects_Touching) {
    AABB aabb1(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));
    AABB aabb2(Vec3(1.0f, 0.0f, 0.0f), Vec3(2.0f, 1.0f, 1.0f));

    EXPECT_TRUE(aabb1.intersects(aabb2));  // Touching counts as intersecting
}

TEST(AABBTest, Intersects_Separated) {
    AABB aabb1(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));
    AABB aabb2(Vec3(2.0f, 0.0f, 0.0f), Vec3(3.0f, 1.0f, 1.0f));

    EXPECT_FALSE(aabb1.intersects(aabb2));
    EXPECT_FALSE(aabb2.intersects(aabb1));
}

TEST(AABBTest, Intersects_Contained) {
    AABB outer(Vec3(0.0f, 0.0f, 0.0f), Vec3(4.0f, 4.0f, 4.0f));
    AABB inner(Vec3(1.0f, 1.0f, 1.0f), Vec3(3.0f, 3.0f, 3.0f));

    EXPECT_TRUE(outer.intersects(inner));
    EXPECT_TRUE(inner.intersects(outer));
}

// Expansion tests

TEST(AABBTest, ExpandByPoint_Inside) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 2.0f, 2.0f));
    Vec3 point(1.0f, 1.0f, 1.0f);

    aabb.expand(point);

    EXPECT_TRUE(almostEqual(aabb.min, Vec3(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(almostEqual(aabb.max, Vec3(2.0f, 2.0f, 2.0f)));
}

TEST(AABBTest, ExpandByPoint_Outside) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 2.0f, 2.0f));
    Vec3 point(3.0f, 3.0f, 3.0f);

    aabb.expand(point);

    EXPECT_TRUE(almostEqual(aabb.min, Vec3(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(almostEqual(aabb.max, Vec3(3.0f, 3.0f, 3.0f)));
}

TEST(AABBTest, ExpandByPoint_NegativeDirection) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 2.0f, 2.0f));
    Vec3 point(-1.0f, -1.0f, -1.0f);

    aabb.expand(point);

    EXPECT_TRUE(almostEqual(aabb.min, Vec3(-1.0f, -1.0f, -1.0f)));
    EXPECT_TRUE(almostEqual(aabb.max, Vec3(2.0f, 2.0f, 2.0f)));
}

TEST(AABBTest, ExpandByMargin) {
    AABB aabb(Vec3(1.0f, 1.0f, 1.0f), Vec3(2.0f, 2.0f, 2.0f));
    float margin = 0.5f;

    aabb.expand(margin);

    EXPECT_TRUE(almostEqual(aabb.min, Vec3(0.5f, 0.5f, 0.5f)));
    EXPECT_TRUE(almostEqual(aabb.max, Vec3(2.5f, 2.5f, 2.5f)));
}

TEST(AABBTest, ExpandFromEmpty) {
    AABB aabb = AABB::empty();
    Vec3 point(1.0f, 2.0f, 3.0f);

    aabb.expand(point);

    EXPECT_TRUE(almostEqual(aabb.min, point));
    EXPECT_TRUE(almostEqual(aabb.max, point));
    EXPECT_TRUE(aabb.isValid());
}

// Merge tests

TEST(AABBTest, MergeMethod) {
    AABB aabb1(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));
    AABB aabb2(Vec3(0.5f, 0.5f, 0.5f), Vec3(2.0f, 2.0f, 2.0f));

    aabb1.merge(aabb2);

    EXPECT_TRUE(almostEqual(aabb1.min, Vec3(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(almostEqual(aabb1.max, Vec3(2.0f, 2.0f, 2.0f)));
}

TEST(AABBTest, MergeStatic) {
    AABB aabb1(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));
    AABB aabb2(Vec3(-1.0f, -1.0f, -1.0f), Vec3(0.5f, 0.5f, 0.5f));

    AABB merged = AABB::merge(aabb1, aabb2);

    EXPECT_TRUE(almostEqual(merged.min, Vec3(-1.0f, -1.0f, -1.0f)));
    EXPECT_TRUE(almostEqual(merged.max, Vec3(1.0f, 1.0f, 1.0f)));

    // Original AABBs should be unchanged
    EXPECT_TRUE(almostEqual(aabb1.min, Vec3(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(almostEqual(aabb2.min, Vec3(-1.0f, -1.0f, -1.0f)));
}

TEST(AABBTest, MergeDisjoint) {
    AABB aabb1(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));
    AABB aabb2(Vec3(10.0f, 10.0f, 10.0f), Vec3(11.0f, 11.0f, 11.0f));

    AABB merged = AABB::merge(aabb1, aabb2);

    EXPECT_TRUE(almostEqual(merged.min, Vec3(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(almostEqual(merged.max, Vec3(11.0f, 11.0f, 11.0f)));
}

// Transformation tests

TEST(AABBTest, TransformIdentity) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));
    Mat4 identity = Mat4::identity();

    AABB transformed = aabb.transform(identity);

    EXPECT_TRUE(almostEqual(transformed, aabb));
}

TEST(AABBTest, TransformTranslation) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));
    Mat4 translation = Mat4::translation(Vec3(5.0f, 5.0f, 5.0f));

    AABB transformed = aabb.transform(translation);

    EXPECT_TRUE(almostEqual(transformed.min, Vec3(5.0f, 5.0f, 5.0f)));
    EXPECT_TRUE(almostEqual(transformed.max, Vec3(6.0f, 6.0f, 6.0f)));
}

TEST(AABBTest, TransformScale) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));
    Mat4 scale = Mat4::scaling(Vec3(2.0f, 2.0f, 2.0f));

    AABB transformed = aabb.transform(scale);

    EXPECT_TRUE(almostEqual(transformed.min, Vec3(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(almostEqual(transformed.max, Vec3(2.0f, 2.0f, 2.0f)));
}

TEST(AABBTest, TransformRotation) {
    // Unit cube centered at origin
    AABB aabb(Vec3(-1.0f, -1.0f, -1.0f), Vec3(1.0f, 1.0f, 1.0f));
    // 45 degree rotation around Z axis
    Mat4 rotation = Mat4::rotationZ(PI<float> / 4.0f);

    AABB transformed = aabb.transform(rotation);

    // After rotation, the AABB should expand to contain the rotated cube
    // The diagonal of the square face is sqrt(2) * 2 = 2.828...
    float expectedExtent = std::sqrt(2.0f);

    EXPECT_TRUE(almostEqual(transformed.min.x, -expectedExtent, 1e-4f));
    EXPECT_TRUE(almostEqual(transformed.max.x, expectedExtent, 1e-4f));
    EXPECT_TRUE(almostEqual(transformed.min.y, -expectedExtent, 1e-4f));
    EXPECT_TRUE(almostEqual(transformed.max.y, expectedExtent, 1e-4f));
    // Z should remain unchanged
    EXPECT_TRUE(almostEqual(transformed.min.z, -1.0f));
    EXPECT_TRUE(almostEqual(transformed.max.z, 1.0f));
}

TEST(AABBTest, TransformCombined) {
    AABB aabb(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));

    // Scale, then translate
    Mat4 scale = Mat4::scaling(2.0f);
    Mat4 translation = Mat4::translation(Vec3(10.0f, 10.0f, 10.0f));
    Mat4 combined = translation * scale;

    AABB transformed = aabb.transform(combined);

    EXPECT_TRUE(almostEqual(transformed.min, Vec3(10.0f, 10.0f, 10.0f)));
    EXPECT_TRUE(almostEqual(transformed.max, Vec3(12.0f, 12.0f, 12.0f)));
}

// Comparison tests

TEST(AABBTest, EqualityOperator) {
    AABB aabb1(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));
    AABB aabb2(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));

    EXPECT_TRUE(aabb1 == aabb2);
    EXPECT_FALSE(aabb1 != aabb2);
}

TEST(AABBTest, InequalityOperator) {
    AABB aabb1(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));
    AABB aabb2(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 1.0f, 1.0f));

    EXPECT_FALSE(aabb1 == aabb2);
    EXPECT_TRUE(aabb1 != aabb2);
}

// Edge case tests

TEST(AABBTest, ZeroVolumeAABB) {
    Vec3 point(1.0f, 2.0f, 3.0f);
    AABB aabb(point);

    EXPECT_TRUE(aabb.isValid());
    EXPECT_TRUE(almostEqual(aabb.volume(), 0.0f));
    EXPECT_TRUE(almostEqual(aabb.surfaceArea(), 0.0f));
    EXPECT_TRUE(aabb.contains(point));
}

TEST(AABBTest, NegativeCoordinates) {
    AABB aabb(Vec3(-5.0f, -5.0f, -5.0f), Vec3(-1.0f, -1.0f, -1.0f));

    EXPECT_TRUE(aabb.isValid());
    EXPECT_TRUE(almostEqual(aabb.center(), Vec3(-3.0f, -3.0f, -3.0f)));
    EXPECT_TRUE(aabb.contains(Vec3(-3.0f, -3.0f, -3.0f)));
}

TEST(AABBTest, LargeCoordinates) {
    AABB aabb(Vec3(1000.0f, 1000.0f, 1000.0f), Vec3(2000.0f, 2000.0f, 2000.0f));

    EXPECT_TRUE(aabb.isValid());
    EXPECT_TRUE(almostEqual(aabb.size(), Vec3(1000.0f, 1000.0f, 1000.0f)));
}
