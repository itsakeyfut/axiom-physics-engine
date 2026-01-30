#include "axiom/core/error_code.hpp"

#include <gtest/gtest.h>

using namespace axiom::core;

// Test error code to string conversion
TEST(ErrorCodeTest, ErrorCodeToString_Success) {
    EXPECT_STREQ(errorCodeToString(ErrorCode::Success), "Success");
}

TEST(ErrorCodeTest, ErrorCodeToString_MathErrors) {
    EXPECT_STREQ(errorCodeToString(ErrorCode::DivisionByZero), "Division by zero");
    EXPECT_STREQ(errorCodeToString(ErrorCode::NormalizationOfZeroVector),
                 "Cannot normalize zero vector");
    EXPECT_STREQ(errorCodeToString(ErrorCode::InvalidMatrixInversion),
                 "Matrix is singular and cannot be inverted");
    EXPECT_STREQ(errorCodeToString(ErrorCode::InvalidQuaternion),
                 "Quaternion is invalid (not normalized or contains NaN)");
}

TEST(ErrorCodeTest, ErrorCodeToString_MemoryErrors) {
    EXPECT_STREQ(errorCodeToString(ErrorCode::OutOfMemory), "Out of memory");
    EXPECT_STREQ(errorCodeToString(ErrorCode::InvalidAllocation), "Invalid allocation parameters");
    EXPECT_STREQ(errorCodeToString(ErrorCode::NullPointer), "Unexpected null pointer");
}

TEST(ErrorCodeTest, ErrorCodeToString_CollisionErrors) {
    EXPECT_STREQ(errorCodeToString(ErrorCode::InvalidShape), "Invalid collision shape");
    EXPECT_STREQ(errorCodeToString(ErrorCode::GJKFailedToConverge),
                 "GJK algorithm failed to converge");
    EXPECT_STREQ(errorCodeToString(ErrorCode::EPAFailedToConverge),
                 "EPA algorithm failed to converge");
}

TEST(ErrorCodeTest, ErrorCodeToString_DynamicsErrors) {
    EXPECT_STREQ(errorCodeToString(ErrorCode::InvalidRigidBody), "Invalid rigid body properties");
    EXPECT_STREQ(errorCodeToString(ErrorCode::ConstraintSolverFailure),
                 "Constraint solver failed to converge");
}

TEST(ErrorCodeTest, ErrorCodeToString_GPUErrors) {
    EXPECT_STREQ(errorCodeToString(ErrorCode::VulkanInitializationFailed),
                 "Vulkan initialization failed");
    EXPECT_STREQ(errorCodeToString(ErrorCode::ShaderCompilationFailed),
                 "Shader compilation failed");
    EXPECT_STREQ(errorCodeToString(ErrorCode::BufferAllocationFailed),
                 "GPU buffer allocation failed");
}

TEST(ErrorCodeTest, ErrorCodeToString_ValidationErrors) {
    EXPECT_STREQ(errorCodeToString(ErrorCode::InvalidParameter), "Invalid parameter");
    EXPECT_STREQ(errorCodeToString(ErrorCode::OutOfRange), "Value out of range");
}

// Test error category detection
TEST(ErrorCodeTest, GetErrorCategory_Success) {
    EXPECT_EQ(getErrorCategory(ErrorCode::Success), ErrorCategory::None);
}

TEST(ErrorCodeTest, GetErrorCategory_MathErrors) {
    EXPECT_EQ(getErrorCategory(ErrorCode::DivisionByZero), ErrorCategory::Math);
    EXPECT_EQ(getErrorCategory(ErrorCode::NormalizationOfZeroVector), ErrorCategory::Math);
    EXPECT_EQ(getErrorCategory(ErrorCode::InvalidMatrixInversion), ErrorCategory::Math);
    EXPECT_EQ(getErrorCategory(ErrorCode::InvalidQuaternion), ErrorCategory::Math);
}

