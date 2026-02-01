# Axiom Core Module
# Provides fundamental utilities, logging, error handling, and profiling

# Source files
set(AXIOM_CORE_SOURCES
    version.cpp
    error_code.cpp
    assert.cpp
    threading/job_system.cpp
)

# Header files (for IDE organization)
set(AXIOM_CORE_HEADERS
    ${CMAKE_SOURCE_DIR}/include/axiom/core/version.hpp
    ${CMAKE_SOURCE_DIR}/include/axiom/core/error_code.hpp
    ${CMAKE_SOURCE_DIR}/include/axiom/core/result.hpp
    ${CMAKE_SOURCE_DIR}/include/axiom/core/assert.hpp
    ${CMAKE_SOURCE_DIR}/include/axiom/core/profiler.hpp
    ${CMAKE_SOURCE_DIR}/include/axiom/core/threading/work_stealing_queue.hpp
    ${CMAKE_SOURCE_DIR}/include/axiom/core/threading/job_system.hpp
)

# Create library target
add_library(axiom_core ${AXIOM_CORE_SOURCES} ${AXIOM_CORE_HEADERS})

# Add alias for consistent naming
add_library(axiom::core ALIAS axiom_core)

# Target properties
set_target_properties(axiom_core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "axiom_core"
    EXPORT_NAME "core"
)

# Include directories
target_include_directories(axiom_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Link dependencies
target_link_libraries(axiom_core
    PUBLIC
        # Future: Add spdlog for logging
    PRIVATE
        # Internal dependencies
)

# Conditionally link Tracy profiler
if(AXIOM_ENABLE_PROFILING)
    target_link_libraries(axiom_core PUBLIC Tracy::TracyClient)
endif()

# Compile features
target_compile_features(axiom_core PUBLIC cxx_std_20)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(axiom_core PRIVATE AXIOM_CORE_EXPORTS)
endif()

# Installation
install(TARGETS axiom_core
    EXPORT axiomTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/axiom/core
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/axiom
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)
