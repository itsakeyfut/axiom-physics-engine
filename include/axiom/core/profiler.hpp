#pragma once

/**
 * @file profiler.hpp
 * @brief Performance profiling infrastructure with Tracy Profiler integration
 *
 * This header provides profiling macros that integrate with Tracy Profiler when
 * AXIOM_ENABLE_PROFILING is defined. When profiling is disabled, all macros
 * expand to no-ops with zero runtime cost.
 *
 * Usage:
 * @code
 * void PhysicsWorld::step(float dt) {
 *     AXIOM_PROFILE_FUNCTION();  // Profile entire function
 *
 *     {
 *         AXIOM_PROFILE_SCOPE("Broadphase");
 *         broadphase_.update();
 *         AXIOM_PROFILE_VALUE("BroadphasePairs", broadphase_.getPairCount());
 *     }
 *
 *     AXIOM_PROFILE_FRAME();  // Mark end of frame
 * }
 * @endcode
 *
 * Build with profiling enabled:
 * @code
 * cmake --preset windows-relwithdebinfo -DAXIOM_ENABLE_PROFILING=ON
 * cmake --build build/windows-relwithdebinfo
 * @endcode
 *
 * @see https://github.com/wolfpld/tracy
 */

#ifdef AXIOM_ENABLE_PROFILING

// Include Tracy Profiler headers
#include <tracy/Tracy.hpp>

/**
 * @brief Mark the end of a frame for per-frame statistics
 *
 * Place this at the end of your main loop to track frame boundaries.
 * Tracy uses this to aggregate per-frame statistics and detect performance issues.
 */
#define AXIOM_PROFILE_FRAME() FrameMark

/**
 * @brief Profile a named scope
 * @param name String literal for the scope name (must be a compile-time constant)
 *
 * Creates a profiling zone for the current scope. The zone ends when
 * execution leaves the scope (RAII style).
 *
 * Example:
 * @code
 * {
 *     AXIOM_PROFILE_SCOPE("Collision Detection");
 *     detectCollisions();
 * }
 * @endcode
 */
#define AXIOM_PROFILE_SCOPE(name) ZoneScopedN(name)

/**
 * @brief Profile the current function
 *
 * Automatically uses the function name as the zone name.
 * Place this at the beginning of functions you want to profile.
 *
 * Example:
 * @code
 * void update() {
 *     AXIOM_PROFILE_FUNCTION();
 *     // function body
 * }
 * @endcode
 */
#define AXIOM_PROFILE_FUNCTION() ZoneScoped

/**
 * @brief Add a text annotation to the current profiling zone
 * @param name Unused parameter (kept for API compatibility)
 * @param val String to display in the profiling zone
 *
 * Adds descriptive text to the current zone. Useful for tracking
 * dynamic values or state information.
 *
 * Example:
 * @code
 * AXIOM_PROFILE_TAG("State", "Solving");
 * @endcode
 */
#define AXIOM_PROFILE_TAG(name, val) ZoneText(val, strlen(val))

/**
 * @brief Plot a numeric value for visualization
 * @param name Name of the plot (string literal)
 * @param val Numeric value to plot
 *
 * Creates a time-series plot in Tracy that can be visualized
 * alongside timing data. Useful for tracking counters, memory usage, etc.
 *
 * Example:
 * @code
 * AXIOM_PROFILE_VALUE("ContactCount", numContacts);
 * AXIOM_PROFILE_VALUE("MemoryUsage", allocatedBytes);
 * @endcode
 */
#define AXIOM_PROFILE_VALUE(name, val) TracyPlot(name, val)

/**
 * @brief Profile a GPU zone (Vulkan)
 * @param ctx Tracy Vulkan context
 * @param name Zone name (string literal)
 *
 * Tracks GPU execution time for Vulkan compute or rendering operations.
 * Requires Tracy Vulkan context to be initialized.
 *
 * Example:
 * @code
 * AXIOM_PROFILE_GPU_ZONE(tracyCtx, "Particle Update");
 * @endcode
 */
#define AXIOM_PROFILE_GPU_ZONE(ctx, name) TracyVkZone(ctx, name)

/**
 * @brief Collect GPU profiling data (Vulkan)
 * @param ctx Tracy Vulkan context
 *
 * Retrieves GPU timing data from Vulkan. Call this after GPU work
 * has completed to upload profiling results to Tracy.
 */
#define AXIOM_PROFILE_GPU_COLLECT(ctx) TracyVkCollect(ctx)

/**
 * @brief Track memory allocation
 * @param ptr Pointer to allocated memory
 * @param size Size of allocation in bytes
 *
 * Registers a memory allocation with Tracy's memory profiler.
 * Pair with AXIOM_PROFILE_FREE for accurate leak detection.
 *
 * Example:
 * @code
 * void* ptr = malloc(size);
 * AXIOM_PROFILE_ALLOC(ptr, size);
 * @endcode
 */
#define AXIOM_PROFILE_ALLOC(ptr, size) TracyAlloc(ptr, size)

/**
 * @brief Track memory deallocation
 * @param ptr Pointer to memory being freed
 *
 * Unregisters a memory allocation from Tracy's memory profiler.
 * Must match a previous AXIOM_PROFILE_ALLOC call.
 *
 * Example:
 * @code
 * AXIOM_PROFILE_FREE(ptr);
 * free(ptr);
 * @endcode
 */
#define AXIOM_PROFILE_FREE(ptr) TracyFree(ptr)

#else  // AXIOM_ENABLE_PROFILING not defined

// No-op macros when profiling is disabled
// These expand to empty statements with zero runtime cost

/// @brief No-op when profiling disabled
#define AXIOM_PROFILE_FRAME() ((void)0)

/// @brief No-op when profiling disabled
#define AXIOM_PROFILE_SCOPE(name) ((void)0)

/// @brief No-op when profiling disabled
#define AXIOM_PROFILE_FUNCTION() ((void)0)

/// @brief No-op when profiling disabled
#define AXIOM_PROFILE_TAG(name, val) ((void)0)

/// @brief No-op when profiling disabled
#define AXIOM_PROFILE_VALUE(name, val) ((void)0)

/// @brief No-op when profiling disabled
#define AXIOM_PROFILE_GPU_ZONE(ctx, name) ((void)0)

/// @brief No-op when profiling disabled
#define AXIOM_PROFILE_GPU_COLLECT(ctx) ((void)0)

/// @brief No-op when profiling disabled
#define AXIOM_PROFILE_ALLOC(ptr, size) ((void)0)

/// @brief No-op when profiling disabled
#define AXIOM_PROFILE_FREE(ptr) ((void)0)

#endif  // AXIOM_ENABLE_PROFILING
