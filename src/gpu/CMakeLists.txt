# Axiom GPU module - Vulkan compute infrastructure

# Source files
set(GPU_SOURCES
    vk_instance.cpp
    vk_memory.cpp
    vk_command.cpp
    vk_sync.cpp
    vk_swapchain.cpp
    vk_shader.cpp
    vk_descriptor.cpp
    vk_compute_pipeline.cpp
    vk_graphics_pipeline.cpp
    render_pass.cpp
    gpu_buffer.cpp
)

# Header files (for IDE integration)
set(GPU_HEADERS
    ${PROJECT_SOURCE_DIR}/include/axiom/gpu/vk_instance.hpp
    ${PROJECT_SOURCE_DIR}/include/axiom/gpu/vk_memory.hpp
    ${PROJECT_SOURCE_DIR}/include/axiom/gpu/vk_command.hpp
    ${PROJECT_SOURCE_DIR}/include/axiom/gpu/vk_sync.hpp
    ${PROJECT_SOURCE_DIR}/include/axiom/gpu/vk_swapchain.hpp
    ${PROJECT_SOURCE_DIR}/include/axiom/gpu/vk_shader.hpp
    ${PROJECT_SOURCE_DIR}/include/axiom/gpu/vk_descriptor.hpp
    ${PROJECT_SOURCE_DIR}/include/axiom/gpu/vk_compute_pipeline.hpp
    ${PROJECT_SOURCE_DIR}/include/axiom/gpu/vk_graphics_pipeline.hpp
    ${PROJECT_SOURCE_DIR}/include/axiom/gpu/render_pass.hpp
    ${PROJECT_SOURCE_DIR}/include/axiom/gpu/gpu_buffer.hpp
)

# Create library target
add_library(axiom_gpu ${GPU_SOURCES} ${GPU_HEADERS})

# Set target properties
set_target_properties(axiom_gpu PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# Include directories
target_include_directories(axiom_gpu
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(axiom_gpu
    PUBLIC
        axiom_core
        Vulkan::Vulkan
    PRIVATE
        GPUOpen::VulkanMemoryAllocator
)

# Add library alias
add_library(axiom::gpu ALIAS axiom_gpu)

# Installation rules
install(TARGETS axiom_gpu
    EXPORT axiomTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

message(STATUS "  axiom_gpu configured")
