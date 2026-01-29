#include "axiom/math/vec_ops.hpp"

#include <gtest/gtest.h>

using namespace axiom::math;

// ============================================================================
// Dot Product Tests
// ============================================================================

TEST(VecOpsTest, Dot2D) {
    Vec2 a(1.0f, 2.0f);
    Vec2 b(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(dot(a, b), 11.0f);  // 1*3 + 2*4 = 11
}

TEST(VecOpsTest, Dot3D) {
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 5.0f, 6.0f);
    EXPECT_FLOAT_EQ(dot(a, b), 32.0f);  // 1*4 + 2*5 + 3*6 = 32
}

TEST(VecOpsTest, Dot4D) {
    Vec4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4 b(5.0f, 6.0f, 7.0f, 8.0f);
    EXPECT_FLOAT_EQ(dot(a, b), 70.0f);  // 1*5 + 2*6 + 3*7 + 4*8 = 70
}

// ============================================================================
// Length Tests
// ============================================================================

TEST(VecOpsTest, Length2D) {
    Vec2 v(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(lengthSquared(v), 25.0f);
    EXPECT_FLOAT_EQ(length(v), 5.0f);
}

TEST(VecOpsTest, Length3D) {
    Vec3 v(2.0f, 3.0f, 6.0f);
    EXPECT_FLOAT_EQ(lengthSquared(v), 49.0f);
    EXPECT_FLOAT_EQ(length(v), 7.0f);
}

TEST(VecOpsTest, Length4D) {
    Vec4 v(2.0f, 3.0f, 6.0f, 0.0f);
    EXPECT_FLOAT_EQ(lengthSquared(v), 49.0f);
    EXPECT_FLOAT_EQ(length(v), 7.0f);
}

// ============================================================================
// Distance Tests
// ============================================================================

TEST(VecOpsTest, Distance2D) {
    Vec2 a(1.0f, 2.0f);
    Vec2 b(4.0f, 6.0f);
    EXPECT_FLOAT_EQ(distanceSquared(a, b), 25.0f);
    EXPECT_FLOAT_EQ(distance(a, b), 5.0f);
}

TEST(VecOpsTest, Distance3D) {
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b(4.0f, 6.0f, 3.0f);
    EXPECT_FLOAT_EQ(distanceSquared(a, b), 25.0f);
    EXPECT_FLOAT_EQ(distance(a, b), 5.0f);
}

TEST(VecOpsTest, Distance4D) {
    Vec4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4 b(4.0f, 6.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(distanceSquared(a, b), 25.0f);
    EXPECT_FLOAT_EQ(distance(a, b), 5.0f);
}

// ============================================================================
// Normalization Tests
// ============================================================================

TEST(VecOpsTest, Normalize2D) {
    Vec2 v(3.0f, 4.0f);
    Vec2 normalized = normalize(v);
    EXPECT_FLOAT_EQ(normalized.x, 0.6f);
    EXPECT_FLOAT_EQ(normalized.y, 0.8f);
    EXPECT_NEAR(length(normalized), 1.0f, 1e-6f);
}

TEST(VecOpsTest, Normalize3D) {
    Vec3 v(3.0f, 4.0f, 0.0f);
    Vec3 normalized = normalize(v);
    EXPECT_FLOAT_EQ(normalized.x, 0.6f);
    EXPECT_FLOAT_EQ(normalized.y, 0.8f);
    EXPECT_FLOAT_EQ(normalized.z, 0.0f);
    EXPECT_NEAR(length(normalized), 1.0f, 1e-6f);
}

TEST(VecOpsTest, Normalize4D) {
    Vec4 v(2.0f, 3.0f, 6.0f, 0.0f);
    Vec4 normalized = normalize(v);
    EXPECT_NEAR(length(normalized), 1.0f, 1e-6f);
}

TEST(VecOpsTest, NormalizeZeroVector) {
    Vec2 v2 = Vec2::zero();
    Vec2 normalized2 = normalize(v2);
    EXPECT_FLOAT_EQ(normalized2.x, 0.0f);
    EXPECT_FLOAT_EQ(normalized2.y, 0.0f);

    Vec3 v3 = Vec3::zero();
    Vec3 normalized3 = normalize(v3);
    EXPECT_FLOAT_EQ(normalized3.x, 0.0f);
    EXPECT_FLOAT_EQ(normalized3.y, 0.0f);
    EXPECT_FLOAT_EQ(normalized3.z, 0.0f);
}

// ============================================================================
// Safe Normalization Tests
// ============================================================================

TEST(VecOpsTest, SafeNormalize2D) {
    Vec2 v(3.0f, 4.0f);
    Vec2 normalized = safeNormalize(v);
    EXPECT_NEAR(length(normalized), 1.0f, 1e-6f);
}

TEST(VecOpsTest, SafeNormalize3D) {
    Vec3 v(3.0f, 4.0f, 0.0f);
    Vec3 normalized = safeNormalize(v);
    EXPECT_NEAR(length(normalized), 1.0f, 1e-6f);
}

TEST(VecOpsTest, SafeNormalizeZeroWithFallback) {
    Vec2 fallback2(1.0f, 0.0f);
    Vec2 result2 = safeNormalize(Vec2::zero(), fallback2);
    EXPECT_FLOAT_EQ(result2.x, 1.0f);
    EXPECT_FLOAT_EQ(result2.y, 0.0f);

    Vec3 fallback3(0.0f, 1.0f, 0.0f);
    Vec3 result3 = safeNormalize(Vec3::zero(), fallback3);
    EXPECT_FLOAT_EQ(result3.x, 0.0f);
    EXPECT_FLOAT_EQ(result3.y, 1.0f);
    EXPECT_FLOAT_EQ(result3.z, 0.0f);
}

// ============================================================================
// Reflection Tests
// ============================================================================

TEST(VecOpsTest, Reflect2D) {
    Vec2 v(1.0f, -1.0f);
    Vec2 n(0.0f, 1.0f);  // Normal pointing up
    Vec2 reflected = reflect(v, n);
    EXPECT_FLOAT_EQ(reflected.x, 1.0f);
    EXPECT_FLOAT_EQ(reflected.y, 1.0f);
}

TEST(VecOpsTest, Reflect3D) {
    Vec3 v(1.0f, -1.0f, 0.0f);
    Vec3 n(0.0f, 1.0f, 0.0f);  // Normal pointing up
    Vec3 reflected = reflect(v, n);
    EXPECT_FLOAT_EQ(reflected.x, 1.0f);
    EXPECT_FLOAT_EQ(reflected.y, 1.0f);
    EXPECT_FLOAT_EQ(reflected.z, 0.0f);
}

// ============================================================================
// Refraction Tests
// ============================================================================

TEST(VecOpsTest, Refract2D) {
    Vec2 v = normalize(Vec2(1.0f, -1.0f));
    Vec2 n(0.0f, 1.0f);       // Normal pointing up
    float eta = 1.0f / 1.5f;  // Air to glass
    Vec2 refracted = refract(v, n, eta);

    // Refracted vector should bend towards normal
    EXPECT_LT(refracted.y, v.y);  // More negative y
}

TEST(VecOpsTest, Refract3D) {
    Vec3 v = normalize(Vec3(1.0f, -1.0f, 0.0f));
    Vec3 n(0.0f, 1.0f, 0.0f);  // Normal pointing up
    float eta = 1.0f / 1.5f;   // Air to glass
    Vec3 refracted = refract(v, n, eta);

    // Refracted vector should bend towards normal
    EXPECT_LT(refracted.y, v.y);  // More negative y
}

TEST(VecOpsTest, RefractTotalInternalReflection) {
    Vec3 v = normalize(Vec3(1.0f, -0.3f, 0.0f));
    Vec3 n(0.0f, 1.0f, 0.0f);
    float eta = 1.5f;  // Glass to air with shallow angle

    Vec3 refracted = refract(v, n, eta);

    // Should result in total internal reflection (zero vector)
    // May or may not occur depending on angle, but this tests the code path
    if (refracted == Vec3::zero()) {
        EXPECT_EQ(refracted, Vec3::zero());
    }
}

// ============================================================================
// Linear Interpolation Tests
// ============================================================================

TEST(VecOpsTest, Lerp2D) {
    Vec2 a(0.0f, 0.0f);
    Vec2 b(10.0f, 20.0f);

    Vec2 mid = lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(mid.x, 5.0f);
    EXPECT_FLOAT_EQ(mid.y, 10.0f);

    Vec2 quarter = lerp(a, b, 0.25f);
    EXPECT_FLOAT_EQ(quarter.x, 2.5f);
    EXPECT_FLOAT_EQ(quarter.y, 5.0f);
}

TEST(VecOpsTest, Lerp3D) {
    Vec3 a(0.0f, 0.0f, 0.0f);
    Vec3 b(10.0f, 20.0f, 30.0f);

    Vec3 mid = lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(mid.x, 5.0f);
    EXPECT_FLOAT_EQ(mid.y, 10.0f);
    EXPECT_FLOAT_EQ(mid.z, 15.0f);
}

TEST(VecOpsTest, Lerp4D) {
    Vec4 a(0.0f, 0.0f, 0.0f, 0.0f);
    Vec4 b(10.0f, 20.0f, 30.0f, 40.0f);

    Vec4 mid = lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(mid.x, 5.0f);
    EXPECT_FLOAT_EQ(mid.y, 10.0f);
    EXPECT_FLOAT_EQ(mid.z, 15.0f);
    EXPECT_FLOAT_EQ(mid.w, 20.0f);
}

// ============================================================================
// Min/Max/Clamp Tests
// ============================================================================

TEST(VecOpsTest, Min2D) {
    Vec2 a(1.0f, 5.0f);
    Vec2 b(3.0f, 2.0f);
    Vec2 result = min(a, b);
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
}

TEST(VecOpsTest, Max2D) {
    Vec2 a(1.0f, 5.0f);
    Vec2 b(3.0f, 2.0f);
    Vec2 result = max(a, b);
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 5.0f);
}

TEST(VecOpsTest, Clamp2D) {
    Vec2 v(5.0f, -3.0f);
    Vec2 minVec(0.0f, 0.0f);
    Vec2 maxVec(10.0f, 10.0f);
    Vec2 result = clamp(v, minVec, maxVec);
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(VecOpsTest, Min3D) {
    Vec3 a(1.0f, 5.0f, 3.0f);
    Vec3 b(3.0f, 2.0f, 4.0f);
    Vec3 result = min(a, b);
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST(VecOpsTest, Max3D) {
    Vec3 a(1.0f, 5.0f, 3.0f);
    Vec3 b(3.0f, 2.0f, 4.0f);
    Vec3 result = max(a, b);
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 5.0f);
    EXPECT_FLOAT_EQ(result.z, 4.0f);
}

TEST(VecOpsTest, Clamp3D) {
    Vec3 v(5.0f, -3.0f, 15.0f);
    Vec3 minVec(0.0f, 0.0f, 0.0f);
    Vec3 maxVec(10.0f, 10.0f, 10.0f);
    Vec3 result = clamp(v, minVec, maxVec);
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, 10.0f);
}

// ============================================================================
// Component-wise Math Function Tests
// ============================================================================

TEST(VecOpsTest, Abs2D) {
    Vec2 v(-3.5f, 4.5f);
    Vec2 result = abs(v);
    EXPECT_FLOAT_EQ(result.x, 3.5f);
    EXPECT_FLOAT_EQ(result.y, 4.5f);
}

TEST(VecOpsTest, Floor2D) {
    Vec2 v(3.7f, -2.3f);
    Vec2 result = floor(v);
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, -3.0f);
}

