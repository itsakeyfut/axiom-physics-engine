# Axiom benchmarks

# Pool allocator benchmark
add_executable(pool_allocator_benchmark
    pool_allocator_benchmark.cpp
)

target_link_libraries(pool_allocator_benchmark
    PRIVATE
        axiom_memory
        benchmark::benchmark
)

# Set output directory
set_target_properties(pool_allocator_benchmark
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Linear allocator benchmark
add_executable(linear_allocator_benchmark
    memory/linear_allocator_benchmark.cpp
)

target_link_libraries(linear_allocator_benchmark
    PRIVATE
        axiom_memory
        benchmark::benchmark
)

# Set output directory
set_target_properties(linear_allocator_benchmark
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Stack allocator benchmark
add_executable(stack_allocator_benchmark
    memory/stack_allocator_benchmark.cpp
)

target_link_libraries(stack_allocator_benchmark
    PRIVATE
        axiom_memory
        benchmark::benchmark
)

# Set output directory
set_target_properties(stack_allocator_benchmark
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
