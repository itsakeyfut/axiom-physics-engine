# Axiom test suite
# Tests will be added as modules are implemented

message(STATUS "Test directory configured (tests will be added in future phases)")

# Example of how to add tests in the future:
# add_executable(axiom_tests
#     math/test_vec3.cpp
#     math/test_mat4.cpp
#     # ...
# )
# target_link_libraries(axiom_tests
#     PRIVATE
#         axiom::core
#         axiom::math
#         GTest::gtest
#         GTest::gtest_main
# )
# add_test(NAME axiom_tests COMMAND axiom_tests)
