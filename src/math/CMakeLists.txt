# Axiom Math Module
# Provides vector, matrix, quaternion, and transform operations

# Source files
set(AXIOM_MATH_SOURCES
    math_placeholder.cpp
)

# Header files (for IDE organization)
set(AXIOM_MATH_HEADERS
    ${CMAKE_SOURCE_DIR}/include/axiom/math/vec2.hpp
    ${CMAKE_SOURCE_DIR}/include/axiom/math/vec3.hpp
    ${CMAKE_SOURCE_DIR}/include/axiom/math/vec4.hpp
    ${CMAKE_SOURCE_DIR}/include/axiom/math/vec_ops.hpp
)

# Create library target
add_library(axiom_math ${AXIOM_MATH_SOURCES} ${AXIOM_MATH_HEADERS})

# Add alias for consistent naming
add_library(axiom::math ALIAS axiom_math)

# Target properties
set_target_properties(axiom_math PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "axiom_math"
    EXPORT_NAME "math"
)

# Include directories
target_include_directories(axiom_math
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Link dependencies
target_link_libraries(axiom_math
    PUBLIC
        glm::glm
        axiom::core
    PRIVATE
        # Internal dependencies
)

# Compile features
target_compile_features(axiom_math PUBLIC cxx_std_20)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(axiom_math PRIVATE AXIOM_MATH_EXPORTS)
endif()

# Installation
install(TARGETS axiom_math
    EXPORT axiomTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/axiom/math
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/axiom
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)
