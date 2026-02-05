# Building Axiom Physics Engine

This guide explains how to build, test, and develop the Axiom Physics Engine.

## Prerequisites

### Required Tools

- **CMake** 3.24 or later
- **C++20 compiler**:
  - Windows: MSVC 2022 or later
  - Linux: GCC 11+ or Clang 14+
  - macOS: Xcode 14+ or Clang 14+
- **vcpkg** (for dependency management)
- **Vulkan SDK** 1.3 or later
- **Slang Compiler** (slangc) for shader compilation

### Installing Slang

Download and install Slang from [shader-slang/slang releases](https://github.com/shader-slang/slang/releases).

After installation, ensure `slangc` is in your PATH:

```bash
# Windows (PowerShell)
$env:PATH += ";C:\path\to\slang\bin"

# Linux/macOS
export PATH=$PATH:/path/to/slang/bin

# Verify installation
slangc --version
```

### Setting up vcpkg

```bash
# Set vcpkg root environment variable (first time only)
# Windows (PowerShell)
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

# Linux/macOS
export VCPKG_ROOT=/path/to/vcpkg
```

## Build Instructions

### 1. Compile Shaders

Before building the project, compile the Slang shaders to SPIR-V:

```bash
# Using justfile (recommended)
just compile-shaders

# Or manually
slangc -target spirv -entry main shaders/test/simple.slang -o shaders/test/simple.comp.spv
slangc -target spirv -entry main shaders/test/array_add.slang -o shaders/test/array_add.comp.spv
```

**Note:** Shader compilation is currently manual. Future improvements will integrate this into the CMake build process.

### 2. Configure and Build

#### Debug Build

```bash
# Using CMake presets
cmake --preset windows-debug
cmake --build build/windows-debug

# Using justfile
just build-debug
```

#### Release Build

```bash
cmake --preset windows-release
cmake --build build/windows-release

# Using justfile
just build-release
```

#### Build with Profiling Enabled

```bash
cmake --preset windows-relwithdebinfo -DAXIOM_ENABLE_PROFILING=ON
cmake --build build/windows-relwithdebinfo
```

### 3. Run Tests

```bash
# Run all tests
ctest --preset windows-debug

# Run specific test
./build/windows-debug/bin/axiom_tests --gtest_filter="ComputeShaderTest.*"

# Using justfile
just test
```

## Development Workflow

### Installing just

`just` is a handy command runner that simplifies development tasks:

```bash
# Windows (PowerShell)
winget install --id Casey.Just

# macOS
brew install just

# Linux
cargo install just
```

### Quick Start

```bash
# Show all available commands
just --list

# Full build (compile shaders + build + test)
just full-build
just test

# Or use the development workflow command
just dev-build
```

### Testing GPU Compute Pipeline

The compute shader test (`tests/gpu/test_compute.cpp`) verifies the Vulkan compute infrastructure:

```bash
# Build and run compute tests
just build-debug
./build/windows-debug/bin/axiom_tests --gtest_filter="ComputeShaderTest.*"
```

**Note:** GPU tests will be skipped in CI environments without GPU support. This is expected behavior.

## Troubleshooting

### Shader Not Found

If tests fail with "Test shader not found":

1. Ensure shaders are compiled: `just compile-shaders`
2. Check that `.spv` files exist in `shaders/test/`
3. Verify slangc is installed and in PATH

### Vulkan Not Available

If tests are skipped with "Vulkan not available":

1. Install Vulkan SDK
2. Ensure GPU drivers are up to date
3. On Linux, install Vulkan loader: `sudo apt install vulkan-tools libvulkan-dev`

### Build Errors

If CMake configuration fails:

1. Verify vcpkg is set up: `echo $VCPKG_ROOT`
2. Update vcpkg: `git -C $VCPKG_ROOT pull`
3. Clear CMake cache: `rm -rf build/` and reconfigure

## Platform-Specific Notes

### Windows (MSVC)

- Use "x64 Native Tools Command Prompt for VS 2022"
- Ensure Vulkan SDK is in PATH

### Linux

```bash
# Install required packages
sudo apt install build-essential cmake ninja-build
sudo apt install vulkan-tools libvulkan-dev
```

### macOS

MoltenVK is required for Vulkan support on macOS:

```bash
brew install molten-vk
export VK_ICD_FILENAMES=/usr/local/share/vulkan/icd.d/MoltenVK_icd.json
```

## Continuous Integration

CI environments may not have GPU support. GPU-related tests use `GTEST_SKIP()` to gracefully skip when Vulkan is unavailable.

To test locally without GPU:

```bash
# Tests will skip GPU tests but run CPU-only tests
just test
```

## Performance Profiling

To enable Tracy profiler integration:

```bash
cmake --preset windows-relwithdebinfo -DAXIOM_ENABLE_PROFILING=ON
cmake --build build/windows-relwithdebinfo

# Run with Tracy server connected
./build/windows-relwithdebinfo/bin/axiom_example
```

See `CLAUDE.md` for profiling macro usage.

## Additional Resources

- **Project Structure**: See `CLAUDE.md`
- **Coding Guidelines**: See `CONTRIBUTING.md`
- **Roadmap**: See `docs/roadmap/`
- **Architecture**: See `docs/ARCHITECTURE.md`
