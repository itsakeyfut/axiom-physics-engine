#include "axiom/math/random.hpp"
#include "axiom/math/vec3.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <unordered_set>

using namespace axiom::math;

// ============================================================================
// DeterministicRNG Tests
// ============================================================================

TEST(RandomTest, DeterministicRNGSameSeedProducesSameSequence) {
    DeterministicRNG rng1(12345);
    DeterministicRNG rng2(12345);

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng1.next(), rng2.next());
    }
}

TEST(RandomTest, DeterministicRNGDifferentSeedsProduceDifferentSequences) {
    DeterministicRNG rng1(12345);
    DeterministicRNG rng2(54321);

    bool foundDifference = false;
    for (int i = 0; i < 100; ++i) {
        if (rng1.next() != rng2.next()) {
            foundDifference = true;
            break;
        }
    }

    EXPECT_TRUE(foundDifference);
}

TEST(RandomTest, DeterministicRNGNextProducesVariedValues) {
    DeterministicRNG rng(42);

    std::unordered_set<uint32_t> values;
    for (int i = 0; i < 1000; ++i) {
        values.insert(rng.next());
    }

    // Should have generated many unique values (at least 90% unique)
    EXPECT_GT(values.size(), 900U);
}

TEST(RandomTest, DeterministicRNGNextFloatRange) {
    DeterministicRNG rng(42);

    for (int i = 0; i < 1000; ++i) {
        float value = rng.nextFloat();
        EXPECT_GE(value, 0.0f);
        EXPECT_LT(value, 1.0f);
    }
}

TEST(RandomTest, DeterministicRNGNextFloatCustomRange) {
    DeterministicRNG rng(42);

    float min = 10.0f;
    float max = 20.0f;

    for (int i = 0; i < 1000; ++i) {
        float value = rng.nextFloat(min, max);
        EXPECT_GE(value, min);
        EXPECT_LT(value, max);
    }
}

TEST(RandomTest, DeterministicRNGNextFloatNegativeRange) {
    DeterministicRNG rng(42);

    float min = -10.0f;
    float max = -5.0f;

    for (int i = 0; i < 1000; ++i) {
        float value = rng.nextFloat(min, max);
        EXPECT_GE(value, min);
        EXPECT_LT(value, max);
    }
}

TEST(RandomTest, DeterministicRNGNextFloatDistribution) {
    DeterministicRNG rng(42);

    // Generate many samples and check distribution
    int countLow = 0;
    int countHigh = 0;
    int samples = 10000;

    for (int i = 0; i < samples; ++i) {
        float value = rng.nextFloat(0.0f, 10.0f);
        if (value < 5.0f) {
            countLow++;
        } else {
            countHigh++;
        }
    }

    // Should be roughly 50/50 split (allow 10% tolerance)
    float ratio = static_cast<float>(countLow) / static_cast<float>(samples);
    EXPECT_GT(ratio, 0.45f);
    EXPECT_LT(ratio, 0.55f);
}

// ============================================================================
// Helper Function Tests
// ============================================================================

TEST(RandomTest, RandomFloatRange) {
    for (int i = 0; i < 100; ++i) {
        float value = randomFloat(0.0f, 1.0f);
        EXPECT_GE(value, 0.0f);
        EXPECT_LT(value, 1.0f);
    }
}

TEST(RandomTest, RandomFloatCustomRange) {
    for (int i = 0; i < 100; ++i) {
        float value = randomFloat(-5.0f, 5.0f);
        EXPECT_GE(value, -5.0f);
        EXPECT_LT(value, 5.0f);
    }
}

TEST(RandomTest, RandomVec3Range) {
    for (int i = 0; i < 100; ++i) {
        Vec3 v = randomVec3(0.0f, 1.0f);

        EXPECT_GE(v.x, 0.0f);
        EXPECT_LT(v.x, 1.0f);

        EXPECT_GE(v.y, 0.0f);
        EXPECT_LT(v.y, 1.0f);

        EXPECT_GE(v.z, 0.0f);
        EXPECT_LT(v.z, 1.0f);
    }
}

TEST(RandomTest, RandomVec3CustomRange) {
    for (int i = 0; i < 100; ++i) {
        Vec3 v = randomVec3(-10.0f, 10.0f);

        EXPECT_GE(v.x, -10.0f);
        EXPECT_LT(v.x, 10.0f);

        EXPECT_GE(v.y, -10.0f);
        EXPECT_LT(v.y, 10.0f);

        EXPECT_GE(v.z, -10.0f);
        EXPECT_LT(v.z, 10.0f);
    }
}

