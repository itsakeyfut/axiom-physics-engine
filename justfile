# Axiom Physics Engine - Development Commands
# Modern task runner using just (https://github.com/casey/just)

# Use PowerShell on Windows, bash elsewhere
set windows-shell := ["powershell.exe", "-NoLogo", "-Command"]

# Configuration
preset_debug := if os() == "windows" { "windows-debug" } else if os() == "macos" { "macos-debug" } else { "linux-debug" }
preset_release := if os() == "windows" { "windows-release" } else if os() == "macos" { "macos-release" } else { "linux-release" }
preset_relwithdebinfo := if os() == "windows" { "windows-relwithdebinfo" } else if os() == "macos" { "macos-relwithdebinfo" } else { "linux-relwithdebinfo" }

build_dir_debug := "build/" + preset_debug
build_dir_release := "build/" + preset_release
build_dir_relwithdebinfo := "build/" + preset_relwithdebinfo

exe_suffix := if os() == "windows" { ".exe" } else { "" }

# Default recipe (show help)
default:
    @just --list

# ============================================================================
# General
# ============================================================================

# Show help with available commands
help:
    @echo "Axiom Physics Engine - Development Commands"
    @echo ""
    @echo "Platform: {{os()}}"
    @echo "Arch: {{arch()}}"
    @echo ""
    @just --list

# Display build information
info:
    @echo "Axiom Physics Engine - Build Information"
    @echo "========================================"
    @echo "Platform:           {{os()}}"
    @echo "Architecture:       {{arch()}}"
    @echo "Debug Preset:       {{preset_debug}}"
    @echo "Release Preset:     {{preset_release}}"
    @echo "RelWithDebInfo:     {{preset_relwithdebinfo}}"
    @echo ""
    @echo "Build Directories:"
    @echo "  Debug:            {{build_dir_debug}}"
    @echo "  Release:          {{build_dir_release}}"
    @echo "  RelWithDebInfo:   {{build_dir_relwithdebinfo}}"
    @echo ""
    @echo ""
    @cmake --version

# ============================================================================
# Dependencies
# ============================================================================

# Install vcpkg dependencies
vcpkg-install:
    @echo "Installing dependencies via vcpkg..."
    vcpkg install

# Update vcpkg and dependencies
vcpkg-update:
    @echo "Updating vcpkg..."
    git -C $env:VCPKG_ROOT pull
    vcpkg upgrade --no-dry-run

# ============================================================================
# Shaders
# ============================================================================

# Compile all test shaders to SPIR-V
compile-shaders:
    @echo "Compiling test shaders..."
    slangc -target spirv -entry main shaders/test/simple.slang -o shaders/test/simple.comp.spv
    slangc -target spirv -entry main shaders/test/array_add.slang -o shaders/test/array_add.comp.spv
    @echo "Shader compilation complete!"

# Compile specific shader
compile-shader shader output:
    @echo "Compiling {{shader}}..."
    slangc -target spirv -entry main {{shader}} -o {{output}}

# Clean compiled shaders
clean-shaders:
    @echo "Cleaning compiled shaders..."
    {{ if os() == "windows" { "Remove-Item -Force -ErrorAction SilentlyContinue shaders/**/*.spv" } else { "rm -f shaders/**/*.spv" } }}

# ============================================================================
# Build - Debug
# ============================================================================

# Configure Debug build
configure-debug:
    @echo "Configuring Debug build..."
    cmake --preset {{preset_debug}}

# Build Debug configuration
build-debug: configure-debug
    @echo "Building Debug..."
    cmake --build {{build_dir_debug}} --parallel

# Quick build Debug without reconfiguring
quick-build:
    @echo "Quick building Debug..."
    cmake --build {{build_dir_debug}} --parallel

# Clean and rebuild Debug
rebuild-debug: clean-debug build-debug

# ============================================================================
# Build - Release
# ============================================================================

# Configure Release build
configure-release:
    @echo "Configuring Release build..."
    cmake --preset {{preset_release}}

# Build Release configuration
build-release: configure-release
    @echo "Building Release..."
    cmake --build {{build_dir_release}} --parallel

# Clean and rebuild Release
rebuild-release: clean-release build-release

# ============================================================================
# Build - RelWithDebInfo
# ============================================================================

# Configure RelWithDebInfo build
configure-relwithdebinfo:
    @echo "Configuring RelWithDebInfo build..."
    cmake --preset {{preset_relwithdebinfo}}

# Build RelWithDebInfo configuration
build-relwithdebinfo: configure-relwithdebinfo
    @echo "Building RelWithDebInfo..."
    cmake --build {{build_dir_relwithdebinfo}} --parallel

# Clean and rebuild RelWithDebInfo
rebuild-relwithdebinfo: clean-relwithdebinfo build-relwithdebinfo

# ============================================================================
# Build - Shortcuts
# ============================================================================

# Alias for build-debug (default build)
build: build-debug

# Alias for build-release
release: build-release

# Build both Debug and Release
all: build-debug build-release

