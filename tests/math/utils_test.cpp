#include "axiom/math/constants.hpp"
#include "axiom/math/utils.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

using namespace axiom::math;

// ============================================================================
// Angle Conversion Tests
// ============================================================================

TEST(MathUtilsTest, RadiansConversion) {
    // Test common angles
    EXPECT_FLOAT_EQ(radians(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(radians(90.0f), HALF_PI_F);
    EXPECT_FLOAT_EQ(radians(180.0f), PI_F);
    EXPECT_FLOAT_EQ(radians(360.0f), TWO_PI_F);

    // Test negative angles
    EXPECT_FLOAT_EQ(radians(-90.0f), -HALF_PI_F);
    EXPECT_FLOAT_EQ(radians(-180.0f), -PI_F);

    // Test constexpr
    static_assert(radians(180.0f) > 3.14f && radians(180.0f) < 3.15f, "radians is constexpr");
}

TEST(MathUtilsTest, DegreesConversion) {
    // Test common angles
    EXPECT_FLOAT_EQ(degrees(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(degrees(HALF_PI_F), 90.0f);
    EXPECT_FLOAT_EQ(degrees(PI_F), 180.0f);
    EXPECT_FLOAT_EQ(degrees(TWO_PI_F), 360.0f);

    // Test negative angles
    EXPECT_FLOAT_EQ(degrees(-HALF_PI_F), -90.0f);
    EXPECT_FLOAT_EQ(degrees(-PI_F), -180.0f);

    // Test constexpr
    static_assert(degrees(3.14159f) > 179.0f && degrees(3.14159f) < 181.0f, "degrees is constexpr");
}

TEST(MathUtilsTest, AngleConversionRoundTrip) {
    // Test round-trip conversion
    float angle = 45.0f;
    EXPECT_FLOAT_EQ(degrees(radians(angle)), angle);

    float rad = 1.5f;
    EXPECT_FLOAT_EQ(radians(degrees(rad)), rad);
}

// ============================================================================
// Clamping Tests
// ============================================================================

TEST(MathUtilsTest, ClampBasic) {
    // Value within range
    EXPECT_FLOAT_EQ(clamp(5.0f, 0.0f, 10.0f), 5.0f);

    // Value below range
    EXPECT_FLOAT_EQ(clamp(-5.0f, 0.0f, 10.0f), 0.0f);

    // Value above range
    EXPECT_FLOAT_EQ(clamp(15.0f, 0.0f, 10.0f), 10.0f);

    // Value at boundaries
    EXPECT_FLOAT_EQ(clamp(0.0f, 0.0f, 10.0f), 0.0f);
    EXPECT_FLOAT_EQ(clamp(10.0f, 0.0f, 10.0f), 10.0f);
}

TEST(MathUtilsTest, ClampNegativeRange) {
    EXPECT_FLOAT_EQ(clamp(-5.0f, -10.0f, -1.0f), -5.0f);
    EXPECT_FLOAT_EQ(clamp(-15.0f, -10.0f, -1.0f), -10.0f);
    EXPECT_FLOAT_EQ(clamp(0.0f, -10.0f, -1.0f), -1.0f);
}

TEST(MathUtilsTest, ClampIsConstexpr) {
    static_assert(clamp(5.0f, 0.0f, 10.0f) == 5.0f, "clamp is constexpr");
}

// ============================================================================
// Linear Interpolation Tests
// ============================================================================

TEST(MathUtilsTest, LerpBasic) {
    // Test at boundaries
    EXPECT_FLOAT_EQ(lerp(0.0f, 10.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(lerp(0.0f, 10.0f, 1.0f), 10.0f);

    // Test midpoint
    EXPECT_FLOAT_EQ(lerp(0.0f, 10.0f, 0.5f), 5.0f);

    // Test quarter points
    EXPECT_FLOAT_EQ(lerp(0.0f, 10.0f, 0.25f), 2.5f);
    EXPECT_FLOAT_EQ(lerp(0.0f, 10.0f, 0.75f), 7.5f);
}

TEST(MathUtilsTest, LerpNegativeRange) {
    EXPECT_FLOAT_EQ(lerp(-10.0f, 10.0f, 0.5f), 0.0f);
    EXPECT_FLOAT_EQ(lerp(-10.0f, -5.0f, 0.5f), -7.5f);
}

TEST(MathUtilsTest, LerpExtrapolation) {
    // lerp can extrapolate outside [0, 1]
    EXPECT_FLOAT_EQ(lerp(0.0f, 10.0f, 2.0f), 20.0f);
    EXPECT_FLOAT_EQ(lerp(0.0f, 10.0f, -1.0f), -10.0f);
}

TEST(MathUtilsTest, LerpIsConstexpr) {
    static_assert(lerp(0.0f, 10.0f, 0.5f) == 5.0f, "lerp is constexpr");
}

// ============================================================================
// Smoothstep Tests
// ============================================================================

TEST(MathUtilsTest, SmoothstepBasic) {
    // Test at boundaries
    EXPECT_FLOAT_EQ(smoothstep(0.0f, 1.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(smoothstep(0.0f, 1.0f, 1.0f), 1.0f);

    // Test outside range (should clamp)
    EXPECT_FLOAT_EQ(smoothstep(0.0f, 1.0f, -1.0f), 0.0f);
    EXPECT_FLOAT_EQ(smoothstep(0.0f, 1.0f, 2.0f), 1.0f);

    // Test midpoint (should be 0.5)
    EXPECT_FLOAT_EQ(smoothstep(0.0f, 1.0f, 0.5f), 0.5f);
}

TEST(MathUtilsTest, SmoothstepSmootherThanLinear) {
    // Smoothstep should have zero derivative at edges
    // At t=0.25, smoothstep should be less than linear lerp
    float smoothValue = smoothstep(0.0f, 1.0f, 0.25f);
    float linearValue = 0.25f;
    EXPECT_LT(smoothValue, linearValue);

    // At t=0.75, smoothstep should be greater than linear lerp
    smoothValue = smoothstep(0.0f, 1.0f, 0.75f);
    linearValue = 0.75f;
    EXPECT_GT(smoothValue, linearValue);
}

TEST(MathUtilsTest, SmoothstepCustomRange) {
    EXPECT_FLOAT_EQ(smoothstep(10.0f, 20.0f, 10.0f), 0.0f);
    EXPECT_FLOAT_EQ(smoothstep(10.0f, 20.0f, 20.0f), 1.0f);
    EXPECT_FLOAT_EQ(smoothstep(10.0f, 20.0f, 15.0f), 0.5f);
}

// ============================================================================
// Sign Tests
// ============================================================================

TEST(MathUtilsTest, SignBasic) {
    EXPECT_FLOAT_EQ(sign(10.0f), 1.0f);
    EXPECT_FLOAT_EQ(sign(-10.0f), -1.0f);
    EXPECT_FLOAT_EQ(sign(0.0f), 0.0f);

    EXPECT_FLOAT_EQ(sign(0.001f), 1.0f);
    EXPECT_FLOAT_EQ(sign(-0.001f), -1.0f);
}

TEST(MathUtilsTest, SignIsConstexpr) {
    static_assert(sign(5.0f) == 1.0f, "sign is constexpr");
    static_assert(sign(-5.0f) == -1.0f, "sign is constexpr");
    static_assert(sign(0.0f) == 0.0f, "sign is constexpr");
}

// ============================================================================
// Almost Equal Tests
// ============================================================================

TEST(MathUtilsTest, AlmostEqualBasic) {
    // Exactly equal
    EXPECT_TRUE(almostEqual(1.0f, 1.0f));

    // Very close (within default epsilon)
    EXPECT_TRUE(almostEqual(1.0f, 1.0f + EPSILON_F * 0.5f));

    // Not close enough
    EXPECT_FALSE(almostEqual(1.0f, 1.1f));
}

TEST(MathUtilsTest, AlmostEqualCustomEpsilon) {
    float a = 1.0f;
    float b = 1.01f;

    EXPECT_FALSE(almostEqual(a, b, 0.001f));
    EXPECT_TRUE(almostEqual(a, b, 0.1f));
}

TEST(MathUtilsTest, AlmostEqualNegativeValues) {
    EXPECT_TRUE(almostEqual(-1.0f, -1.0f));
    EXPECT_TRUE(almostEqual(-1.0f, -1.0f + EPSILON_F * 0.5f));
}

TEST(MathUtilsTest, AlmostEqualZero) {
    EXPECT_TRUE(almostEqual(0.0f, 0.0f));
    EXPECT_TRUE(almostEqual(0.0f, EPSILON_F * 0.5f));
    EXPECT_FALSE(almostEqual(0.0f, 0.1f));
}

// ============================================================================
// Power of Two Tests
// ============================================================================

TEST(MathUtilsTest, IsPowerOfTwoBasic) {
    // Valid powers of two
    EXPECT_TRUE(isPowerOfTwo(1));
    EXPECT_TRUE(isPowerOfTwo(2));
    EXPECT_TRUE(isPowerOfTwo(4));
    EXPECT_TRUE(isPowerOfTwo(8));
    EXPECT_TRUE(isPowerOfTwo(16));
    EXPECT_TRUE(isPowerOfTwo(1024));
    EXPECT_TRUE(isPowerOfTwo(1U << 30));

    // Not powers of two
    EXPECT_FALSE(isPowerOfTwo(0));
    EXPECT_FALSE(isPowerOfTwo(3));
    EXPECT_FALSE(isPowerOfTwo(5));
    EXPECT_FALSE(isPowerOfTwo(6));
    EXPECT_FALSE(isPowerOfTwo(7));
    EXPECT_FALSE(isPowerOfTwo(100));
}

TEST(MathUtilsTest, IsPowerOfTwoIsConstexpr) {
    static_assert(isPowerOfTwo(16), "isPowerOfTwo is constexpr");
    static_assert(!isPowerOfTwo(15), "isPowerOfTwo is constexpr");
}

TEST(MathUtilsTest, NextPowerOfTwoBasic) {
    // Already power of two
    EXPECT_EQ(nextPowerOfTwo(1), 1U);
    EXPECT_EQ(nextPowerOfTwo(2), 2U);
    EXPECT_EQ(nextPowerOfTwo(4), 4U);
    EXPECT_EQ(nextPowerOfTwo(8), 8U);

    // Round up to next power of two
    EXPECT_EQ(nextPowerOfTwo(3), 4U);
    EXPECT_EQ(nextPowerOfTwo(5), 8U);
    EXPECT_EQ(nextPowerOfTwo(6), 8U);
    EXPECT_EQ(nextPowerOfTwo(7), 8U);
    EXPECT_EQ(nextPowerOfTwo(9), 16U);
    EXPECT_EQ(nextPowerOfTwo(100), 128U);
}

TEST(MathUtilsTest, NextPowerOfTwoZero) {
    EXPECT_EQ(nextPowerOfTwo(0), 0U);
}

TEST(MathUtilsTest, NextPowerOfTwoLargeValues) {
    EXPECT_EQ(nextPowerOfTwo(1U << 20), 1U << 20);
    EXPECT_EQ(nextPowerOfTwo((1U << 20) + 1), 1U << 21);
}