// ============================================================================
// Random Direction Tests
// ============================================================================

TEST(RandomTest, RandomDirectionIsNormalized) {
    for (int i = 0; i < 100; ++i) {
        Vec3 dir = randomDirection();
        float length = dir.length();

        // Should be very close to 1.0
        EXPECT_NEAR(length, 1.0f, 1e-5f);
    }
}

TEST(RandomTest, RandomDirectionCoversAllDirections) {
    // Generate many random directions and check they cover different octants
    int octantCount[8] = {0};

    for (int i = 0; i < 1000; ++i) {
        Vec3 dir = randomDirection();

        int octant = 0;
        if (dir.x > 0.0f)
            octant |= 1;
        if (dir.y > 0.0f)
            octant |= 2;
        if (dir.z > 0.0f)
            octant |= 4;

        octantCount[octant]++;
    }

    // Each octant should have some samples (at least 5% of total)
    for (int i = 0; i < 8; ++i) {
        EXPECT_GT(octantCount[i], 50);
    }
}

TEST(RandomTest, RandomDirectionUniformDistribution) {
    // Check that directions are roughly uniformly distributed
    // by measuring average distance from origin (should be 0)
    Vec3 sum(0.0f, 0.0f, 0.0f);
    int samples = 10000;

    for (int i = 0; i < samples; ++i) {
        sum = sum + randomDirection();
    }

    Vec3 average = sum / static_cast<float>(samples);

    // Average should be close to zero for uniform distribution
    EXPECT_NEAR(average.length(), 0.0f, 0.05f);
}

// ============================================================================
// Random In Sphere Tests
// ============================================================================

TEST(RandomTest, RandomInSphereIsInsideUnitSphere) {
    for (int i = 0; i < 1000; ++i) {
        Vec3 p = randomInSphere();
        float lengthSq = p.lengthSquared();

        // Should be inside or on unit sphere
        EXPECT_LE(lengthSq, 1.0f);
    }
}

TEST(RandomTest, RandomInSphereCoversVolume) {
    // Check that points are distributed throughout the volume
    // by measuring average radius (should be around 0.75 for uniform volume distribution)
    float sumRadius = 0.0f;
    int samples = 10000;

    for (int i = 0; i < samples; ++i) {
        Vec3 p = randomInSphere();
        sumRadius += p.length();
    }

    float avgRadius = sumRadius / static_cast<float>(samples);

    // For uniform distribution in sphere, average radius is 3/4
    // Allow some tolerance
    EXPECT_GT(avgRadius, 0.70f);
    EXPECT_LT(avgRadius, 0.80f);
}

TEST(RandomTest, RandomInSphereCentered) {
    // Check that points are centered around origin
    Vec3 sum(0.0f, 0.0f, 0.0f);
    int samples = 10000;

    for (int i = 0; i < samples; ++i) {
        sum = sum + randomInSphere();
    }

    Vec3 average = sum / static_cast<float>(samples);

    // Average should be close to zero
    EXPECT_NEAR(average.length(), 0.0f, 0.05f);
}

// ============================================================================
// Random On Sphere Tests
// ============================================================================

TEST(RandomTest, RandomOnSphereIsOnUnitSphere) {
    for (int i = 0; i < 100; ++i) {
        Vec3 p = randomOnSphere();
        float length = p.length();

        // Should be very close to 1.0
        EXPECT_NEAR(length, 1.0f, 1e-5f);
    }
}

TEST(RandomTest, RandomOnSphereCoversAllDirections) {
    // Generate many random points and check they cover different octants
    int octantCount[8] = {0};

    for (int i = 0; i < 1000; ++i) {
        Vec3 p = randomOnSphere();

        int octant = 0;
        if (p.x > 0.0f)
            octant |= 1;
        if (p.y > 0.0f)
            octant |= 2;
        if (p.z > 0.0f)
            octant |= 4;

        octantCount[octant]++;
    }

    // Each octant should have some samples (at least 5% of total)
    for (int i = 0; i < 8; ++i) {
        EXPECT_GT(octantCount[i], 50);
    }
}

TEST(RandomTest, RandomOnSphereCentered) {
    // Check that points on sphere are centered around origin
    Vec3 sum(0.0f, 0.0f, 0.0f);
    int samples = 10000;

    for (int i = 0; i < samples; ++i) {
        sum = sum + randomOnSphere();
    }

    Vec3 average = sum / static_cast<float>(samples);

    // Average should be close to zero for uniform distribution
    EXPECT_NEAR(average.length(), 0.0f, 0.05f);
}
