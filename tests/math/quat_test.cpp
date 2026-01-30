#include "axiom/math/constants.hpp"
#include "axiom/math/mat4.hpp"
#include "axiom/math/quat.hpp"
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
}  // namespace

// Constructor tests

TEST(QuatTest, DefaultConstructor) {
    Quat q;
    // Should initialize to identity quaternion
    EXPECT_FLOAT_EQ(q.x, 0.0f);
    EXPECT_FLOAT_EQ(q.y, 0.0f);
    EXPECT_FLOAT_EQ(q.z, 0.0f);
    EXPECT_FLOAT_EQ(q.w, 1.0f);
}

TEST(QuatTest, ComponentConstructor) {
    Quat q(0.5f, 0.5f, 0.5f, 0.5f);
    EXPECT_FLOAT_EQ(q.x, 0.5f);
    EXPECT_FLOAT_EQ(q.y, 0.5f);
    EXPECT_FLOAT_EQ(q.z, 0.5f);
    EXPECT_FLOAT_EQ(q.w, 0.5f);
}

TEST(QuatTest, ArrayAccess) {
    Quat q(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(q[0], 1.0f);
    EXPECT_FLOAT_EQ(q[1], 2.0f);
    EXPECT_FLOAT_EQ(q[2], 3.0f);
    EXPECT_FLOAT_EQ(q[3], 4.0f);

    q[0] = 5.0f;
    EXPECT_FLOAT_EQ(q.x, 5.0f);
}

// Factory method tests

TEST(QuatTest, Identity) {
    Quat q = Quat::identity();
    EXPECT_FLOAT_EQ(q.x, 0.0f);
    EXPECT_FLOAT_EQ(q.y, 0.0f);
    EXPECT_FLOAT_EQ(q.z, 0.0f);
    EXPECT_FLOAT_EQ(q.w, 1.0f);
}

TEST(QuatTest, FromAxisAngle) {
    // 90 degree rotation around Z axis
    Vec3 axis(0.0f, 0.0f, 1.0f);
    float angle = PI<float> / 2.0f;
    Quat q = Quat::fromAxisAngle(axis, angle);

    // Verify by rotating a vector
    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 rotated = q * v;

    EXPECT_TRUE(almostEqual(rotated.x, 0.0f));
    EXPECT_TRUE(almostEqual(rotated.y, 1.0f));
    EXPECT_TRUE(almostEqual(rotated.z, 0.0f));
}

TEST(QuatTest, FromAxisAngleXAxis) {
    // 90 degree rotation around X axis
    Vec3 axis(1.0f, 0.0f, 0.0f);
    float angle = PI<float> / 2.0f;
    Quat q = Quat::fromAxisAngle(axis, angle);

    // Rotate (0, 1, 0) around X should give (0, 0, 1)
    Vec3 v(0.0f, 1.0f, 0.0f);
    Vec3 rotated = q * v;

    EXPECT_TRUE(almostEqual(rotated.x, 0.0f));
    EXPECT_TRUE(almostEqual(rotated.y, 0.0f));
    EXPECT_TRUE(almostEqual(rotated.z, 1.0f));
}

TEST(QuatTest, FromEuler) {
    // Simple rotation around Y axis
    float pitch = 0.0f;
    float yaw = PI<float> / 2.0f;  // 90 degrees
    float roll = 0.0f;
    Quat q = Quat::fromEuler(pitch, yaw, roll);

    // Rotate (1, 0, 0) should give approximately (0, 0, -1)
    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 rotated = q * v;

    EXPECT_TRUE(almostEqual(rotated.x, 0.0f));
    EXPECT_TRUE(almostEqual(rotated.y, 0.0f));
    EXPECT_TRUE(almostEqual(rotated.z, -1.0f));
}

TEST(QuatTest, FromMatrix) {
    // Create a rotation matrix for 90 degrees around Z
    Mat4 mat = Mat4::rotationZ(PI<float> / 2.0f);
    Quat q = Quat::fromMatrix(mat);

    // Verify by rotating a vector
    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 rotated = q * v;

    EXPECT_TRUE(almostEqual(rotated.x, 0.0f));
    EXPECT_TRUE(almostEqual(rotated.y, 1.0f));
    EXPECT_TRUE(almostEqual(rotated.z, 0.0f));
}

// Operation tests

TEST(QuatTest, Conjugate) {
    Quat q(1.0f, 2.0f, 3.0f, 4.0f);
    Quat conj = q.conjugate();

    EXPECT_FLOAT_EQ(conj.x, -1.0f);
    EXPECT_FLOAT_EQ(conj.y, -2.0f);
    EXPECT_FLOAT_EQ(conj.z, -3.0f);
    EXPECT_FLOAT_EQ(conj.w, 4.0f);
}

TEST(QuatTest, Length) {
    Quat q(1.0f, 2.0f, 2.0f, 0.0f);
    float len = q.length();
    EXPECT_FLOAT_EQ(len, 3.0f);  // sqrt(1 + 4 + 4) = 3
}

TEST(QuatTest, LengthSquared) {
    Quat q(1.0f, 2.0f, 2.0f, 0.0f);
    float lenSq = q.lengthSquared();
    EXPECT_FLOAT_EQ(lenSq, 9.0f);  // 1 + 4 + 4 = 9
}

TEST(QuatTest, Normalized) {
    Quat q(1.0f, 2.0f, 2.0f, 0.0f);
    Quat normalized = q.normalized();

    float len = normalized.length();
    EXPECT_TRUE(almostEqual(len, 1.0f));

    // Original should be unchanged
    EXPECT_FLOAT_EQ(q.x, 1.0f);
}

TEST(QuatTest, Normalize) {
    Quat q(1.0f, 2.0f, 2.0f, 0.0f);
    q.normalize();

    float len = q.length();
    EXPECT_TRUE(almostEqual(len, 1.0f));
}

TEST(QuatTest, Dot) {
    Quat a(1.0f, 0.0f, 0.0f, 0.0f);
    Quat b(0.0f, 1.0f, 0.0f, 0.0f);
    Quat c(1.0f, 0.0f, 0.0f, 0.0f);

    EXPECT_FLOAT_EQ(a.dot(b), 0.0f);
    EXPECT_FLOAT_EQ(a.dot(c), 1.0f);
}

TEST(QuatTest, Inverse) {
    Quat q = Quat::fromAxisAngle(Vec3(0.0f, 0.0f, 1.0f), PI<float> / 4.0f);
    Quat inv = q.inverse();

    // q * q^-1 should be identity
    Quat result = q * inv;

    EXPECT_TRUE(almostEqual(result.x, 0.0f));
    EXPECT_TRUE(almostEqual(result.y, 0.0f));
    EXPECT_TRUE(almostEqual(result.z, 0.0f));
    EXPECT_TRUE(almostEqual(result.w, 1.0f));
}

TEST(QuatTest, Multiplication) {
    // Rotate 90 degrees around Z, then 90 degrees around X
    Quat qz = Quat::fromAxisAngle(Vec3(0.0f, 0.0f, 1.0f), PI<float> / 2.0f);
    Quat qx = Quat::fromAxisAngle(Vec3(1.0f, 0.0f, 0.0f), PI<float> / 2.0f);

    Quat combined = qx * qz;  // Apply qz first, then qx

    // Test on a vector
    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 rotated = combined * v;

    // (1,0,0) -> rotZ90 -> (0,1,0) -> rotX90 -> (0,0,1)
    EXPECT_TRUE(almostEqual(rotated.x, 0.0f));
    EXPECT_TRUE(almostEqual(rotated.y, 0.0f));
    EXPECT_TRUE(almostEqual(rotated.z, 1.0f));
}

TEST(QuatTest, VectorRotation) {
    // 180 degree rotation around Z
    Quat q = Quat::fromAxisAngle(Vec3(0.0f, 0.0f, 1.0f), PI<float>);
    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 rotated = q * v;

    EXPECT_TRUE(almostEqual(rotated.x, -1.0f));
    EXPECT_TRUE(almostEqual(rotated.y, 0.0f));
    EXPECT_TRUE(almostEqual(rotated.z, 0.0f));
}

// Interpolation tests

TEST(QuatTest, Slerp) {
    Quat a = Quat::identity();
    Quat b = Quat::fromAxisAngle(Vec3(0.0f, 0.0f, 1.0f), PI<float> / 2.0f);

    // At t=0, should be a
    Quat result0 = Quat::slerp(a, b, 0.0f);
    EXPECT_TRUE(almostEqual(result0, a));

    // At t=1, should be b
    Quat result1 = Quat::slerp(a, b, 1.0f);
    EXPECT_TRUE(almostEqual(result1, b));

    // At t=0.5, should be halfway
    Quat result05 = Quat::slerp(a, b, 0.5f);
    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 rotated = result05 * v;

    // 45 degree rotation
    float cos45 = std::cos(PI<float> / 4.0f);
    float sin45 = std::sin(PI<float> / 4.0f);
    EXPECT_TRUE(almostEqual(rotated.x, cos45));
    EXPECT_TRUE(almostEqual(rotated.y, sin45));
    EXPECT_TRUE(almostEqual(rotated.z, 0.0f));
}

TEST(QuatTest, Nlerp) {
    Quat a = Quat::identity();
    Quat b = Quat::fromAxisAngle(Vec3(0.0f, 0.0f, 1.0f), PI<float> / 2.0f);

    // At t=0, should be close to a
    Quat result0 = Quat::nlerp(a, b, 0.0f);
    EXPECT_TRUE(almostEqual(result0, a));

    // At t=1, should be close to b
    Quat result1 = Quat::nlerp(a, b, 1.0f);
    EXPECT_TRUE(almostEqual(result1, b));

    // Result should be normalized
    Quat result05 = Quat::nlerp(a, b, 0.5f);
    EXPECT_TRUE(almostEqual(result05.length(), 1.0f));
}

// Conversion tests

TEST(QuatTest, ToMatrix) {
    Quat q = Quat::fromAxisAngle(Vec3(0.0f, 0.0f, 1.0f), PI<float> / 2.0f);
    Mat4 mat = q.toMatrix();

    // Use matrix to rotate a point
    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 rotated = mat.transformVector(v);

    EXPECT_TRUE(almostEqual(rotated.x, 0.0f));
    EXPECT_TRUE(almostEqual(rotated.y, 1.0f));
    EXPECT_TRUE(almostEqual(rotated.z, 0.0f));
}

TEST(QuatTest, ToAxisAngle) {
    Vec3 originalAxis(0.0f, 1.0f, 0.0f);
    float originalAngle = PI<float> / 3.0f;  // 60 degrees

    Quat q = Quat::fromAxisAngle(originalAxis, originalAngle);

    Vec3 axis;
    float angle;
    q.toAxisAngle(axis, angle);

    EXPECT_TRUE(almostEqual(axis, originalAxis));
    EXPECT_TRUE(almostEqual(angle, originalAngle));
}

TEST(QuatTest, ToEuler) {
    // Create quaternion from Euler angles
    float originalPitch = 0.3f;
    float originalYaw = 0.5f;
    float originalRoll = 0.7f;

    Quat q = Quat::fromEuler(originalPitch, originalYaw, originalRoll);

    // Convert back to Euler
    float pitch, yaw, roll;
    q.toEuler(pitch, yaw, roll);

    // Note: Euler angles may have multiple representations, so we verify by rotation
    Quat qReconstructed = Quat::fromEuler(pitch, yaw, roll);
    EXPECT_TRUE(almostEqual(q, qReconstructed, 1e-4f));
}

// Gimbal lock tests

TEST(QuatTest, GimbalLockAvoidance) {
    // Test rotation sequence that would cause gimbal lock with Euler angles
    // Pitch = 90 degrees (pointing straight up)
    float pitch = PI<float> / 2.0f;
    float yaw = 0.3f;
    float roll = 0.5f;

    Quat q = Quat::fromEuler(pitch, yaw, roll);

    // Quaternion should handle this without issue
    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 rotated = q * v;

    // Should produce a valid rotation (not NaN or infinite)
    EXPECT_FALSE(std::isnan(rotated.x));
    EXPECT_FALSE(std::isnan(rotated.y));
    EXPECT_FALSE(std::isnan(rotated.z));
    EXPECT_FALSE(std::isinf(rotated.x));
    EXPECT_FALSE(std::isinf(rotated.y));
    EXPECT_FALSE(std::isinf(rotated.z));
}

TEST(QuatTest, ContinuousRotation) {
    // Test that multiple small rotations compose correctly (no drift)
    Quat q = Quat::fromAxisAngle(Vec3(0.0f, 0.0f, 1.0f), PI<float> / 180.0f);  // 1 degree

    Quat accumulated = Quat::identity();
    for (int i = 0; i < 90; ++i) {
        accumulated = q * accumulated;
    }

    // After 90 rotations of 1 degree each, should be 90 degrees total
    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 rotated = accumulated * v;

    EXPECT_TRUE(almostEqual(rotated.x, 0.0f, 1e-4f));
    EXPECT_TRUE(almostEqual(rotated.y, 1.0f, 1e-4f));
    EXPECT_TRUE(almostEqual(rotated.z, 0.0f, 1e-4f));
}

// Comparison tests

TEST(QuatTest, Equality) {
    Quat a(1.0f, 2.0f, 3.0f, 4.0f);
    Quat b(1.0f, 2.0f, 3.0f, 4.0f);

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(QuatTest, Inequality) {
    Quat a(1.0f, 2.0f, 3.0f, 4.0f);
    Quat b(1.0f, 2.0f, 3.0f, 5.0f);

    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a == b);
}

// Edge case tests

TEST(QuatTest, IdentityRotation) {
    Quat identity = Quat::identity();
    Vec3 v(1.0f, 2.0f, 3.0f);
    Vec3 rotated = identity * v;

    EXPECT_TRUE(almostEqual(rotated, v));
}

TEST(QuatTest, ZeroAngleRotation) {
    Quat q = Quat::fromAxisAngle(Vec3(1.0f, 0.0f, 0.0f), 0.0f);

    // Should be identity
    EXPECT_TRUE(almostEqual(q.x, 0.0f));
    EXPECT_TRUE(almostEqual(q.y, 0.0f));
    EXPECT_TRUE(almostEqual(q.z, 0.0f));
    EXPECT_TRUE(almostEqual(q.w, 1.0f));
}

TEST(QuatTest, FullRotation) {
    // 360 degree rotation should be close to identity
    Quat q = Quat::fromAxisAngle(Vec3(0.0f, 0.0f, 1.0f), 2.0f * PI<float>);

    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 rotated = q * v;

    EXPECT_TRUE(almostEqual(rotated.x, 1.0f, 1e-4f));
    EXPECT_TRUE(almostEqual(rotated.y, 0.0f, 1e-4f));
    EXPECT_TRUE(almostEqual(rotated.z, 0.0f, 1e-4f));
}
