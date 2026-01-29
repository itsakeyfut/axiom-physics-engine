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
- Vulkan SDK 1.3+ (optional, for future GPU features)

## Dependencies

This project uses vcpkg for dependency management. The following dependencies are automatically managed:

- **GLM** (>= 1.0.0): Mathematics library for graphics and physics
- **spdlog** (>= 1.12.0): Fast C++ logging library
- **Google Test** (>= 1.14.0): Unit testing framework
- **Google Benchmark** (>= 1.8.0): Performance benchmarking framework
- **Tracy** (optional): Profiler for performance analysis

## Building

### Setting up vcpkg

If you don't have vcpkg installed, follow these steps:

#### Windows

```bash
# Clone vcpkg
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Set VCPKG_ROOT environment variable (add to system environment variables for persistence)
set VCPKG_ROOT=C:\vcpkg

# Optionally, add vcpkg to PATH
set PATH=%PATH%;C:\vcpkg
```

#### Linux

```bash
# Clone vcpkg
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
cd ~/vcpkg

# Bootstrap vcpkg
./bootstrap-vcpkg.sh

# Set VCPKG_ROOT environment variable (add to ~/.bashrc or ~/.zshrc for persistence)
export VCPKG_ROOT=~/vcpkg

# Optionally, add vcpkg to PATH
export PATH=$PATH:~/vcpkg
```

#### macOS

```bash
# Clone vcpkg
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
cd ~/vcpkg

# Bootstrap vcpkg
./bootstrap-vcpkg.sh

# Set VCPKG_ROOT environment variable (add to ~/.zshrc for persistence)
export VCPKG_ROOT=~/vcpkg

# Optionally, add vcpkg to PATH
export PATH=$PATH:~/vcpkg
```

### Installing Dependencies

Dependencies are managed via `vcpkg.json` manifest and will be installed automatically during CMake configuration.

To manually install dependencies:

```bash
# Windows
vcpkg install --triplet x64-windows

# Linux
vcpkg install --triplet x64-linux

# macOS
vcpkg install --triplet x64-osx
```

### Build Commands

#### Windows

```bash
# Configure and build (debug)
cmake --preset windows-debug
cmake --build build/windows-debug

# Configure and build (release)
cmake --preset windows-release
cmake --build build/windows-release

# Build with Tracy Profiler enabled
cmake --preset windows-relwithdebinfo -DAXIOM_ENABLE_PROFILING=ON
cmake --build build/windows-relwithdebinfo
```

#### Linux

```bash
# Configure and build (debug)
cmake --preset linux-debug
cmake --build build/linux-debug

# Configure and build (release)
cmake --preset linux-release
cmake --build build/linux-release
```

#### macOS

```bash
# Configure and build (debug)
cmake --preset macos-debug
cmake --build build/macos-debug

# Configure and build (release)
cmake --preset macos-release
cmake --build build/macos-release
```

### Using the Makefile

For convenience, a Makefile is provided with common commands:

```bash
# Show available commands
make help

# Configure and build
make build

# Build only (after configuration)
make compile

# Run tests
make test

# Format code
make format

# Run linter
make lint

# Clean build artifacts
make clean
```

## Testing

### Running Tests

```bash
# Windows - Run all tests
ctest --preset windows-debug

# Linux - Run all tests
ctest --preset linux-debug

# macOS - Run all tests
ctest --preset macos-debug

# Run specific test (Google Test filter)
./build/windows-debug/bin/axiom_tests --gtest_filter="GJKTest.*"

# Run benchmarks
./build/windows-release/bin/axiom_benchmark
```

## Troubleshooting

### vcpkg Issues

**Error: "builtin-baseline was not a valid commit sha"**

Update vcpkg to the latest version:
```bash
cd $VCPKG_ROOT
git pull
./bootstrap-vcpkg.bat  # Windows
./bootstrap-vcpkg.sh   # Linux/macOS
```

**Error: "Could not find CMAKE_TOOLCHAIN_FILE"**

Ensure `VCPKG_ROOT` environment variable is set correctly:
```bash
# Windows
echo %VCPKG_ROOT%

# Linux/macOS
echo $VCPKG_ROOT
```

**Error: Package installation fails**

Clear vcpkg cache and try again:
```bash
cd $VCPKG_ROOT
rm -rf buildtrees packages
vcpkg install --triplet x64-windows  # or your platform triplet
```

### CMake Issues

**Error: "Could not find package"**

Ensure dependencies are installed via vcpkg:
```bash
# From project root
vcpkg install --triplet x64-windows  # or your platform triplet
```

**Error: "Ninja not found"**

Install Ninja build system:
```bash
# Windows (using vcpkg)
vcpkg install ninja

# Linux
sudo apt install ninja-build  # Debian/Ubuntu
sudo dnf install ninja-build  # Fedora

# macOS
brew install ninja
```

### Build Issues

**Error: C++20 features not supported**

Verify compiler version:
```bash
# Windows (MSVC)
cl /?  # Should be version 19.29 or later (VS 2022)

# Linux (GCC)
g++ --version  # Should be 11.0 or later

# macOS (Clang)
clang++ --version  # Should be 13.0 or later
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
