# Axiom debug module

add_library(axiom_debug STATIC
    debug_draw.cpp
    physics_debug_draw.cpp
)

# Create alias for consistent naming
add_library(axiom::debug ALIAS axiom_debug)

target_include_directories(axiom_debug
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
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

# Installation
install(TARGETS axiom_debug
    EXPORT axiomTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/axiom/debug
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/axiom
    FILES_MATCHING PATTERN "*.hpp"
)

message(STATUS "Axiom debug module configured")