# Compile shaders and build project
full-build: compile-shaders build-debug

# ============================================================================
# Testing
# ============================================================================

# Run all tests (Debug)
test: build-debug
    @echo "Running tests..."
    ctest --preset {{preset_debug}} --output-on-failure

# Run tests with verbose output
test-verbose: build-debug
    @echo "Running tests (verbose)..."
    ctest --preset {{preset_debug}} --verbose

# Run tests (Release)
test-release: build-release
    @echo "Running tests (Release)..."
    ctest --preset {{preset_release}} --output-on-failure

# Quick test without full rebuild
quick-test: quick-build
    @echo "Quick testing..."
    ctest --preset {{preset_debug}} --output-on-failure

# Build and run tests
build-test: build-debug test

# Run specific test by name
test-filter filter: build-debug
    @echo "Running filtered tests: {{filter}}"
    {{build_dir_debug}}/bin/axiom_tests{{exe_suffix}} --gtest_filter="{{filter}}"

# ============================================================================
# Benchmarks
# ============================================================================

# Run all available benchmarks
benchmark: build-release
    @echo "Running all benchmarks..."
    {{build_dir_release}}/bin/pool_allocator_benchmark{{exe_suffix}} --benchmark_min_time=0.5s

# Run pool allocator benchmarks (all tests, 0.5s each)
benchmark-pool: build-release
    @echo "Running Pool Allocator benchmarks..."
    {{build_dir_release}}/bin/pool_allocator_benchmark{{exe_suffix}} --benchmark_min_time=0.5s

# Run filtered pool allocator benchmarks
benchmark-pool-filter filter: build-release
    @echo "Running filtered Pool Allocator benchmarks: {{filter}}"
    {{build_dir_release}}/bin/pool_allocator_benchmark{{exe_suffix}} --benchmark_filter={{filter}} --benchmark_min_time=0.5s

# Run pool allocator benchmarks (quick mode)
benchmark-pool-quick: build-release
    @echo "Running Pool Allocator benchmarks (quick mode)..."
    {{build_dir_release}}/bin/pool_allocator_benchmark{{exe_suffix}} --benchmark_min_time=0.1s

# Run pool allocator peak throughput benchmarks
benchmark-pool-peak: build-release
    @echo "Running Pool Allocator peak throughput benchmarks..."
    {{build_dir_release}}/bin/pool_allocator_benchmark{{exe_suffix}} --benchmark_filter=PeakThroughput --benchmark_min_time=1s

# Compare pool allocator vs heap/std allocators
benchmark-pool-compare: build-release
    @echo "Comparing Pool Allocator performance..."
    {{build_dir_release}}/bin/pool_allocator_benchmark{{exe_suffix}} --benchmark_filter="Sequential_Small/8192|PeakThroughput" --benchmark_min_time=1s

# Benchmark small object allocations
benchmark-pool-small: build-release
    @echo "Running small object benchmarks..."
    {{build_dir_release}}/bin/pool_allocator_benchmark{{exe_suffix}} --benchmark_filter="Small" --benchmark_min_time=0.5s

# Benchmark medium object allocations
benchmark-pool-medium: build-release
    @echo "Running medium object benchmarks..."
    {{build_dir_release}}/bin/pool_allocator_benchmark{{exe_suffix}} --benchmark_filter="Medium" --benchmark_min_time=0.5s

# Benchmark large object allocations
benchmark-pool-large: build-release
    @echo "Running large object benchmarks..."
    {{build_dir_release}}/bin/pool_allocator_benchmark{{exe_suffix}} --benchmark_filter="Large" --benchmark_min_time=0.5s

# ============================================================================
# Examples & Application
# ============================================================================

# Build examples
examples: build-debug
    @echo "Examples built in {{build_dir_debug}}/bin/"

# Run specific example
run-example example: build-debug
    @echo "Running example: {{example}}"
    {{build_dir_debug}}/bin/{{example}}{{exe_suffix}}

# Run the application (Debug)
run: build-debug
    @echo "Running Axiom Physics Engine (Debug)..."
    {{build_dir_debug}}/bin/axiom_app{{exe_suffix}}

# Run the application (Release)
run-release: build-release
    @echo "Running Axiom Physics Engine (Release)..."
    {{build_dir_release}}/bin/axiom_app{{exe_suffix}}

# Alias for 'run'
app: run

# ============================================================================
# Code Quality
# ============================================================================

# Format all source files with clang-format
format:
    @echo "Formatting source files..."
    {{ if os() == "windows" { "Get-ChildItem -Recurse -Include *.cpp,*.hpp,*.h src,include,tests,benchmarks,examples | ForEach-Object { clang-format -i $_.FullName }" } else { "find src include tests benchmarks examples -name '*.cpp' -o -name '*.hpp' -o -name '*.h' | xargs clang-format -i" } }}
    @echo "Format complete!"

