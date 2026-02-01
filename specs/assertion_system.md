# Assertion System

The Axiom Physics Engine provides a comprehensive assertion system for debug-time validation of preconditions, postconditions, and invariants.

## Overview

Assertions are used to catch **programmer errors** (bugs in the code) during development, while the `Result<T>` type is used for **expected errors** (invalid user input, external failures). This distinction is crucial for robust error handling.

## Assertion Macros

### AXIOM_ASSERT(expr, msg)

Core assertion macro for general validation checks.

- **Enabled**: Debug and RelWithDebInfo builds
- **Disabled**: Release builds (compiled to no-op)
- **Use case**: General programmer error checking

```cpp
void setVelocity(const Vec3& velocity) {
    AXIOM_ASSERT(!std::isnan(velocity.x), "Velocity contains NaN");
    velocity_ = velocity;
}
```

### AXIOM_VERIFY(expr, msg)

Verification macro that always runs, even in Release builds.

- **Enabled**: All build configurations
- **Use case**: Critical checks that must always execute
- **Note**: Expression is always evaluated (preserves side effects)

```cpp
void updatePhysics(float deltaTime) {
    AXIOM_VERIFY(deltaTime > 0.0f, "Delta time must be positive");
    // Physics update code...
}
```

### AXIOM_PRECONDITION(expr)

Validates function entry conditions.

- **Enabled**: Debug and RelWithDebInfo builds
- **Disabled**: Release builds
- **Use case**: Validate input parameters and object state at function entry

```cpp
Result<Vec3> normalize(const Vec3& v) {
    AXIOM_PRECONDITION(!std::isnan(v.x));
    AXIOM_PRECONDITION(!std::isnan(v.y));
    AXIOM_PRECONDITION(!std::isnan(v.z));

    float len = length(v);
    if (len < EPSILON) {
        return Result<Vec3>::failure(
            ErrorCode::NormalizationOfZeroVector,
            "Cannot normalize zero-length vector"
        );
    }

    return Result<Vec3>::success(v / len);
}
```

### AXIOM_POSTCONDITION(expr)

Validates function exit conditions.

- **Enabled**: Debug and RelWithDebInfo builds
- **Disabled**: Release builds
- **Use case**: Validate return values and object state at function exit

```cpp
Mat4 inverse(const Mat4& m) {
    AXIOM_PRECONDITION(determinant(m) != 0.0f);

    Mat4 result = calculateInverse(m);

    AXIOM_POSTCONDITION(almostEqual(result * m, Mat4::identity()));
    return result;
}
```

### AXIOM_UNREACHABLE()

Marks code paths that should never execute.

- **Debug/RelWithDebInfo**: Aborts with error message
- **Release**: Provides compiler hint for optimization (UB if reached)
- **Use case**: Mark unreachable code in switch statements, etc.

```cpp
Vec3 getAxis(Axis axis) {
    switch (axis) {
        case Axis::X: return Vec3(1, 0, 0);
        case Axis::Y: return Vec3(0, 1, 0);
        case Axis::Z: return Vec3(0, 0, 1);
    }
    AXIOM_UNREACHABLE();
}
```

## Assertions vs Result&lt;T&gt;

### Use Assertions For:

- **Programmer errors** that should never happen in correct code
- **Precondition violations** (invalid function parameters)
- **Invariant violations** (internal consistency checks)
- **Logic errors** in the physics engine itself

### Use Result&lt;T&gt; For:

- **Expected errors** that can occur during normal operation
- **User input validation** (invalid configuration, bad file data)
- **External failures** (file not found, network errors)
- **Recoverable errors** that the caller should handle

### Example: Combined Usage