TEST(ErrorCodeTest, GetErrorCategory_MemoryErrors) {
    EXPECT_EQ(getErrorCategory(ErrorCode::OutOfMemory), ErrorCategory::Memory);
    EXPECT_EQ(getErrorCategory(ErrorCode::InvalidAllocation), ErrorCategory::Memory);
    EXPECT_EQ(getErrorCategory(ErrorCode::NullPointer), ErrorCategory::Memory);
}

TEST(ErrorCodeTest, GetErrorCategory_CollisionErrors) {
    EXPECT_EQ(getErrorCategory(ErrorCode::InvalidShape), ErrorCategory::Collision);
    EXPECT_EQ(getErrorCategory(ErrorCode::GJKFailedToConverge), ErrorCategory::Collision);
    EXPECT_EQ(getErrorCategory(ErrorCode::EPAFailedToConverge), ErrorCategory::Collision);
}

TEST(ErrorCodeTest, GetErrorCategory_DynamicsErrors) {
    EXPECT_EQ(getErrorCategory(ErrorCode::InvalidRigidBody), ErrorCategory::Dynamics);
    EXPECT_EQ(getErrorCategory(ErrorCode::ConstraintSolverFailure), ErrorCategory::Dynamics);
}

TEST(ErrorCodeTest, GetErrorCategory_GPUErrors) {
    EXPECT_EQ(getErrorCategory(ErrorCode::VulkanInitializationFailed), ErrorCategory::GPU);
    EXPECT_EQ(getErrorCategory(ErrorCode::ShaderCompilationFailed), ErrorCategory::GPU);
    EXPECT_EQ(getErrorCategory(ErrorCode::BufferAllocationFailed), ErrorCategory::GPU);
}

TEST(ErrorCodeTest, GetErrorCategory_ValidationErrors) {
    EXPECT_EQ(getErrorCategory(ErrorCode::InvalidParameter), ErrorCategory::Validation);
    EXPECT_EQ(getErrorCategory(ErrorCode::OutOfRange), ErrorCategory::Validation);
}

// Test error category to string conversion
TEST(ErrorCodeTest, ErrorCategoryToString_AllCategories) {
    EXPECT_STREQ(errorCategoryToString(ErrorCategory::None), "None");
    EXPECT_STREQ(errorCategoryToString(ErrorCategory::Math), "Math");
    EXPECT_STREQ(errorCategoryToString(ErrorCategory::Memory), "Memory");
    EXPECT_STREQ(errorCategoryToString(ErrorCategory::Collision), "Collision");
    EXPECT_STREQ(errorCategoryToString(ErrorCategory::Dynamics), "Dynamics");
    EXPECT_STREQ(errorCategoryToString(ErrorCategory::SoftBody), "SoftBody");
    EXPECT_STREQ(errorCategoryToString(ErrorCategory::Fluid), "Fluid");
    EXPECT_STREQ(errorCategoryToString(ErrorCategory::GPU), "GPU");
    EXPECT_STREQ(errorCategoryToString(ErrorCategory::IO), "IO");
    EXPECT_STREQ(errorCategoryToString(ErrorCategory::Validation), "Validation");
}

// Test that error code strings are non-null
TEST(ErrorCodeTest, ErrorCodeStrings_NonNull) {
    EXPECT_NE(errorCodeToString(ErrorCode::Success), nullptr);
    EXPECT_NE(errorCodeToString(ErrorCode::DivisionByZero), nullptr);
    EXPECT_NE(errorCodeToString(ErrorCode::OutOfMemory), nullptr);
    EXPECT_NE(errorCodeToString(ErrorCode::InvalidShape), nullptr);
}

// Test that category strings are non-null
TEST(ErrorCodeTest, ErrorCategoryStrings_NonNull) {
    EXPECT_NE(errorCategoryToString(ErrorCategory::None), nullptr);
    EXPECT_NE(errorCategoryToString(ErrorCategory::Math), nullptr);
    EXPECT_NE(errorCategoryToString(ErrorCategory::Memory), nullptr);
}
