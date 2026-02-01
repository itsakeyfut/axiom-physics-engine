# Axiom examples
# Example programs demonstrating library usage

# Profiling example - demonstrates profiling infrastructure
add_executable(profiling_example
    profiling_example.cpp
)

target_link_libraries(profiling_example
    PRIVATE
        axiom::core
)

set_target_properties(profiling_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Job system examples will be added here in the future

message(STATUS "Examples configured: profiling_example")
