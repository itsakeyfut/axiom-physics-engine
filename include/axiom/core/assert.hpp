#pragma once

#include <cstdlib>

namespace axiom::core {

/// Function pointer type for custom assertion failure handlers
/// @param expr The assertion expression that failed (as string)
/// @param msg Optional message describing the failure
/// @param file Source file where the assertion failed
/// @param line Line number where the assertion failed
using AssertHandler = void (*)(const char* expr, const char* msg, const char* file, int line);

/// Set a custom assertion failure handler
/// @param handler The custom handler to use. Pass nullptr to restore default handler.
/// @note The handler will be called when an assertion fails before the program aborts
/// @note This function is thread-safe (uses atomic operations internally)
void setAssertHandler(AssertHandler handler) noexcept;

/// Default assertion failure handler
/// Prints assertion details to stderr and optionally captures stack trace
/// @param expr The assertion expression that failed (as string)
/// @param msg Optional message describing the failure
/// @param file Source file where the assertion failed
/// @param line Line number where the assertion failed
[[noreturn]] void defaultAssertHandler(const char* expr,
                                       const char* msg,
                                       const char* file,
                                       int line) noexcept;

/// Internal function called when an assertion fails
/// @param expr The assertion expression that failed (as string)
/// @param msg Optional message describing the failure
/// @param file Source file where the assertion failed
/// @param line Line number where the assertion failed
[[noreturn]] void assertionFailed(const char* expr,
                                  const char* msg,
                                  const char* file,
                                  int line) noexcept;

}  // namespace axiom::core

// Platform detection for stack trace support
#if defined(_WIN32) || defined(_WIN64)
#define AXIOM_PLATFORM_WINDOWS 1
#elif defined(__linux__)
#define AXIOM_PLATFORM_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define AXIOM_PLATFORM_MACOS 1
#else
#define AXIOM_PLATFORM_UNKNOWN 1
#endif

// Build configuration detection
#if !defined(NDEBUG)
#define AXIOM_DEBUG_BUILD 1
#else
#define AXIOM_DEBUG_BUILD 0
#endif

// Check if this is a RelWithDebInfo build (has both optimizations and debug info)
#if defined(NDEBUG) && (defined(_DEBUG) || !defined(__OPTIMIZE__))
#define AXIOM_RELWITHDEBINFO_BUILD 1
#else
#define AXIOM_RELWITHDEBINFO_BUILD 0
#endif

// Determine if assertions should be enabled
#if AXIOM_DEBUG_BUILD || AXIOM_RELWITHDEBINFO_BUILD
#define AXIOM_ASSERTIONS_ENABLED 1
#else
#define AXIOM_ASSERTIONS_ENABLED 0
#endif

//=============================================================================
// AXIOM_ASSERT: Core assertion macro
// - Enabled in Debug and RelWithDebInfo builds
// - Disabled in Release builds (no-op)
// - Use for general programmer error checking
//=============================================================================

#if AXIOM_ASSERTIONS_ENABLED
#define AXIOM_ASSERT(expr, msg)                                                              \
    do {                                                                                     \
        if (!(expr)) {                                                                       \
            ::axiom::core::assertionFailed(#expr, msg, __FILE__, __LINE__);                 \
        }                                                                                    \
    } while (false)
#else
#define AXIOM_ASSERT(expr, msg) ((void)0)
#endif

//=============================================================================
// AXIOM_VERIFY: Verification macro
// - Always enabled in all build configurations
// - Use for critical checks that must run even in Release builds
// - The expression is always evaluated (side effects are preserved)
//=============================================================================

#define AXIOM_VERIFY(expr, msg)                                                              \
    do {                                                                                     \
        if (!(expr)) {                                                                       \
            ::axiom::core::assertionFailed(#expr, msg, __FILE__, __LINE__);                 \
        }                                                                                    \
    } while (false)

//=============================================================================
// AXIOM_PRECONDITION: Precondition assertion
// - Enabled in Debug and RelWithDebInfo builds
// - Disabled in Release builds
// - Use at function entry to validate input parameters and state
//=============================================================================

#if AXIOM_ASSERTIONS_ENABLED
#define AXIOM_PRECONDITION(expr)                                                             \
    do {                                                                                     \
        if (!(expr)) {                                                                       \
            ::axiom::core::assertionFailed(#expr, "Precondition violated", __FILE__,        \
                                           __LINE__);                                        \
        }                                                                                    \
    } while (false)
#else
#define AXIOM_PRECONDITION(expr) ((void)0)
#endif

//=============================================================================
// AXIOM_POSTCONDITION: Postcondition assertion
// - Enabled in Debug and RelWithDebInfo builds
// - Disabled in Release builds
// - Use at function exit to validate return values and state
//=============================================================================

#if AXIOM_ASSERTIONS_ENABLED
#define AXIOM_POSTCONDITION(expr)                                                            \
    do {                                                                                     \
        if (!(expr)) {                                                                       \
            ::axiom::core::assertionFailed(#expr, "Postcondition violated", __FILE__,       \
                                           __LINE__);                                        \
        }                                                                                    \
    } while (false)
#else
#define AXIOM_POSTCONDITION(expr) ((void)0)
#endif

//=============================================================================
// AXIOM_UNREACHABLE: Mark unreachable code paths
// - In Debug/RelWithDebInfo: Aborts with an error message
// - In Release: Provides compiler hint for optimization (undefined behavior if reached)
// - Use to mark code paths that should never execute
//=============================================================================

#if AXIOM_ASSERTIONS_ENABLED
#define AXIOM_UNREACHABLE()                                                                  \
    do {                                                                                     \
        ::axiom::core::assertionFailed("false", "Unreachable code reached", __FILE__,       \
                                       __LINE__);                                            \
    } while (false)
#else
// In Release builds, use compiler-specific unreachable hints
#if defined(__GNUC__) || defined(__clang__)
#define AXIOM_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
#define AXIOM_UNREACHABLE() __assume(0)
#else
#define AXIOM_UNREACHABLE() ((void)0)
#endif
#endif