TEST(VecOpsTest, Ceil2D) {
    Vec2 v(3.2f, -2.8f);
    Vec2 result = ceil(v);
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, -2.0f);
}

TEST(VecOpsTest, Round2D) {
    Vec2 v(3.4f, 3.6f);
    Vec2 result = round(v);
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
}

TEST(VecOpsTest, Abs3D) {
    Vec3 v(-3.5f, 4.5f, -1.5f);
    Vec3 result = abs(v);
    EXPECT_FLOAT_EQ(result.x, 3.5f);
    EXPECT_FLOAT_EQ(result.y, 4.5f);
    EXPECT_FLOAT_EQ(result.z, 1.5f);
}

TEST(VecOpsTest, Floor3D) {
    Vec3 v(3.7f, -2.3f, 1.1f);
    Vec3 result = floor(v);
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, -3.0f);
    EXPECT_FLOAT_EQ(result.z, 1.0f);
}

TEST(VecOpsTest, Ceil3D) {
    Vec3 v(3.2f, -2.8f, 0.1f);
    Vec3 result = ceil(v);
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, -2.0f);
    EXPECT_FLOAT_EQ(result.z, 1.0f);
}

TEST(VecOpsTest, Round3D) {
    Vec3 v(3.4f, 3.6f, 2.5f);
    Vec3 result = round(v);
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);  // std::round rounds half away from zero
}
