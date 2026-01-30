#pragma once

#include "axiom/math/vec3.hpp"

#include <cmath>
#include <cstdint>

namespace axiom::math {

/**
 * @brief Deterministic random number generator using PCG algorithm
 *
 * PCG (Permuted Congruential Generator) provides good statistical properties
 * and fast generation, suitable for physics simulations where determinism
 * and reproducibility are critical.
 *
 * @see https://www.pcg-random.org/
 */
class DeterministicRNG {
public:
    /**
     * @brief Construct a deterministic RNG with a given seed
     * @param seed Initial seed value (default: 0)
     */
    explicit DeterministicRNG(uint64_t seed = 0) noexcept : state_(seed | 1ULL) {
        // Ensure state is odd (required for PCG)
        // Warm up the generator
        for (int i = 0; i < 10; ++i) {
            next();
        }
    }

    /**
     * @brief Generate next random 32-bit unsigned integer
     * @return Random uint32_t value
     */
    uint32_t next() noexcept {
        // PCG-XSH-RR algorithm
        uint64_t oldState = state_;
        // LCG step: state = state * 6364136223846793005 + increment
        state_ = oldState * 6364136223846793005ULL + 1442695040888963407ULL;

        // XSH-RR output function
        uint32_t xorshifted = static_cast<uint32_t>(((oldState >> 18U) ^ oldState) >> 27U);
        uint32_t rot = static_cast<uint32_t>(oldState >> 59U);
        return (xorshifted >> rot) | (xorshifted << ((~rot + 1U) & 31));
    }

    /**
     * @brief Generate random float in range [0, 1)
     * @return Random float value
     */
    float nextFloat() noexcept {
        // Convert to float in [0, 1) by dividing by 2^32
        return static_cast<float>(next()) / static_cast<float>(0x100000000ULL);
    }

    /**
     * @brief Generate random float in range [min, max)
     * @param min Minimum value (inclusive)
     * @param max Maximum value (exclusive)
     * @return Random float value in [min, max)
     */
    float nextFloat(float min, float max) noexcept { return min + nextFloat() * (max - min); }

private:
    uint64_t state_;  ///< Internal RNG state
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Generate random float in range [min, max)
 * @param min Minimum value (inclusive, default: 0.0)
 * @param max Maximum value (exclusive, default: 1.0)
 * @return Random float value
 * @note Uses thread-local RNG instance for convenience
 */
inline float randomFloat(float min = 0.0f, float max = 1.0f) {
    thread_local DeterministicRNG rng(12345);  // Fixed seed for determinism
    return rng.nextFloat(min, max);
}

/**
 * @brief Generate random Vec3 with each component in range [min, max)
 * @param min Minimum value for each component (default: 0.0)
 * @param max Maximum value for each component (default: 1.0)
 * @return Random Vec3
 */
inline Vec3 randomVec3(float min = 0.0f, float max = 1.0f) {
    thread_local DeterministicRNG rng(23456);  // Fixed seed for determinism
    return Vec3(rng.nextFloat(min, max), rng.nextFloat(min, max), rng.nextFloat(min, max));
}

/**
 * @brief Generate random unit direction vector
 * @return Random normalized Vec3 (uniformly distributed on unit sphere surface)
 * @note Uses Marsaglia's method for uniform distribution
 */
inline Vec3 randomDirection() {
    thread_local DeterministicRNG rng(34567);  // Fixed seed for determinism

    // Marsaglia's method for uniform sphere point picking
    float x, y, z, lengthSq;
    do {
        x = rng.nextFloat(-1.0f, 1.0f);
        y = rng.nextFloat(-1.0f, 1.0f);
        z = rng.nextFloat(-1.0f, 1.0f);
        lengthSq = x * x + y * y + z * z;
    } while (lengthSq > 1.0f || lengthSq < 1e-6f);

    const float invLength = 1.0f / std::sqrt(lengthSq);
    return Vec3(x * invLength, y * invLength, z * invLength);
}

/**
 * @brief Generate random point inside unit sphere
 * @return Random Vec3 with length <= 1.0
 * @note Uses rejection sampling for uniform distribution
 */
inline Vec3 randomInSphere() {
    thread_local DeterministicRNG rng(45678);  // Fixed seed for determinism

    // Rejection sampling
    float x, y, z, lengthSq;
    do {
        x = rng.nextFloat(-1.0f, 1.0f);
        y = rng.nextFloat(-1.0f, 1.0f);
        z = rng.nextFloat(-1.0f, 1.0f);
        lengthSq = x * x + y * y + z * z;
    } while (lengthSq > 1.0f);

    return Vec3(x, y, z);
}

/**
 * @brief Generate random point on unit sphere surface
 * @return Random normalized Vec3 (uniformly distributed on unit sphere)
 */
inline Vec3 randomOnSphere() {
    // randomDirection() already provides uniform distribution on sphere
    return randomDirection();
}

}  // namespace axiom::math
