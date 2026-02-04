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

# Vulkan test - demonstrates Vulkan context creation
add_executable(vk_test
    vk_test.cpp
)

target_link_libraries(vk_test
    PRIVATE
        axiom::gpu
        axiom::core
)

set_target_properties(vk_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Command buffer example - demonstrates command buffer management
add_executable(command_buffer_example
    command_buffer_example.cpp
)

target_link_libraries(command_buffer_example
    PRIVATE
        axiom::gpu
        axiom::core
)

set_target_properties(command_buffer_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

message(STATUS "Examples configured: profiling_example, vk_test, command_buffer_example")