```cpp
Result<RigidBody*> createRigidBody(const RigidBodyDesc& desc) {
    // Assertions check for programmer errors (passing invalid data structures)
    AXIOM_PRECONDITION(desc.mass >= 0.0f);
    AXIOM_PRECONDITION(desc.shape != nullptr);

    // Result<T> handles expected errors (out of memory, invalid configuration)
    if (desc.mass == 0.0f && !desc.isStatic) {
        return Result<RigidBody*>::failure(
            ErrorCode::InvalidParameter,
            "Dynamic rigid body must have non-zero mass"
        );
    }

    RigidBody* body = allocate<RigidBody>();
    if (!body) {
        return Result<RigidBody*>::failure(
            ErrorCode::OutOfMemory,
            "Failed to allocate rigid body"
        );
    }

    initializeBody(body, desc);

    AXIOM_POSTCONDITION(body->isInitialized());
    return Result<RigidBody*>::success(body);
}
```

## Custom Assertion Handlers

You can register a custom handler to control assertion failure behavior:

```cpp
#include "axiom/core/assert.hpp"

void myAssertHandler(const char* expr, const char* msg,
                     const char* file, int line) {
    // Log to custom logging system
    myLogger.error("Assertion failed: {} at {}:{}", msg, file, line);

    // Show dialog in debug tools
    showAssertDialog(expr, msg, file, line);

    // Crash dump for post-mortem debugging
    generateCrashDump();

    // Must abort or throw
    std::abort();
}

int main() {
    axiom::core::setAssertHandler(myAssertHandler);
    // ... application code
}
```

## Stack Trace Support

When an assertion fails, the system automatically captures and displays a stack trace (platform-dependent):

- **Windows**: Uses `CaptureStackBackTrace` and `DbgHelp` API
- **Linux/macOS**: Uses `backtrace()` and demangled symbols
- **Fallback**: Displays file and line number only

Example output:

```
================================================================================
ASSERTION FAILED
================================================================================
Expression: velocity.length() > 0.0f
Message:    Velocity must be non-zero
Location:   src/dynamics/rigid_body.cpp:142

Stack trace:
  [0] axiom::dynamics::RigidBody::applyForce
  [1] axiom::dynamics::PhysicsWorld::stepSimulation
  [2] main
================================================================================
```

## Build Configuration Behavior

| Macro | Debug | RelWithDebInfo | Release |
|-------|-------|----------------|---------|
| `AXIOM_ASSERT` | ✓ Enabled | ✓ Enabled | ✗ Disabled |
| `AXIOM_VERIFY` | ✓ Enabled | ✓ Enabled | ✓ Enabled |
| `AXIOM_PRECONDITION` | ✓ Enabled | ✓ Enabled | ✗ Disabled |
| `AXIOM_POSTCONDITION` | ✓ Enabled | ✓ Enabled | ✗ Disabled |
| `AXIOM_UNREACHABLE` | Abort | Abort | UB hint |

## Performance Considerations

- **Debug builds**: Assertions add validation overhead for catching bugs early
- **Release builds**: Most assertions compile to no-ops (zero overhead)
- **AXIOM_VERIFY**: Always evaluates expression, use sparingly in hot paths

## Best Practices

1. **Check preconditions early**: Validate inputs at function entry
2. **Keep expressions simple**: Avoid complex computations in assertions
3. **Provide helpful messages**: Explain what went wrong and why
4. **Don't rely on side effects**: Assertion expressions may be disabled
5. **Use Result&lt;T&gt; for user errors**: Assertions are for programmer errors only
6. **Document assumptions**: Assertions serve as executable documentation

## Integration with Result&lt;T&gt;

The `Result<T>::value()` method automatically uses assertions to detect misuse:

```cpp
Result<int> result = computeSomething();

// Safe: Check before accessing
if (result.isSuccess()) {
    int value = result.value();  // OK
}

// Unsafe: Will assert in debug builds
int value = result.value();  // Assertion failure if result is error!
```

This catches programmer errors where you forget to check for errors before accessing the value.

## References

- Implementation: `include/axiom/core/assert.hpp`, `src/core/assert.cpp`
- Tests: `tests/core/assert_test.cpp`
- Roadmap: `docs/roadmap/phase-1/ROADMAP.md` - Issue #021
