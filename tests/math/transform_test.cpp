#include "axiom/math/constants.hpp"
#include "axiom/math/mat4.hpp"
#include "axiom/math/quat.hpp"
#include "axiom/math/transform.hpp"
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

bool almostEqual(const Quat& a, const Quat& b, float epsilon = TEST_EPSILON) {
    return almostEqual(a.x, b.x, epsilon) && almostEqual(a.y, b.y, epsilon) &&
           almostEqual(a.z, b.z, epsilon) && almostEqual(a.w, b.w, epsilon);
}

bool almostEqual(const Transform& a, const Transform& b, float epsilon = TEST_EPSILON) {
    return almostEqual(a.position, b.position, epsilon) &&
           almostEqual(a.rotation, b.rotation, epsilon) && almostEqual(a.scale, b.scale, epsilon);
}
}  // namespace

// Constructor tests

TEST(TransformTest, DefaultConstructor) {
    Transform t;
    // Should initialize to identity transform
    EXPECT_TRUE(almostEqual(t.position, Vec3(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(almostEqual(t.rotation, Quat::identity()));
    EXPECT_TRUE(almostEqual(t.scale, Vec3(1.0f, 1.0f, 1.0f)));
}

TEST(TransformTest, PositionRotationScaleConstructor) {
    Vec3 pos(1.0f, 2.0f, 3.0f);
    Quat rot = Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 4.0f);
    Vec3 scl(2.0f, 2.0f, 2.0f);

    Transform t(pos, rot, scl);

    EXPECT_TRUE(almostEqual(t.position, pos));
    EXPECT_TRUE(almostEqual(t.rotation, rot));
    EXPECT_TRUE(almostEqual(t.scale, scl));
}

TEST(TransformTest, PositionRotationConstructor) {
    Vec3 pos(1.0f, 2.0f, 3.0f);
    Quat rot = Quat::fromAxisAngle(Vec3::unitY(), PI<float> / 2.0f);

    Transform t(pos, rot);

    EXPECT_TRUE(almostEqual(t.position, pos));
    EXPECT_TRUE(almostEqual(t.rotation, rot));
    EXPECT_TRUE(almostEqual(t.scale, Vec3(1.0f, 1.0f, 1.0f)));
}

TEST(TransformTest, PositionOnlyConstructor) {
    Vec3 pos(5.0f, 6.0f, 7.0f);

    Transform t(pos);

    EXPECT_TRUE(almostEqual(t.position, pos));
    EXPECT_TRUE(almostEqual(t.rotation, Quat::identity()));
    EXPECT_TRUE(almostEqual(t.scale, Vec3(1.0f, 1.0f, 1.0f)));
}

// Factory method tests

TEST(TransformTest, Identity) {
    Transform t = Transform::identity();

    EXPECT_TRUE(almostEqual(t.position, Vec3(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(almostEqual(t.rotation, Quat::identity()));
    EXPECT_TRUE(almostEqual(t.scale, Vec3(1.0f, 1.0f, 1.0f)));
}

// Matrix conversion tests

TEST(TransformTest, ToMatrixIdentity) {
    Transform t = Transform::identity();
    Mat4 m = t.toMatrix();
    Mat4 expected = Mat4::identity();

    EXPECT_TRUE(m == expected);
}

TEST(TransformTest, ToMatrixTranslation) {
    Transform t(Vec3(1.0f, 2.0f, 3.0f));
    Mat4 m = t.toMatrix();
    Mat4 expected = Mat4::translation(Vec3(1.0f, 2.0f, 3.0f));

    EXPECT_TRUE(almostEqual(m.at(0, 3), expected.at(0, 3)));
    EXPECT_TRUE(almostEqual(m.at(1, 3), expected.at(1, 3)));
    EXPECT_TRUE(almostEqual(m.at(2, 3), expected.at(2, 3)));
}

TEST(TransformTest, ToMatrixRotation) {
    Quat rot = Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 2.0f);
    Transform t(Vec3::zero(), rot);
    Mat4 m = t.toMatrix();

    // Rotate point (1, 0, 0) should give approximately (0, 1, 0)
    Vec3 point(1.0f, 0.0f, 0.0f);
    Vec3 rotated = m.transformPoint(point);

    EXPECT_TRUE(almostEqual(rotated.x, 0.0f));
    EXPECT_TRUE(almostEqual(rotated.y, 1.0f));
    EXPECT_TRUE(almostEqual(rotated.z, 0.0f));
}

TEST(TransformTest, ToMatrixScale) {
    Transform t(Vec3::zero(), Quat::identity(), Vec3(2.0f, 3.0f, 4.0f));
    Mat4 m = t.toMatrix();

    // Transform point (1, 1, 1) should give (2, 3, 4)
    Vec3 point(1.0f, 1.0f, 1.0f);
    Vec3 scaled = m.transformPoint(point);

    EXPECT_TRUE(almostEqual(scaled.x, 2.0f));
    EXPECT_TRUE(almostEqual(scaled.y, 3.0f));
    EXPECT_TRUE(almostEqual(scaled.z, 4.0f));
}

TEST(TransformTest, ToMatrixCombined) {
    // Translation, rotation, and scale combined
    Vec3 pos(1.0f, 2.0f, 3.0f);
    Quat rot = Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 2.0f);
    Vec3 scl(2.0f, 2.0f, 2.0f);

    Transform t(pos, rot, scl);
    Mat4 m = t.toMatrix();

    // Transform a point and verify it matches manual calculation
    Vec3 point(1.0f, 0.0f, 0.0f);
    Vec3 transformed = m.transformPoint(point);

    // Expected: scale (2,0,0), rotate to (0,2,0), translate to (1,4,3)
    EXPECT_TRUE(almostEqual(transformed.x, 1.0f));
    EXPECT_TRUE(almostEqual(transformed.y, 4.0f));
    EXPECT_TRUE(almostEqual(transformed.z, 3.0f));
}

TEST(TransformTest, FromMatrixIdentity) {
    Mat4 m = Mat4::identity();
    Transform t = Transform::fromMatrix(m);

    EXPECT_TRUE(almostEqual(t, Transform::identity()));
}

TEST(TransformTest, FromMatrixTranslation) {
    Mat4 m = Mat4::translation(Vec3(1.0f, 2.0f, 3.0f));
    Transform t = Transform::fromMatrix(m);

    EXPECT_TRUE(almostEqual(t.position, Vec3(1.0f, 2.0f, 3.0f)));
    EXPECT_TRUE(almostEqual(t.rotation, Quat::identity()));
    EXPECT_TRUE(almostEqual(t.scale, Vec3(1.0f, 1.0f, 1.0f)));
}

TEST(TransformTest, FromMatrixRotation) {
    Quat rot = Quat::fromAxisAngle(Vec3::unitY(), PI<float> / 4.0f);
    Mat4 m = Mat4::rotation(rot);
    Transform t = Transform::fromMatrix(m);

    EXPECT_TRUE(almostEqual(t.position, Vec3::zero()));
    EXPECT_TRUE(almostEqual(t.rotation, rot));
    EXPECT_TRUE(almostEqual(t.scale, Vec3(1.0f, 1.0f, 1.0f)));
}

TEST(TransformTest, FromMatrixScale) {
    Mat4 m = Mat4::scaling(Vec3(2.0f, 3.0f, 4.0f));
    Transform t = Transform::fromMatrix(m);

    EXPECT_TRUE(almostEqual(t.position, Vec3::zero()));
    EXPECT_TRUE(almostEqual(t.rotation, Quat::identity()));
    EXPECT_TRUE(almostEqual(t.scale, Vec3(2.0f, 3.0f, 4.0f)));
}

TEST(TransformTest, MatrixRoundTrip) {
    // Create transform, convert to matrix, convert back
    Vec3 pos(1.0f, 2.0f, 3.0f);
    Quat rot = Quat::fromAxisAngle(Vec3(0.577f, 0.577f, 0.577f).normalized(), PI<float> / 3.0f);
    Vec3 scl(2.0f, 3.0f, 4.0f);

    Transform original(pos, rot, scl);
    Mat4 m = original.toMatrix();
    Transform roundTrip = Transform::fromMatrix(m);

    EXPECT_TRUE(almostEqual(roundTrip.position, original.position));
    EXPECT_TRUE(almostEqual(roundTrip.rotation, original.rotation));
    EXPECT_TRUE(almostEqual(roundTrip.scale, original.scale));
}

// Inverse tests

TEST(TransformTest, InverseIdentity) {
    Transform t = Transform::identity();
    Transform inv = t.inverse();

    EXPECT_TRUE(almostEqual(inv, Transform::identity()));
}

TEST(TransformTest, InverseTranslation) {
    Transform t(Vec3(1.0f, 2.0f, 3.0f));
    Transform inv = t.inverse();

    EXPECT_TRUE(almostEqual(inv.position, Vec3(-1.0f, -2.0f, -3.0f)));
    EXPECT_TRUE(almostEqual(inv.rotation, Quat::identity()));
    EXPECT_TRUE(almostEqual(inv.scale, Vec3(1.0f, 1.0f, 1.0f)));
}

TEST(TransformTest, InverseRotation) {
    Quat rot = Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 2.0f);
    Transform t(Vec3::zero(), rot);
    Transform inv = t.inverse();

    Quat expectedRot = rot.conjugate();
    EXPECT_TRUE(almostEqual(inv.rotation, expectedRot));
}

TEST(TransformTest, InverseScale) {
    Transform t(Vec3::zero(), Quat::identity(), Vec3(2.0f, 4.0f, 8.0f));
    Transform inv = t.inverse();

    EXPECT_TRUE(almostEqual(inv.scale, Vec3(0.5f, 0.25f, 0.125f)));
}

TEST(TransformTest, InverseComposition) {
    Vec3 pos(1.0f, 2.0f, 3.0f);
    Quat rot = Quat::fromAxisAngle(Vec3::unitY(), PI<float> / 4.0f);
    Vec3 scl(2.0f, 2.0f, 2.0f);

    Transform t(pos, rot, scl);
    Transform inv = t.inverse();
    Transform composed = t * inv;

    // t * t^-1 should equal identity
    EXPECT_TRUE(almostEqual(composed.position, Vec3::zero(), 1e-4f));
    EXPECT_TRUE(almostEqual(composed.rotation, Quat::identity(), 1e-4f));
    EXPECT_TRUE(almostEqual(composed.scale, Vec3(1.0f, 1.0f, 1.0f), 1e-4f));
}

// Point transformation tests

TEST(TransformTest, TransformPointIdentity) {
    Transform t = Transform::identity();
    Vec3 point(1.0f, 2.0f, 3.0f);
    Vec3 transformed = t.transformPoint(point);

    EXPECT_TRUE(almostEqual(transformed, point));
}

TEST(TransformTest, TransformPointTranslation) {
    Transform t(Vec3(1.0f, 2.0f, 3.0f));
    Vec3 point(1.0f, 1.0f, 1.0f);
    Vec3 transformed = t.transformPoint(point);

    EXPECT_TRUE(almostEqual(transformed, Vec3(2.0f, 3.0f, 4.0f)));
}

TEST(TransformTest, TransformPointRotation) {
    Quat rot = Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 2.0f);
    Transform t(Vec3::zero(), rot);
    Vec3 point(1.0f, 0.0f, 0.0f);
    Vec3 transformed = t.transformPoint(point);

    EXPECT_TRUE(almostEqual(transformed, Vec3(0.0f, 1.0f, 0.0f)));
}

TEST(TransformTest, TransformPointScale) {
    Transform t(Vec3::zero(), Quat::identity(), Vec3(2.0f, 3.0f, 4.0f));
    Vec3 point(1.0f, 1.0f, 1.0f);
    Vec3 transformed = t.transformPoint(point);

    EXPECT_TRUE(almostEqual(transformed, Vec3(2.0f, 3.0f, 4.0f)));
}

// Direction transformation tests

TEST(TransformTest, TransformDirectionNoTranslation) {
    Transform t(Vec3(100.0f, 200.0f, 300.0f));  // Translation should be ignored
    Vec3 dir(1.0f, 0.0f, 0.0f);
    Vec3 transformed = t.transformDirection(dir);

    EXPECT_TRUE(almostEqual(transformed, dir));  // Should be unchanged
}

TEST(TransformTest, TransformDirectionRotation) {
    Quat rot = Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 2.0f);
    Transform t(Vec3::zero(), rot);
    Vec3 dir(1.0f, 0.0f, 0.0f);
    Vec3 transformed = t.transformDirection(dir);

    EXPECT_TRUE(almostEqual(transformed, Vec3(0.0f, 1.0f, 0.0f)));
}

