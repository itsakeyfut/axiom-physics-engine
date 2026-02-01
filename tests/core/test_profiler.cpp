/**
 * @file test_profiler.cpp
 * @brief Unit tests for profiling infrastructure
 */

#include <axiom/core/profiler.hpp>
#include <gtest/gtest.h>

#include <chrono>
#include <thread>

/**
 * @brief Test fixture for profiler tests
 */
class ProfilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

/**
 * @brief Test that profiling macros compile without errors
 *
 * This test verifies that all profiling macros are properly defined
 * and can be used in code without compilation errors.
 */
TEST_F(ProfilerTest, MacrosCompile) {
    // Frame marker
    AXIOM_PROFILE_FRAME();

    // Scope profiling
    {
        AXIOM_PROFILE_SCOPE("TestScope");
        // Do some work
        volatile int x = 0;
        for (int i = 0; i < 100; ++i) {
            x += i;
        }
    }

    // Function profiling (tested in helper function below)
    // Tag annotation
    AXIOM_PROFILE_TAG("TestTag", "test value");

    // Value plotting
    AXIOM_PROFILE_VALUE("TestValue", 42);

    // Memory tracking
    void* ptr = nullptr;
    AXIOM_PROFILE_ALLOC(ptr, 1024);
    AXIOM_PROFILE_FREE(ptr);
    (void)ptr;  // Suppress unused variable warning

    // GPU profiling (no actual GPU context, just testing compilation)
    void* fakeCtx = nullptr;
    AXIOM_PROFILE_GPU_ZONE(fakeCtx, "TestGPUZone");
    AXIOM_PROFILE_GPU_COLLECT(fakeCtx);
    (void)fakeCtx;  // Suppress unused variable warning

    SUCCEED();
}

/**
 * @brief Helper function to test AXIOM_PROFILE_FUNCTION macro
 */
static void helperFunctionWithProfiling() {
    AXIOM_PROFILE_FUNCTION();
    // Do some work
    volatile int x = 0;
    for (int i = 0; i < 100; ++i) {
        x += i;
    }
}

/**
 * @brief Test AXIOM_PROFILE_FUNCTION macro
 */
TEST_F(ProfilerTest, FunctionProfiling) {
    helperFunctionWithProfiling();
    SUCCEED();
}

/**
 * @brief Test nested profiling scopes
 *
 * Verifies that profiling scopes can be nested (hierarchical profiling).
 */
TEST_F(ProfilerTest, NestedScopes) {
    AXIOM_PROFILE_SCOPE("OuterScope");

    {
        AXIOM_PROFILE_SCOPE("InnerScope1");
        volatile int x = 0;
        for (int i = 0; i < 50; ++i) {
            x += i;
        }
    }

    {
        AXIOM_PROFILE_SCOPE("InnerScope2");
        volatile int y = 0;
        for (int i = 0; i < 50; ++i) {
            y += i;
        }
    }

    SUCCEED();
}

/**
 * @brief Test that profiling macros have zero cost when disabled
 *
 * This test verifies that when AXIOM_ENABLE_PROFILING is not defined,
 * the profiling macros expand to no-ops and have no runtime overhead.
 * We test this by ensuring the macros can be used without side effects.
 */
TEST_F(ProfilerTest, ZeroCostWhenDisabled) {
#ifndef AXIOM_ENABLE_PROFILING
    // When profiling is disabled, macros should expand to ((void)0)
    // and have no effect on program behavior

    int counter = 0;

    AXIOM_PROFILE_SCOPE("TestScope");
    counter++;

    AXIOM_PROFILE_FUNCTION();
    counter++;

    AXIOM_PROFILE_FRAME();
    counter++;

    AXIOM_PROFILE_TAG("tag", "value");
    counter++;

    AXIOM_PROFILE_VALUE("value", 42);
    counter++;

    EXPECT_EQ(counter, 5);
#else
    // When profiling is enabled, Tracy is active
    // We can't test for zero cost, but we can verify it compiles
    SUCCEED();
#endif
}

/**
 * @brief Test memory profiling
 *
 * Verifies that memory allocation tracking macros work correctly.
 */
TEST_F(ProfilerTest, MemoryProfiling) {
    const size_t allocSize = 1024;
    void* ptr = malloc(allocSize);

    // Track allocation
    AXIOM_PROFILE_ALLOC(ptr, allocSize);

    // Do some work with the memory
    if (ptr != nullptr) {
        memset(ptr, 0, allocSize);
    }

    // Track deallocation
    AXIOM_PROFILE_FREE(ptr);

    free(ptr);

    SUCCEED();
}

/**
 * @brief Test value plotting with different data types
 */
TEST_F(ProfilerTest, ValuePlottingTypes) {
    // Integer values
    AXIOM_PROFILE_VALUE("IntValue", 42);

    // Floating point values
    AXIOM_PROFILE_VALUE("FloatValue", 3.14f);

    // Double values
    AXIOM_PROFILE_VALUE("DoubleValue", 2.71828);

    // Negative values
    AXIOM_PROFILE_VALUE("NegativeValue", -100);

    SUCCEED();
}

/**
 * @brief Test profiling in a simulated physics loop
 *
 * This test simulates a typical physics engine update loop
 * to demonstrate realistic profiling usage.
 */
TEST_F(ProfilerTest, SimulatedPhysicsLoop) {
    constexpr int numFrames = 3;

    for (int frame = 0; frame < numFrames; ++frame) {
        AXIOM_PROFILE_SCOPE("PhysicsFrame");

        // Broadphase
        {
            AXIOM_PROFILE_SCOPE("Broadphase");
            volatile int pairs = 0;
            for (int i = 0; i < 10; ++i) {
                pairs += i;
            }
            AXIOM_PROFILE_VALUE("BroadphasePairs", pairs);
        }

        // Narrowphase
        {
            AXIOM_PROFILE_SCOPE("Narrowphase");
            volatile int contacts = 0;
            for (int i = 0; i < 5; ++i) {
                contacts += i;
            }
            AXIOM_PROFILE_VALUE("ContactCount", contacts);
        }

        // Solver
        {
            AXIOM_PROFILE_SCOPE("Solver");
            volatile int iterations = 0;
            for (int i = 0; i < 8; ++i) {
                iterations += i;
            }
            AXIOM_PROFILE_VALUE("SolverIterations", iterations);
        }

        // Integration
        {
            AXIOM_PROFILE_SCOPE("Integration");
            volatile float dt = 0.016f;
            for (int i = 0; i < 20; ++i) {
                dt += 0.001f;
            }
        }

        AXIOM_PROFILE_FRAME();
    }

    SUCCEED();
}

/**
 * @brief Test that multiple profile values can be tracked simultaneously
 */
TEST_F(ProfilerTest, MultipleValues) {
    AXIOM_PROFILE_SCOPE("MultipleValues");

    for (int i = 0; i < 10; ++i) {
        AXIOM_PROFILE_VALUE("Counter", i);
        AXIOM_PROFILE_VALUE("Squared", i * i);
        AXIOM_PROFILE_VALUE("Doubled", i * 2);
    }

    SUCCEED();
}

/**
 * @brief Test profiling with string tags
 */
TEST_F(ProfilerTest, StringTags) {
    AXIOM_PROFILE_SCOPE("TaggedScope");

    AXIOM_PROFILE_TAG("State", "Initializing");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    AXIOM_PROFILE_TAG("State", "Processing");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    AXIOM_PROFILE_TAG("State", "Finalizing");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    SUCCEED();
}
