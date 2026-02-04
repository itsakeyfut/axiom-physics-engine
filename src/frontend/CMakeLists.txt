# Axiom Frontend module - Window management and UI integration

# Source files
set(FRONTEND_SOURCES
    window.cpp
)

# Header files (for IDE integration)
set(FRONTEND_HEADERS
    ${PROJECT_SOURCE_DIR}/include/axiom/frontend/window.hpp
)

# Create library target
add_library(axiom_frontend ${FRONTEND_SOURCES} ${FRONTEND_HEADERS})

# Set target properties
set_target_properties(axiom_frontend PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# Include directories
target_include_directories(axiom_frontend
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(axiom_frontend
    PUBLIC
        axiom_core
        axiom_gpu
        glfw
        Vulkan::Vulkan
)

# Add library alias
add_library(axiom::frontend ALIAS axiom_frontend)

# Installation rules
install(TARGETS axiom_frontend
    EXPORT axiomTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

message(STATUS "  axiom_frontend configured")
