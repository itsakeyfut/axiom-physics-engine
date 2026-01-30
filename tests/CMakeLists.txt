# Axiom test suite

# Test executable
add_executable(axiom_tests
    math/test_vec.cpp
    math/test_vec_ops.cpp
    math/mat4_test.cpp
    math/quat_test.cpp
    math/mat4_quat_integration_test.cpp
    math/transform_test.cpp
    math/aabb_test.cpp
    math/utils_test.cpp
    math/random_test.cpp
)

# Link libraries
target_link_libraries(axiom_tests
    PRIVATE
        axiom::math
        axiom::core
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