TEST(TransformTest, TransformDirectionScale) {
    Transform t(Vec3::zero(), Quat::identity(), Vec3(2.0f, 3.0f, 4.0f));
    Vec3 dir(1.0f, 1.0f, 1.0f);
    Vec3 transformed = t.transformDirection(dir);

    EXPECT_TRUE(almostEqual(transformed, Vec3(2.0f, 3.0f, 4.0f)));
}

// Normal transformation tests

TEST(TransformTest, TransformNormalIdentity) {
    Transform t = Transform::identity();
    Vec3 normal(0.0f, 1.0f, 0.0f);
    Vec3 transformed = t.transformNormal(normal);

    EXPECT_TRUE(almostEqual(transformed, normal));
}

TEST(TransformTest, TransformNormalRotation) {
    Quat rot = Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 2.0f);
    Transform t(Vec3::zero(), rot);
    Vec3 normal(1.0f, 0.0f, 0.0f);
    Vec3 transformed = t.transformNormal(normal);

    EXPECT_TRUE(almostEqual(transformed, Vec3(0.0f, 1.0f, 0.0f)));
}

TEST(TransformTest, TransformNormalNonUniformScale) {
    // Non-uniform scale requires inverse transpose
    Transform t(Vec3::zero(), Quat::identity(), Vec3(2.0f, 1.0f, 1.0f));
    Vec3 normal(1.0f, 0.0f, 0.0f);
    Vec3 transformed = t.transformNormal(normal);

    // Normal should be normalized
    EXPECT_TRUE(almostEqual(transformed.length(), 1.0f));
}

