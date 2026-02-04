# Axiom test suite

# Test executable
add_executable(axiom_tests
    core/error_code_test.cpp
    core/result_test.cpp
    core/assert_test.cpp
    core/logger_test.cpp
    core/test_profiler.cpp
    math/test_vec.cpp
    math/test_vec_ops.cpp
    math/mat4_test.cpp
    math/quat_test.cpp
    math/mat4_quat_integration_test.cpp
    math/transform_test.cpp
    math/aabb_test.cpp
    math/utils_test.cpp
    math/random_test.cpp
    memory/allocator_test.cpp
    memory/pool_allocator_test.cpp
    memory/linear_allocator_test.cpp
    memory/stack_allocator_test.cpp
    memory/stl_adapter_test.cpp
    memory/memory_tracker_test.cpp
    gpu/vk_instance_test.cpp
    gpu/vk_memory_test.cpp
    gpu/vk_command_test.cpp
    gpu/vk_sync_test.cpp
    gpu/vk_swapchain_test.cpp
    gpu/vk_shader_test.cpp
    frontend/window_test.cpp
)

# Link libraries
target_link_libraries(axiom_tests
    PRIVATE
        axiom::math
        axiom::core
        axiom::memory
        axiom::gpu
        axiom::frontend
        GTest::gtest
        GTest::gtest_main
)

# Add test to CTest
add_test(NAME axiom_tests COMMAND axiom_tests)

# Set test properties
set_target_properties(axiom_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

message(STATUS "Axiom tests configured")
