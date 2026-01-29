#include "axiom/math/vec2.hpp"
#include "axiom/math/vec3.hpp"
#include "axiom/math/vec4.hpp"

#include <gtest/gtest.h>

using namespace axiom::math;

// Vec2 Tests

TEST(Vec2Test, DefaultConstructor) {
    Vec2 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
}

TEST(Vec2Test, ComponentConstructor) {
    Vec2 v(1.0f, 2.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
}

TEST(Vec2Test, ScalarConstructor) {
    Vec2 v(5.0f);
    EXPECT_FLOAT_EQ(v.x, 5.0f);
    EXPECT_FLOAT_EQ(v.y, 5.0f);
}

TEST(Vec2Test, ArrayAccess) {
    Vec2 v(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v[0], 3.0f);
    EXPECT_FLOAT_EQ(v[1], 4.0f);

    v[0] = 7.0f;
    v[1] = 8.0f;
    EXPECT_FLOAT_EQ(v.x, 7.0f);
    EXPECT_FLOAT_EQ(v.y, 8.0f);
}

TEST(Vec2Test, Addition) {
    Vec2 a(1.0f, 2.0f);
    Vec2 b(3.0f, 4.0f);
    Vec2 c = a + b;
    EXPECT_FLOAT_EQ(c.x, 4.0f);
    EXPECT_FLOAT_EQ(c.y, 6.0f);
}

TEST(Vec2Test, Subtraction) {
    Vec2 a(5.0f, 7.0f);
    Vec2 b(2.0f, 3.0f);
    Vec2 c = a - b;
    EXPECT_FLOAT_EQ(c.x, 3.0f);
    EXPECT_FLOAT_EQ(c.y, 4.0f);
}

TEST(Vec2Test, MultiplicationByVector) {
    Vec2 a(2.0f, 3.0f);
    Vec2 b(4.0f, 5.0f);
    Vec2 c = a * b;
    EXPECT_FLOAT_EQ(c.x, 8.0f);
    EXPECT_FLOAT_EQ(c.y, 15.0f);
}

TEST(Vec2Test, DivisionByVector) {
    Vec2 a(12.0f, 15.0f);
    Vec2 b(3.0f, 5.0f);
    Vec2 c = a / b;
    EXPECT_FLOAT_EQ(c.x, 4.0f);
    EXPECT_FLOAT_EQ(c.y, 3.0f);
}

TEST(Vec2Test, ScalarMultiplication) {
    Vec2 v(2.0f, 3.0f);
    Vec2 c = v * 4.0f;
    EXPECT_FLOAT_EQ(c.x, 8.0f);
    EXPECT_FLOAT_EQ(c.y, 12.0f);

    // Test scalar * vector
    Vec2 d = 4.0f * v;
    EXPECT_FLOAT_EQ(d.x, 8.0f);
    EXPECT_FLOAT_EQ(d.y, 12.0f);
}

TEST(Vec2Test, ScalarDivision) {
    Vec2 v(8.0f, 12.0f);
    Vec2 c = v / 4.0f;
    EXPECT_FLOAT_EQ(c.x, 2.0f);
    EXPECT_FLOAT_EQ(c.y, 3.0f);
}

TEST(Vec2Test, UnaryNegation) {
    Vec2 v(3.0f, -4.0f);
    Vec2 c = -v;
    EXPECT_FLOAT_EQ(c.x, -3.0f);
    EXPECT_FLOAT_EQ(c.y, 4.0f);
}

TEST(Vec2Test, CompoundAddition) {
    Vec2 v(1.0f, 2.0f);
    v += Vec2(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.x, 4.0f);
    EXPECT_FLOAT_EQ(v.y, 6.0f);
}

TEST(Vec2Test, Equality) {
    Vec2 a(1.0f, 2.0f);
    Vec2 b(1.0f, 2.0f);
    Vec2 c(1.0f, 3.0f);

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(Vec2Test, StaticUtilityFunctions) {
    Vec2 zero = Vec2::zero();
    EXPECT_FLOAT_EQ(zero.x, 0.0f);
    EXPECT_FLOAT_EQ(zero.y, 0.0f);

    Vec2 one = Vec2::one();
    EXPECT_FLOAT_EQ(one.x, 1.0f);
    EXPECT_FLOAT_EQ(one.y, 1.0f);

    Vec2 unitX = Vec2::unitX();
    EXPECT_FLOAT_EQ(unitX.x, 1.0f);
    EXPECT_FLOAT_EQ(unitX.y, 0.0f);

    Vec2 unitY = Vec2::unitY();
    EXPECT_FLOAT_EQ(unitY.x, 0.0f);
    EXPECT_FLOAT_EQ(unitY.y, 1.0f);
}

// Vec3 Tests

TEST(Vec3Test, DefaultConstructor) {
    Vec3 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
}

TEST(Vec3Test, ComponentConstructor) {
    Vec3 v(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
    EXPECT_FLOAT_EQ(v.z, 3.0f);
}

TEST(Vec3Test, ScalarConstructor) {
    Vec3 v(5.0f);
    EXPECT_FLOAT_EQ(v.x, 5.0f);
    EXPECT_FLOAT_EQ(v.y, 5.0f);
    EXPECT_FLOAT_EQ(v.z, 5.0f);
}

TEST(Vec3Test, ArrayAccess) {
    Vec3 v(3.0f, 4.0f, 5.0f);
    EXPECT_FLOAT_EQ(v[0], 3.0f);
    EXPECT_FLOAT_EQ(v[1], 4.0f);
    EXPECT_FLOAT_EQ(v[2], 5.0f);

    v[0] = 7.0f;
    v[1] = 8.0f;
    v[2] = 9.0f;
    EXPECT_FLOAT_EQ(v.x, 7.0f);
    EXPECT_FLOAT_EQ(v.y, 8.0f);
    EXPECT_FLOAT_EQ(v.z, 9.0f);
}

TEST(Vec3Test, Addition) {
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);
    Vec3 c = a + b;
    EXPECT_FLOAT_EQ(c.x, 5.0f);
    EXPECT_FLOAT_EQ(c.y, 7.0f);
    EXPECT_FLOAT_EQ(c.z, 9.0f);
}

TEST(Vec3Test, Subtraction) {
    Vec3 a(5.0f, 7.0f, 9.0f);
    Vec3 b(2.0f, 3.0f, 4.0f);
    Vec3 c = a - b;
    EXPECT_FLOAT_EQ(c.x, 3.0f);
    EXPECT_FLOAT_EQ(c.y, 4.0f);
    EXPECT_FLOAT_EQ(c.z, 5.0f);
}

TEST(Vec3Test, ScalarMultiplication) {
    Vec3 v(2.0f, 3.0f, 4.0f);
    Vec3 c = v * 2.0f;
    EXPECT_FLOAT_EQ(c.x, 4.0f);
    EXPECT_FLOAT_EQ(c.y, 6.0f);
    EXPECT_FLOAT_EQ(c.z, 8.0f);

    // Test scalar * vector
    Vec3 d = 2.0f * v;
    EXPECT_FLOAT_EQ(d.x, 4.0f);
    EXPECT_FLOAT_EQ(d.y, 6.0f);
    EXPECT_FLOAT_EQ(d.z, 8.0f);
}

TEST(Vec3Test, DotProduct) {
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);

    float dotResult = a.dot(b);
    EXPECT_FLOAT_EQ(dotResult, 32.0f);  // 1*4 + 2*5 + 3*6 = 32

    // Test free function
    float dotResult2 = dot(a, b);
    EXPECT_FLOAT_EQ(dotResult2, 32.0f);
}

TEST(Vec3Test, CrossProduct) {
    Vec3 a(1.0f, 0.0f, 0.0f);
    Vec3 b(0.0f, 1.0f, 0.0f);

    Vec3 c = a.cross(b);
    EXPECT_FLOAT_EQ(c.x, 0.0f);
    EXPECT_FLOAT_EQ(c.y, 0.0f);
    EXPECT_FLOAT_EQ(c.z, 1.0f);

    // Test free function
    Vec3 d = cross(a, b);
    EXPECT_FLOAT_EQ(d.x, 0.0f);
    EXPECT_FLOAT_EQ(d.y, 0.0f);
    EXPECT_FLOAT_EQ(d.z, 1.0f);

    // Test anti-commutativity
    Vec3 e = b.cross(a);
    EXPECT_FLOAT_EQ(e.x, 0.0f);
    EXPECT_FLOAT_EQ(e.y, 0.0f);
    EXPECT_FLOAT_EQ(e.z, -1.0f);
}

TEST(Vec3Test, LengthAndNormalization) {
    Vec3 v(3.0f, 4.0f, 0.0f);

    EXPECT_FLOAT_EQ(v.lengthSquared(), 25.0f);
    EXPECT_FLOAT_EQ(v.length(), 5.0f);

    Vec3 normalized = v.normalized();
    EXPECT_FLOAT_EQ(normalized.x, 0.6f);
    EXPECT_FLOAT_EQ(normalized.y, 0.8f);
    EXPECT_FLOAT_EQ(normalized.z, 0.0f);
    EXPECT_NEAR(normalized.length(), 1.0f, 1e-6f);

    // Test in-place normalization
    Vec3 v2(3.0f, 4.0f, 0.0f);
    v2.normalize();
    EXPECT_FLOAT_EQ(v2.x, 0.6f);
    EXPECT_FLOAT_EQ(v2.y, 0.8f);
    EXPECT_FLOAT_EQ(v2.z, 0.0f);
}

TEST(Vec3Test, StaticUtilityFunctions) {
    Vec3 zero = Vec3::zero();
    EXPECT_FLOAT_EQ(zero.x, 0.0f);
    EXPECT_FLOAT_EQ(zero.y, 0.0f);
    EXPECT_FLOAT_EQ(zero.z, 0.0f);

    Vec3 one = Vec3::one();
    EXPECT_FLOAT_EQ(one.x, 1.0f);
    EXPECT_FLOAT_EQ(one.y, 1.0f);
    EXPECT_FLOAT_EQ(one.z, 1.0f);

    Vec3 unitX = Vec3::unitX();
    EXPECT_FLOAT_EQ(unitX.x, 1.0f);
    EXPECT_FLOAT_EQ(unitX.y, 0.0f);
    EXPECT_FLOAT_EQ(unitX.z, 0.0f);

    Vec3 unitY = Vec3::unitY();
    EXPECT_FLOAT_EQ(unitY.x, 0.0f);
    EXPECT_FLOAT_EQ(unitY.y, 1.0f);
    EXPECT_FLOAT_EQ(unitY.z, 0.0f);

    Vec3 unitZ = Vec3::unitZ();
    EXPECT_FLOAT_EQ(unitZ.x, 0.0f);
    EXPECT_FLOAT_EQ(unitZ.y, 0.0f);
    EXPECT_FLOAT_EQ(unitZ.z, 1.0f);
}

// Vec4 Tests

TEST(Vec4Test, DefaultConstructor) {
    Vec4 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
    EXPECT_FLOAT_EQ(v.w, 0.0f);
}

TEST(Vec4Test, ComponentConstructor) {
    Vec4 v(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
    EXPECT_FLOAT_EQ(v.z, 3.0f);
    EXPECT_FLOAT_EQ(v.w, 4.0f);
}

TEST(Vec4Test, ScalarConstructor) {
    Vec4 v(5.0f);
    EXPECT_FLOAT_EQ(v.x, 5.0f);
    EXPECT_FLOAT_EQ(v.y, 5.0f);
    EXPECT_FLOAT_EQ(v.z, 5.0f);
    EXPECT_FLOAT_EQ(v.w, 5.0f);
}

TEST(Vec4Test, ArrayAccess) {
    Vec4 v(3.0f, 4.0f, 5.0f, 6.0f);
    EXPECT_FLOAT_EQ(v[0], 3.0f);
    EXPECT_FLOAT_EQ(v[1], 4.0f);
    EXPECT_FLOAT_EQ(v[2], 5.0f);
    EXPECT_FLOAT_EQ(v[3], 6.0f);

    v[0] = 7.0f;
    v[1] = 8.0f;
    v[2] = 9.0f;
    v[3] = 10.0f;
    EXPECT_FLOAT_EQ(v.x, 7.0f);
    EXPECT_FLOAT_EQ(v.y, 8.0f);
    EXPECT_FLOAT_EQ(v.z, 9.0f);
    EXPECT_FLOAT_EQ(v.w, 10.0f);
}

TEST(Vec4Test, Addition) {
    Vec4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4 b(5.0f, 6.0f, 7.0f, 8.0f);
    Vec4 c = a + b;
    EXPECT_FLOAT_EQ(c.x, 6.0f);
    EXPECT_FLOAT_EQ(c.y, 8.0f);
    EXPECT_FLOAT_EQ(c.z, 10.0f);
    EXPECT_FLOAT_EQ(c.w, 12.0f);
}

TEST(Vec4Test, ScalarMultiplication) {
    Vec4 v(2.0f, 3.0f, 4.0f, 5.0f);
    Vec4 c = v * 2.0f;
    EXPECT_FLOAT_EQ(c.x, 4.0f);
    EXPECT_FLOAT_EQ(c.y, 6.0f);
    EXPECT_FLOAT_EQ(c.z, 8.0f);
    EXPECT_FLOAT_EQ(c.w, 10.0f);

    // Test scalar * vector
    Vec4 d = 2.0f * v;
    EXPECT_FLOAT_EQ(d.x, 4.0f);
    EXPECT_FLOAT_EQ(d.y, 6.0f);
    EXPECT_FLOAT_EQ(d.z, 8.0f);
    EXPECT_FLOAT_EQ(d.w, 10.0f);
}

TEST(Vec4Test, DotProduct) {
    Vec4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4 b(5.0f, 6.0f, 7.0f, 8.0f);

    float dotResult = a.dot(b);
    EXPECT_FLOAT_EQ(dotResult, 70.0f);  // 1*5 + 2*6 + 3*7 + 4*8 = 70

    // Test free function
    float dotResult2 = dot(a, b);
    EXPECT_FLOAT_EQ(dotResult2, 70.0f);
}

TEST(Vec4Test, LengthAndNormalization) {
    Vec4 v(2.0f, 3.0f, 6.0f, 0.0f);

    EXPECT_FLOAT_EQ(v.lengthSquared(), 49.0f);
    EXPECT_FLOAT_EQ(v.length(), 7.0f);

    Vec4 normalized = v.normalized();
    EXPECT_NEAR(normalized.length(), 1.0f, 1e-6f);

    // Test in-place normalization
    Vec4 v2(2.0f, 3.0f, 6.0f, 0.0f);
    v2.normalize();
    EXPECT_NEAR(v2.length(), 1.0f, 1e-6f);
}

TEST(Vec4Test, StaticUtilityFunctions) {
    Vec4 zero = Vec4::zero();
    EXPECT_FLOAT_EQ(zero.x, 0.0f);
    EXPECT_FLOAT_EQ(zero.y, 0.0f);
    EXPECT_FLOAT_EQ(zero.z, 0.0f);
    EXPECT_FLOAT_EQ(zero.w, 0.0f);

    Vec4 one = Vec4::one();
    EXPECT_FLOAT_EQ(one.x, 1.0f);
    EXPECT_FLOAT_EQ(one.y, 1.0f);
    EXPECT_FLOAT_EQ(one.z, 1.0f);
    EXPECT_FLOAT_EQ(one.w, 1.0f);

    Vec4 unitX = Vec4::unitX();
    EXPECT_FLOAT_EQ(unitX.x, 1.0f);
    EXPECT_FLOAT_EQ(unitX.y, 0.0f);
    EXPECT_FLOAT_EQ(unitX.z, 0.0f);
    EXPECT_FLOAT_EQ(unitX.w, 0.0f);

    Vec4 unitW = Vec4::unitW();
    EXPECT_FLOAT_EQ(unitW.x, 0.0f);
    EXPECT_FLOAT_EQ(unitW.y, 0.0f);
    EXPECT_FLOAT_EQ(unitW.z, 0.0f);
    EXPECT_FLOAT_EQ(unitW.w, 1.0f);
}
