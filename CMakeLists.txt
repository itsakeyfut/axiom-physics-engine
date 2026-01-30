cmake_minimum_required(VERSION 3.24)

# Project definition
project(axiom
    VERSION 0.1.0
    DESCRIPTION "Next-generation realistic physics engine with C++20 and Vulkan"
    LANGUAGES CXX
)

# C++ standard requirement
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clang-tidy and other tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(AXIOM_BUILD_TESTS "Build test suite" ON)
option(AXIOM_BUILD_BENCHMARKS "Build benchmarks" ON)
option(AXIOM_BUILD_EXAMPLES "Build example programs" ON)
option(AXIOM_ENABLE_PROFILING "Enable Tracy profiler integration" OFF)
option(AXIOM_USE_SIMD "Enable SIMD optimizations" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Compiler warnings
if(MSVC)
    add_compile_options(
        /W4          # Warning level 4
        /WX          # Treat warnings as errors
        /permissive- # Standards conformance mode
        /Zc:__cplusplus # Enable correct __cplusplus macro
    )
    # Disable specific MSVC warnings
    add_compile_options(
        /wd4251 # 'identifier' : class 'type' needs to have dll-interface
    )
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )

    # Additional warnings for GCC/Clang
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(
            -Wcast-align
            -Wconversion
            -Wdouble-promotion
            -Wformat=2
            -Wnon-virtual-dtor
            -Wnull-dereference
            -Wold-style-cast
            -Woverloaded-virtual
            -Wshadow
            -Wsign-conversion
            -Wunused
        )
    endif()
endif()

# Build type configurations
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Configuration-specific settings
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(AXIOM_DEBUG=1)
    add_compile_definitions(AXIOM_ENABLE_MEMORY_TRACKING=1)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(AXIOM_RELEASE=1 NDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_compile_definitions(AXIOM_RELWITHDEBINFO=1)
endif()

# Platform-specific settings
if(WIN32)
    add_compile_definitions(
        AXIOM_PLATFORM_WINDOWS
        NOMINMAX           # Prevent min/max macro conflicts
        WIN32_LEAN_AND_MEAN # Reduce Windows header size
        _CRT_SECURE_NO_WARNINGS
    )
elseif(UNIX AND NOT APPLE)
    add_compile_definitions(AXIOM_PLATFORM_LINUX)
elseif(APPLE)
    add_compile_definitions(AXIOM_PLATFORM_MACOS)
endif()

# SIMD settings
if(AXIOM_USE_SIMD)
    add_compile_definitions(AXIOM_USE_SIMD=1)

    # Detect architecture
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)")
        # ARM64 architecture - use NEON
        message(STATUS "SIMD: Using ARM NEON")
        if(NOT MSVC)
            # GCC/Clang on ARM
            add_compile_options(-march=armv8-a)
        endif()
        # NEON is enabled by default on ARM64, no additional flags needed
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64|x64)")
        # x86_64 architecture - use AVX2/FMA
        message(STATUS "SIMD: Using x86_64 AVX2/FMA")
        if(MSVC)
            # Enable AVX2 on MSVC
            add_compile_options(/arch:AVX2)
        else()
            # Enable AVX2 on GCC/Clang
            add_compile_options(-mavx2 -mfma)
        endif()
    else()
        message(WARNING "SIMD: Unknown architecture ${CMAKE_SYSTEM_PROCESSOR}, SIMD optimizations disabled")
        set(AXIOM_USE_SIMD OFF)
    endif()
endif()

# Tracy profiler integration
if(AXIOM_ENABLE_PROFILING)
    add_compile_definitions(AXIOM_ENABLE_PROFILING=1 TRACY_ENABLE)
endif()

# Find packages
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

if(AXIOM_BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
    enable_testing()
endif()

if(AXIOM_BUILD_BENCHMARKS)
    find_package(benchmark CONFIG REQUIRED)
endif()

if(AXIOM_ENABLE_PROFILING)
    find_package(Tracy CONFIG)
    if(NOT Tracy_FOUND)
        message(WARNING "Tracy not found. Profiling will be disabled.")
        set(AXIOM_ENABLE_PROFILING OFF)
    endif()
endif()

# Subdirectories
add_subdirectory(src)

if(AXIOM_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(AXIOM_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(AXIOM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation rules
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install targets (will be defined in subdirectories)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# Generate package configuration files
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/axiomConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/axiomConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/axiom
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/axiomConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/axiomConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/axiomConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/axiom
)

# Export targets
install(EXPORT axiomTargets
    FILE axiomTargets.cmake
    NAMESPACE axiom::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/axiom
)

# Generate pkg-config files
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/axiom_core.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/axiom_core.pc
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/axiom_math.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/axiom_math.pc
    @ONLY
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/axiom_core.pc
    ${CMAKE_CURRENT_BINARY_DIR}/axiom_math.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# Print configuration summary
message(STATUS "")
message(STATUS "Axiom Physics Engine Configuration:")
message(STATUS "  Version:              ${PROJECT_VERSION}")
message(STATUS "  Build type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard:         C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Install prefix:       ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Build options:")
message(STATUS "  Build tests:          ${AXIOM_BUILD_TESTS}")
message(STATUS "  Build benchmarks:     ${AXIOM_BUILD_BENCHMARKS}")
message(STATUS "  Build examples:       ${AXIOM_BUILD_EXAMPLES}")
message(STATUS "  Enable profiling:     ${AXIOM_ENABLE_PROFILING}")
message(STATUS "  Use SIMD:             ${AXIOM_USE_SIMD}")
message(STATUS "  Build shared libs:    ${BUILD_SHARED_LIBS}")
message(STATUS "")
message(STATUS "Compiler:")
message(STATUS "  ID:                   ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Version:              ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Path:                 ${CMAKE_CXX_COMPILER}")
message(STATUS "")
