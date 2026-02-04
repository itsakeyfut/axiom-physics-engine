# Axiom Physics Engine - Main Application

# Main executable
add_executable(axiom_app
    main.cpp
)

# Set target properties
set_target_properties(axiom_app PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Link libraries
target_link_libraries(axiom_app
    PRIVATE
        axiom::core
        axiom::gpu
        axiom::frontend
)

message(STATUS "Axiom main application configured")