// Inverse transformation tests

TEST(TransformTest, InverseTransformPointRoundTrip) {
    Vec3 pos(1.0f, 2.0f, 3.0f);
    Quat rot = Quat::fromAxisAngle(Vec3::unitY(), PI<float> / 3.0f);
    Vec3 scl(2.0f, 2.0f, 2.0f);
    Transform t(pos, rot, scl);

    Vec3 point(5.0f, 6.0f, 7.0f);
    Vec3 transformed = t.transformPoint(point);
    Vec3 roundTrip = t.inverseTransformPoint(transformed);

    EXPECT_TRUE(almostEqual(roundTrip, point, 1e-4f));
}

TEST(TransformTest, InverseTransformDirectionRoundTrip) {
    Quat rot = Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 4.0f);
    Vec3 scl(3.0f, 3.0f, 3.0f);
    Transform t(Vec3::zero(), rot, scl);

    Vec3 dir(1.0f, 0.0f, 0.0f);
    Vec3 transformed = t.transformDirection(dir);
    Vec3 roundTrip = t.inverseTransformDirection(transformed);

    EXPECT_TRUE(almostEqual(roundTrip, dir));
}

// Composition tests

TEST(TransformTest, CompositionIdentity) {
    Transform t(Vec3(1.0f, 2.0f, 3.0f), Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 4.0f),
                Vec3(2.0f, 2.0f, 2.0f));
    Transform identity = Transform::identity();

    Transform result = identity * t;
    EXPECT_TRUE(almostEqual(result, t));
}

