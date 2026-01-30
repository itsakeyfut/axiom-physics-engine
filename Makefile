# Axiom Physics Engine - Makefile
# Convenient shortcuts for common development tasks

.PHONY: help configure build test clean install format lint benchmark examples all

# Default target
.DEFAULT_GOAL := help

# Detect OS
ifeq ($(OS),Windows_NT)
    PLATFORM := windows
    PRESET_DEBUG := windows-debug
    PRESET_RELEASE := windows-release
    PRESET_RELWITHDEBINFO := windows-relwithdebinfo
    RM := del /Q /F
    RMDIR := rmdir /S /Q
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        PLATFORM := linux
        PRESET_DEBUG := linux-debug
        PRESET_RELEASE := linux-release
        PRESET_RELWITHDEBINFO := linux-relwithdebinfo
    endif
    ifeq ($(UNAME_S),Darwin)
        PLATFORM := macos
        PRESET_DEBUG := macos-debug
        PRESET_RELEASE := macos-release
        PRESET_RELWITHDEBINFO := macos-relwithdebinfo
    endif
    RM := rm -f
    RMDIR := rm -rf
endif

# Build directories
BUILD_DIR_DEBUG := build/$(PRESET_DEBUG)
BUILD_DIR_RELEASE := build/$(PRESET_RELEASE)
BUILD_DIR_RELWITHDEBINFO := build/$(PRESET_RELWITHDEBINFO)

##@ General

help: ## Display this help message
	@echo "Axiom Physics Engine - Development Commands"
	@echo ""
	@echo "Platform: $(PLATFORM)"
	@echo "Presets: $(PRESET_DEBUG), $(PRESET_RELEASE), $(PRESET_RELWITHDEBINFO)"
	@echo ""
	@awk 'BEGIN {FS = ":.*##"; printf "Usage:\n  make \033[36m<target>\033[0m\n"} /^[a-zA-Z_-]+:.*?##/ { printf "  \033[36m%-20s\033[0m %s\n", $$1, $$2 } /^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) } ' $(MAKEFILE_LIST)

##@ Dependencies

vcpkg-install: ## Install vcpkg dependencies
	@echo "Installing dependencies via vcpkg..."
	@vcpkg install

vcpkg-update: ## Update vcpkg and dependencies
	@echo "Updating vcpkg..."
	@git -C $(VCPKG_ROOT) pull
	@vcpkg upgrade --no-dry-run

##@ Build (Debug)

configure-debug: ## Configure Debug build
	@echo "Configuring Debug build..."
	@cmake --preset $(PRESET_DEBUG)

build-debug: configure-debug ## Build Debug configuration
	@echo "Building Debug..."
	@cmake --build $(BUILD_DIR_DEBUG) --parallel

rebuild-debug: clean-debug build-debug ## Clean and rebuild Debug

configure-debug-ninja: ## Configure Debug build with Ninja (for clang-tidy)
	@echo "Configuring Debug build with Ninja..."
	@cmd.exe /c .\configure-ninja.bat

build-debug-ninja: configure-debug-ninja ## Build Debug with Ninja
	@echo "Building Debug with Ninja..."
	@cmd.exe /c .\build-ninja.bat

##@ Build (Release)

configure-release: ## Configure Release build
	@echo "Configuring Release build..."
	@cmake --preset $(PRESET_RELEASE)

build-release: configure-release ## Build Release configuration
	@echo "Building Release..."
	@cmake --build $(BUILD_DIR_RELEASE) --parallel

rebuild-release: clean-release build-release ## Clean and rebuild Release

##@ Build (RelWithDebInfo)

configure-relwithdebinfo: ## Configure RelWithDebInfo build
	@echo "Configuring RelWithDebInfo build..."
	@cmake --preset $(PRESET_RELWITHDEBINFO)

build-relwithdebinfo: configure-relwithdebinfo ## Build RelWithDebInfo configuration
	@echo "Building RelWithDebInfo..."
	@cmake --build $(BUILD_DIR_RELWITHDEBINFO) --parallel

rebuild-relwithdebinfo: clean-relwithdebinfo build-relwithdebinfo ## Clean and rebuild RelWithDebInfo

##@ Build (Shortcuts)

build: build-debug ## Alias for build-debug (default build)

release: build-release ## Alias for build-release

all: build-debug build-release ## Build both Debug and Release

##@ Testing

test: build-debug ## Run all tests (Debug)
	@echo "Running tests..."
	@ctest --preset $(PRESET_DEBUG) --output-on-failure

test-verbose: build-debug ## Run tests with verbose output
	@echo "Running tests (verbose)..."
	@ctest --preset $(PRESET_DEBUG) --verbose

test-release: build-release ## Run tests (Release)
	@echo "Running tests (Release)..."
	@cd $(BUILD_DIR_RELEASE) && ctest --output-on-failure

##@ Benchmarks

benchmark: build-release ## Run benchmarks
	@echo "Running benchmarks..."
	@$(BUILD_DIR_RELEASE)/bin/axiom_benchmark

benchmark-filter: build-release ## Run specific benchmark (usage: make benchmark-filter FILTER=Vec3)
	@echo "Running filtered benchmarks: $(FILTER)"
	@$(BUILD_DIR_RELEASE)/bin/axiom_benchmark --benchmark_filter=$(FILTER)

##@ Examples

examples: build-debug ## Build examples
	@echo "Examples built in $(BUILD_DIR_DEBUG)/bin/"

