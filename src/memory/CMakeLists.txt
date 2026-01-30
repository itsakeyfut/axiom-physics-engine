# Axiom Memory Module

# Source files
set(AXIOM_MEMORY_SOURCES
    heap_allocator.cpp
    linear_allocator.cpp
)

# Create memory library
add_library(axiom_memory ${AXIOM_MEMORY_SOURCES})

# Alias for consistent naming
add_library(axiom::memory ALIAS axiom_memory)

# Include directories
target_include_directories(axiom_memory
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(axiom_memory
    PUBLIC
        # No public dependencies for memory module
    PRIVATE
        # No private dependencies
)

# Set target properties
set_target_properties(axiom_memory PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "axiom_memory"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Install targets
install(TARGETS axiom_memory
    EXPORT axiomTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
