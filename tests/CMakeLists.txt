# Axiom test suite

# Find glslc for shader compilation
find_program(GLSLC glslc HINTS "$ENV{VULKAN_SDK}/Bin")
if(NOT GLSLC)
    message(WARNING "glslc not found. Shader compilation will be skipped. Install Vulkan SDK to enable shader tests.")
endif()

# Function to compile GLSL shaders to SPIR-V
function(add_shader_compilation TARGET SHADER_SOURCE)
    if(GLSLC)
        get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME)
        set(SHADER_INPUT ${CMAKE_SOURCE_DIR}/shaders/${SHADER_SOURCE})
        set(SHADER_OUTPUT ${CMAKE_BINARY_DIR}/shaders/${SHADER_SOURCE}.spv)

        # Create output directory
        get_filename_component(SHADER_OUTPUT_DIR ${SHADER_OUTPUT} DIRECTORY)
        file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

        # Add custom command to compile shader
        add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${GLSLC} ${SHADER_INPUT} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER_INPUT}
            COMMENT "Compiling shader ${SHADER_SOURCE}"
            VERBATIM
        )

        # Add shader output to target dependencies
        target_sources(${TARGET} PRIVATE ${SHADER_OUTPUT})

        # Copy to build directory for tests
        add_custom_command(
            TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${SHADER_OUTPUT}
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${SHADER_SOURCE}.spv
            COMMENT "Copying ${SHADER_NAME}.spv to runtime directory"
        )
    endif()
endfunction()

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
    gpu/vk_descriptor_test.cpp
    gpu/vk_compute_pipeline_test.cpp
    gpu/gpu_buffer_test.cpp
    gpu/test_compute.cpp
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

# Compile test shaders
add_shader_compilation(axiom_tests test/array_add.comp)

message(STATUS "Axiom tests configured")