run-example: build-debug ## Run specific example (usage: make run-example EXAMPLE=simple_simulation)
	@echo "Running example: $(EXAMPLE)"
	@$(BUILD_DIR_DEBUG)/bin/$(EXAMPLE)

##@ Code Quality

format: ## Format all source files with clang-format
	@echo "Formatting source files..."
	@find src include tests benchmarks examples -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | xargs clang-format -i
	@echo "Format complete!"

format-check: ## Check if code is properly formatted
	@echo "Checking code format..."
	@find src include tests benchmarks examples -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | xargs clang-format --dry-run -Werror

lint: configure-debug-ninja ## Run clang-tidy static analysis
	@echo "Running clang-tidy..."
	@find src tests examples -name "*.cpp" | xargs clang-tidy -p build/windows-debug-ninja

lint-fix: configure-debug-ninja ## Run clang-tidy with automatic fixes
	@echo "Running clang-tidy with fixes..."
	@find src tests examples -name "*.cpp" | xargs clang-tidy -p build/windows-debug-ninja -fix

lint-src: ## Run clang-tidy on source files only
	@echo "Running clang-tidy on source files..."
	@find src -name "*.cpp" | xargs clang-tidy -p $(BUILD_DIR_DEBUG)

lint-tests: ## Run clang-tidy on test files only
	@echo "Running clang-tidy on test files..."
	@find tests -name "*.cpp" | xargs clang-tidy -p $(BUILD_DIR_DEBUG)

##@ Cleaning

clean-debug: ## Clean Debug build
	@echo "Cleaning Debug build..."
	@$(RMDIR) $(BUILD_DIR_DEBUG) 2>/dev/null || true

clean-release: ## Clean Release build
	@echo "Cleaning Release build..."
	@$(RMDIR) $(BUILD_DIR_RELEASE) 2>/dev/null || true

clean-relwithdebinfo: ## Clean RelWithDebInfo build
	@echo "Cleaning RelWithDebInfo build..."
	@$(RMDIR) $(BUILD_DIR_RELWITHDEBINFO) 2>/dev/null || true

clean: clean-debug clean-release clean-relwithdebinfo ## Clean all builds
	@echo "All build directories cleaned!"

clean-all: clean ## Clean all builds and CMake cache
	@echo "Cleaning all build artifacts..."
	@$(RMDIR) build 2>/dev/null || true
	@$(RMDIR) install 2>/dev/null || true
	@echo "Clean complete!"

##@ Installation

install-debug: build-debug ## Install Debug build
	@echo "Installing Debug build..."
	@cmake --install $(BUILD_DIR_DEBUG) --prefix install/debug

install-release: build-release ## Install Release build
	@echo "Installing Release build..."
	@cmake --install $(BUILD_DIR_RELEASE) --prefix install/release

install: install-release ## Alias for install-release

##@ Documentation

docs: ## Generate documentation (future: Doxygen)
	@echo "Documentation generation not yet implemented"
	@echo "Will use Doxygen in future phase"

##@ Git Helpers

git-status: ## Show git status
	@git status

git-log: ## Show recent git log
	@git log --oneline --graph --decorate -10

git-branches: ## Show all branches
	@git branch -a

git-clean: ## Remove untracked files (dry-run)
	@git clean -n -d

##@ Development Workflow

dev-setup: vcpkg-install configure-debug ## Initial development setup
	@echo "Development environment setup complete!"
	@echo "Run 'make build' to build the project"

dev-build: format build-debug test ## Format, build, and test (typical development cycle)
	@echo "Development build cycle complete!"

dev-check: format-check lint test ## Run all quality checks
	@echo "All checks passed!"

dev-rebuild: clean build-debug test ## Clean rebuild with tests
	@echo "Rebuild complete!"

##@ Quick Commands

quick-test: ## Quick build and test (no configure)
	@cmake --build $(BUILD_DIR_DEBUG) --parallel
	@ctest --preset $(PRESET_DEBUG) --output-on-failure

quick-build: ## Quick build without configure
	@cmake --build $(BUILD_DIR_DEBUG) --parallel

watch-test: ## Watch mode: rebuild and test on changes (requires entr)
	@echo "Watching for changes... (Ctrl+C to stop)"
	@find src tests -name "*.cpp" -o -name "*.hpp" | entr -c make quick-test

##@ CMake Presets

list-presets: ## List all available CMake presets
	@echo "Available configure presets:"
	@cmake --list-presets
	@echo ""
	@echo "Available build presets:"
	@cmake --build --list-presets

##@ Information

info: ## Display build information
	@echo "Axiom Physics Engine - Build Information"
	@echo "========================================"
	@echo "Platform:           $(PLATFORM)"
	@echo "Debug Preset:       $(PRESET_DEBUG)"
	@echo "Release Preset:     $(PRESET_RELEASE)"
	@echo "RelWithDebInfo:     $(PRESET_RELWITHDEBINFO)"
	@echo ""
	@echo "Build Directories:"
	@echo "  Debug:            $(BUILD_DIR_DEBUG)"
	@echo "  Release:          $(BUILD_DIR_RELEASE)"
	@echo "  RelWithDebInfo:   $(BUILD_DIR_RELWITHDEBINFO)"
	@echo ""
	@echo "VCPKG_ROOT:         $(VCPKG_ROOT)"
	@echo ""
	@cmake --version 2>/dev/null || echo "CMake not found in PATH"
