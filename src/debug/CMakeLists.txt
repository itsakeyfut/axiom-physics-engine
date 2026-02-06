# Axiom debug module

add_library(axiom_debug STATIC
    debug_draw.cpp
)

# Create alias for consistent naming
add_library(axiom::debug ALIAS axiom_debug)

target_include_directories(axiom_debug
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(axiom_debug
    PUBLIC
        axiom::core
        axiom::math
        axiom::gpu
        Vulkan::Vulkan
)

# Set C++20 standard
target_compile_features(axiom_debug PUBLIC cxx_std_20)

# Set compiler warnings
if(MSVC)
    target_compile_options(axiom_debug PRIVATE /W4)
else()
    target_compile_options(axiom_debug PRIVATE -Wall -Wextra -Wpedantic)
endif()

message(STATUS "Axiom debug module configured")