# Check if code is properly formatted
format-check:
    @echo "Checking code format..."
    {{ if os() == "windows" { "Get-ChildItem -Recurse -Include *.cpp,*.hpp,*.h src,include,tests,benchmarks,examples | ForEach-Object { clang-format --dry-run -Werror $_.FullName }" } else { "find src include tests benchmarks examples -name '*.cpp' -o -name '*.hpp' -o -name '*.h' | xargs clang-format --dry-run -Werror" } }}

# Run clang-tidy static analysis
lint:
    @echo "Running clang-tidy..."
    {{ if os() == "windows" { "Get-ChildItem -Recurse -Include *.cpp src,tests,examples | ForEach-Object { clang-tidy -p {{build_dir_debug}} $_.FullName }" } else { "find src tests examples -name '*.cpp' | xargs clang-tidy -p {{build_dir_debug}}" } }}

# Run clang-tidy with automatic fixes
lint-fix:
    @echo "Running clang-tidy with fixes..."
    {{ if os() == "windows" { "Get-ChildItem -Recurse -Include *.cpp src,tests,examples | ForEach-Object { clang-tidy -p {{build_dir_debug}} -fix $_.FullName }" } else { "find src tests examples -name '*.cpp' | xargs clang-tidy -p {{build_dir_debug}} -fix" } }}

# Run clang-tidy on source files only
lint-src:
    @echo "Running clang-tidy on source files..."
    {{ if os() == "windows" { "Get-ChildItem -Recurse -Include *.cpp src | ForEach-Object { clang-tidy -p {{build_dir_debug}} $_.FullName }" } else { "find src -name '*.cpp' | xargs clang-tidy -p {{build_dir_debug}}" } }}

# Run clang-tidy on test files only
lint-tests:
    @echo "Running clang-tidy on test files..."
    {{ if os() == "windows" { "Get-ChildItem -Recurse -Include *.cpp tests | ForEach-Object { clang-tidy -p {{build_dir_debug}} $_.FullName }" } else { "find tests -name '*.cpp' | xargs clang-tidy -p {{build_dir_debug}}" } }}

# ============================================================================
# Cleaning
# ============================================================================

# Clean Debug build
clean-debug:
    @echo "Cleaning Debug build..."
    {{ if os() == "windows" { "Remove-Item -Recurse -Force -ErrorAction SilentlyContinue " + build_dir_debug } else { "rm -rf " + build_dir_debug } }}

# Clean Release build
clean-release:
    @echo "Cleaning Release build..."
    {{ if os() == "windows" { "Remove-Item -Recurse -Force -ErrorAction SilentlyContinue " + build_dir_release } else { "rm -rf " + build_dir_release } }}

# Clean RelWithDebInfo build
clean-relwithdebinfo:
    @echo "Cleaning RelWithDebInfo build..."
    {{ if os() == "windows" { "Remove-Item -Recurse -Force -ErrorAction SilentlyContinue " + build_dir_relwithdebinfo } else { "rm -rf " + build_dir_relwithdebinfo } }}

# Clean all builds
clean: clean-debug clean-release clean-relwithdebinfo
    @echo "All build directories cleaned!"

# Clean all builds and CMake cache
clean-all:
    @echo "Cleaning all build artifacts..."
    {{ if os() == "windows" { "Remove-Item -Recurse -Force -ErrorAction SilentlyContinue build,install" } else { "rm -rf build install" } }}
    @echo "Clean complete!"

# ============================================================================
# Installation
# ============================================================================

# Install Debug build
install-debug: build-debug
    @echo "Installing Debug build..."
    cmake --install {{build_dir_debug}} --prefix install/debug

# Install Release build
install-release: build-release
    @echo "Installing Release build..."
    cmake --install {{build_dir_release}} --prefix install/release

# Alias for install-release
install: install-release

# ============================================================================
# Git Helpers
# ============================================================================

# Update to latest main branch
new:
    git checkout main
    git fetch
    git pull origin main

# Show git status
git-status:
    git status

# Show recent git log
git-log:
    git log --oneline --graph --decorate -10

# Show all branches
git-branches:
    git branch -a

# Remove untracked files (dry-run)
git-clean:
    git clean -n -d

# ============================================================================
# Development Workflow
# ============================================================================

# Initial development setup
dev-setup: vcpkg-install configure-debug
    @echo "Development environment setup complete!"
    @echo "Run 'just build' to build the project"

# Format, build, and test (typical development cycle)
dev-build: format build-debug test
    @echo "Development build cycle complete!"

# Run all quality checks
dev-check: format-check lint test
    @echo "All checks passed!"

# Clean rebuild with tests
dev-rebuild: clean build-debug test
    @echo "Rebuild complete!"

# ============================================================================
# CMake Utilities
# ============================================================================

# List all available CMake presets
list-presets:
    @echo "Available configure presets:"
    @cmake --list-presets
    @echo ""
    @echo "Available build presets:"
    @cmake --build --list-presets

# ============================================================================
# Documentation
# ============================================================================

# Generate documentation (future: Doxygen)
docs:
    @echo "Documentation generation not yet implemented"
    @echo "Will use Doxygen in future phase"
