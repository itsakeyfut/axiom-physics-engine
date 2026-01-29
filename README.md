# Axiom Physics Engine

A next-generation realistic physics engine built with C++20 and Vulkan.

## Overview

Axiom is a high-performance physics engine targeting 120 FPS (8.33ms/frame) that integrates:
- Rigid body dynamics
- Soft body simulation (XPBD)
- Fluid simulation (SPH)
- Gas simulation (Navier-Stokes)
- Destruction systems

## Features

- **Modern C++20**: Leveraging the latest C++ features
- **Vulkan GPU Acceleration**: High-performance GPU compute for physics simulation
- **Modular Architecture**: Clean separation of concerns with well-defined dependencies
- **Cross-Platform**: Windows, Linux, and macOS support

## Requirements

- C++20 compatible compiler (MSVC 2022, GCC 11+, Clang 13+)
- CMake 3.24+
- vcpkg (for dependency management)
- Vulkan SDK 1.3+

## Building

### Setting up vcpkg

First time setup:
```bash
set VCPKG_ROOT=C:\path\to\vcpkg
```

### Build commands

```bash
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

## Testing

```bash
# Run all tests
ctest --preset windows-debug

# Run specific test
./build/windows-debug/bin/axiom_tests --gtest_filter="GJKTest.*"

# Run benchmarks
./build/windows-release/bin/axiom_benchmark
```

## Project Structure

```
axiom/
├── src/           # Source code
│   ├── core/      # Core functionality
│   ├── math/      # Math library
│   ├── memory/    # Memory management
│   ├── collision/ # Collision detection
│   ├── dynamics/  # Rigid body physics
│   ├── softbody/  # Soft body simulation
│   ├── fluid/     # Fluid simulation
│   └── gpu/       # GPU compute
├── include/       # Public headers
├── tests/         # Test code
├── benchmarks/    # Benchmarks
├── examples/      # Sample code
├── docs/          # Documentation
└── third_party/   # Third-party libraries
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Development Roadmap](docs/roadmap/)
- [Contributing Guidelines](CONTRIBUTING.md)

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## Status

This project is currently in Phase 1 of development: Project Foundation and Build System.
See [Phase 1 Roadmap](docs/roadmap/phase-1/ROADMAP.md) for details.