TEST(TransformTest, CompositionTranslation) {
    Transform parent(Vec3(1.0f, 0.0f, 0.0f));
    Transform child(Vec3(0.0f, 1.0f, 0.0f));

    Transform combined = parent * child;
    EXPECT_TRUE(almostEqual(combined.position, Vec3(1.0f, 1.0f, 0.0f)));
}

TEST(TransformTest, CompositionRotation) {
    Quat rot1 = Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 4.0f);
    Quat rot2 = Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 4.0f);
    Transform parent(Vec3::zero(), rot1);
    Transform child(Vec3::zero(), rot2);

    Transform combined = parent * child;
    Quat expectedRot = Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 2.0f);

    EXPECT_TRUE(almostEqual(combined.rotation, expectedRot));
}

TEST(TransformTest, CompositionScale) {
    Transform parent(Vec3::zero(), Quat::identity(), Vec3(2.0f, 2.0f, 2.0f));
    Transform child(Vec3::zero(), Quat::identity(), Vec3(3.0f, 3.0f, 3.0f));

    Transform combined = parent * child;
    EXPECT_TRUE(almostEqual(combined.scale, Vec3(6.0f, 6.0f, 6.0f)));
}

TEST(TransformTest, CompositionHierarchy) {
    // Test parent-child hierarchy
    Transform parent(Vec3(10.0f, 0.0f, 0.0f), Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 2.0f),
                     Vec3(2.0f, 2.0f, 2.0f));
    Transform child(Vec3(5.0f, 0.0f, 0.0f));

    Transform world = parent * child;

    // Child at (5,0,0) in parent space:
    // - Scaled by (2,2,2) -> (10,0,0)
    // - Rotated 90deg around Z -> (0,10,0)
    // - Translated by (10,0,0) -> (10,10,0)
    EXPECT_TRUE(almostEqual(world.position, Vec3(10.0f, 10.0f, 0.0f)));
}

