# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Axiom Physics Engine - A next-generation realistic physics engine built with C++20 and Vulkan. Targets 120 FPS (8.33ms/frame) and integrates rigid bodies, soft bodies (XPBD), fluids (SPH), gas (Navier-Stokes), and destruction systems.

## Build Commands

```bash
# Set vcpkg environment variable (first time only)
set VCPKG_ROOT=C:\path\to\vcpkg

# Debug build
cmake --preset windows-debug
cmake --build build/windows-debug

# Release build
cmake --preset windows-release
cmake --build build/windows-release

# Build with Tracy Profiler enabled
cmake --preset windows-relwithdebinfo -DAXIOM_ENABLE_PROFILING=ON
cmake --build build/windows-relwithdebinfo
```

## Test Commands

```bash
# Run all tests
ctest --preset windows-debug

# Run specific test (Google Test)
./build/windows-debug/bin/axiom_tests --gtest_filter="GJKTest.*"

# Run benchmarks
./build/windows-release/bin/axiom_benchmark
```

## Architecture

### Module Structure

- **core/**: Math (Vec3, Mat4, Quaternion), memory management, threading (JobSystem)
- **collision/**: Collision detection (BVH, GJK, EPA), shapes (Sphere, Box, Capsule), scene queries
- **dynamics/**: Rigid body physics, constraint solver, joints
- **softbody/**: XPBD solver, cloth simulation
- **fluid/**: SPH fluid simulation (CPU/GPU)
- **gas/**: Navier-Stokes gas simulation
- **destruction/**: Voronoi-based fracture system
- **character/**: Character controller (capsule shape)
- **world/**: PhysicsWorld, UnifiedPhysicsWorld

### Dependency Rules

1. Top-down only (lower modules cannot depend on upper modules)
2. `core` can be depended on by all modules
3. Cross-module coupling goes through `coupling_manager`

### Namespaces

```cpp
axiom::core::math      // Vec3, Mat4, Transform
axiom::collision       // GJK, BVH, shapes
axiom::dynamics        // RigidBody, constraints
axiom::fluid::sph      // SPHSolver
axiom::detail          // Internal implementation (not public API)
```

## Coding Conventions

| Type | Convention | Example |
|------|------------|---------|
| Files | snake_case | `rigid_body.hpp` |
| Classes | PascalCase | `RigidBody` |
| Functions | camelCase | `getPosition()` |
| Member variables | camelCase + `_` | `position_` |
| Constants | UPPER_SNAKE_CASE | `MAX_CONTACTS` |

### Include Guards

Use `#pragma once`

### Include Order

1. Corresponding header
2. Project headers (`axiom/`)
3. Third-party (glm, spdlog)
4. Standard library

## Key Design Decisions

- **GJK-based CCD**: Prevents tunneling for fast-moving objects
- **Single library output**: All modules compiled into `axiom.lib/axiom.dll`
- **GPU implementations**: Located in `gpu/` subdirectory within each module
- **Error handling**: Graceful recovery, no exceptions
- **Profiling**: Tracy integration (enable with `AXIOM_ENABLE_PROFILING`)

## Profiling

The engine integrates with Tracy Profiler for performance analysis. Profiling macros are available in `axiom/core/profiler.hpp`.

### Build with Profiling

```bash
# Enable profiling in CMake configuration
cmake --preset windows-relwithdebinfo -DAXIOM_ENABLE_PROFILING=ON
cmake --build build/windows-relwithdebinfo

# Install Tracy dependency (if not already installed)
vcpkg install --features=tracy
```

### Usage

```cpp
#include <axiom/core/profiler.hpp>

void PhysicsWorld::step(float dt) {
    AXIOM_PROFILE_FUNCTION();  // Profile entire function

    {
        AXIOM_PROFILE_SCOPE("Broadphase");
        broadphase_.update();
        AXIOM_PROFILE_VALUE("BroadphasePairs", broadphase_.getPairCount());
    }

    {
        AXIOM_PROFILE_SCOPE("Narrowphase");
        narrowphase_.detectCollisions();
        AXIOM_PROFILE_VALUE("ContactCount", narrowphase_.getContactCount());
    }

    AXIOM_PROFILE_FRAME();  // Mark end of frame
}
```

### Available Macros

- `AXIOM_PROFILE_FRAME()` - Mark frame boundary for per-frame statistics
- `AXIOM_PROFILE_SCOPE(name)` - Profile a named scope (RAII)
- `AXIOM_PROFILE_FUNCTION()` - Profile current function (uses `__FUNCTION__`)
- `AXIOM_PROFILE_VALUE(name, val)` - Plot numeric value over time
- `AXIOM_PROFILE_TAG(name, val)` - Add text annotation to current zone
- `AXIOM_PROFILE_ALLOC(ptr, size)` - Track memory allocation
- `AXIOM_PROFILE_FREE(ptr)` - Track memory deallocation
- `AXIOM_PROFILE_GPU_ZONE(ctx, name)` - Profile GPU work (Vulkan)
- `AXIOM_PROFILE_GPU_COLLECT(ctx)` - Collect GPU profiling data

When `AXIOM_ENABLE_PROFILING` is not defined, all macros expand to no-ops with zero runtime cost.

### Viewing Results

1. Build the project with profiling enabled
2. Run the Tracy server application
3. Launch your application
4. Tracy will automatically connect and display profiling data
5. Results can be exported to Chrome Tracing format for visualization in `chrome://tracing`

See [Tracy documentation](https://github.com/wolfpld/tracy) for more details.

## Documentation

- **Specifications & Design**: `docs/` (numbered documents 01-18, in Japanese)
- **Architecture**: `docs/ARCHITECTURE.md`
- **Roadmap**: `docs/roadmap/phase-{1..10}/`
