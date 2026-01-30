#include "axiom/math/constants.hpp"
#include "axiom/math/mat4.hpp"
#include "axiom/math/quat.hpp"
#include "axiom/math/vec3.hpp"

#include <gtest/gtest.h>

#include <cmath>

using namespace axiom::math;

namespace {
constexpr float TEST_EPSILON = 1e-5f;

bool almostEqual(float a, float b, float epsilon = TEST_EPSILON) {
    return std::fabs(a - b) < epsilon;
}

bool almostEqual(const Vec3& a, const Vec3& b, float epsilon = TEST_EPSILON) {
    return almostEqual(a.x, b.x, epsilon) && almostEqual(a.y, b.y, epsilon) &&
           almostEqual(a.z, b.z, epsilon);
}
}  // namespace

TEST(Mat4QuatIntegrationTest, QuatToMatrixRotation) {
    // Create quaternion for 90 degree rotation around Z
    Quat q = Quat::fromAxisAngle(Vec3(0.0f, 0.0f, 1.0f), PI<float> / 2.0f);

    // Convert to matrix using Mat4::rotation
    Mat4 mat = Mat4::rotation(q);

    // Test rotation
    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 rotated = mat.transformVector(v);

    EXPECT_TRUE(almostEqual(rotated.x, 0.0f));
    EXPECT_TRUE(almostEqual(rotated.y, 1.0f));
    EXPECT_TRUE(almostEqual(rotated.z, 0.0f));
}

TEST(Mat4QuatIntegrationTest, QuatMatrixRoundTrip) {
    // Create quaternion
    Quat q1 = Quat::fromAxisAngle(Vec3(1.0f, 1.0f, 1.0f).normalized(), 0.7f);

    // Convert to matrix
    Mat4 mat = Mat4::rotation(q1);

    // Rotate a vector with both quaternion and matrix
    Vec3 v(1.0f, 2.0f, 3.0f);
    Vec3 rotated1 = q1 * v;
    Vec3 rotated2 = mat.transformVector(v);

    // Both should produce the same result
    EXPECT_TRUE(almostEqual(rotated1, rotated2));
}
