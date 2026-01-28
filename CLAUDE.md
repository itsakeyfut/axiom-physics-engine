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
cmake --preset windows-relwithdebinfo -DAXIOM_ENABLE_TRACY=ON
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
- **Profiling**: Tracy integration (enable with `AXIOM_ENABLE_TRACY`)

## Documentation

- **Specifications & Design**: `docs/` (numbered documents 01-18, in Japanese)
- **Architecture**: `docs/ARCHITECTURE.md`
- **Roadmap**: `docs/roadmap/phase-{1..10}/`
