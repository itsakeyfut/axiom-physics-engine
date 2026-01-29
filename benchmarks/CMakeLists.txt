# Axiom benchmarks
# Benchmarks will be added as modules are implemented

message(STATUS "Benchmark directory configured (benchmarks will be added in future phases)")

# Example of how to add benchmarks in the future:
# add_executable(axiom_benchmark
#     math/benchmark_vec3.cpp
#     math/benchmark_mat4.cpp
#     # ...
# )
# target_link_libraries(axiom_benchmark
#     PRIVATE
#         axiom::core
#         axiom::math
#         benchmark::benchmark
#         benchmark::benchmark_main
# )