TEST(TransformTest, CompositionPointTransform) {
    Transform parent(Vec3(1.0f, 0.0f, 0.0f));
    Transform child(Vec3(1.0f, 0.0f, 0.0f));
    Transform combined = parent * child;

    Vec3 point(1.0f, 0.0f, 0.0f);
    Vec3 transformed1 = combined.transformPoint(point);
    Vec3 transformed2 = parent.transformPoint(child.transformPoint(point));

    EXPECT_TRUE(almostEqual(transformed1, transformed2));
}

// Comparison operator tests

TEST(TransformTest, EqualityOperator) {
    Transform t1(Vec3(1.0f, 2.0f, 3.0f), Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 4.0f),
                 Vec3(2.0f, 2.0f, 2.0f));
    Transform t2(Vec3(1.0f, 2.0f, 3.0f), Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 4.0f),
                 Vec3(2.0f, 2.0f, 2.0f));

    EXPECT_TRUE(t1 == t2);
}

TEST(TransformTest, InequalityOperator) {
    Transform t1(Vec3(1.0f, 2.0f, 3.0f));
    Transform t2(Vec3(1.0f, 2.0f, 4.0f));

    EXPECT_TRUE(t1 != t2);
}

// Numerical precision tests

TEST(TransformTest, NumericalPrecision) {
    // Test that operations maintain precision within epsilon
    Vec3 pos(1.234567f, 2.345678f, 3.456789f);
    Quat rot = Quat::fromAxisAngle(Vec3::unitY(), 1.23456f);
    Vec3 scl(1.5f, 2.5f, 3.5f);

    Transform t(pos, rot, scl);
    Mat4 m = t.toMatrix();
    Transform roundTrip = Transform::fromMatrix(m);

    // Precision should be within 1e-5
    EXPECT_TRUE(almostEqual(roundTrip.position, pos, 1e-5f));
    EXPECT_TRUE(almostEqual(roundTrip.rotation, rot, 1e-5f));
    EXPECT_TRUE(almostEqual(roundTrip.scale, scl, 1e-5f));
}
