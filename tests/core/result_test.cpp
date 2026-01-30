#include "axiom/core/result.hpp"

#include <gtest/gtest.h>

#include <string>

using namespace axiom::core;

// Test Result<T> with int type
TEST(ResultTest, Success_Int) {
    auto result = Result<int>::success(42);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_FALSE(result.isFailure());
    EXPECT_EQ(result.value(), 42);
    EXPECT_EQ(result.errorCode(), ErrorCode::Success);
}

TEST(ResultTest, Failure_Int) {
    auto result = Result<int>::failure(ErrorCode::InvalidParameter, "Test error");
    EXPECT_FALSE(result.isSuccess());
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
    EXPECT_STREQ(result.errorMessage(), "Test error");
}

TEST(ResultTest, ValueOr_SuccessCase) {
    auto result = Result<int>::success(42);
    EXPECT_EQ(result.valueOr(100), 42);
}

TEST(ResultTest, ValueOr_FailureCase) {
    auto result = Result<int>::failure(ErrorCode::OutOfMemory);
    EXPECT_EQ(result.valueOr(100), 100);
}

// Test Result<T> with std::string type
TEST(ResultTest, Success_String) {
    auto result = Result<std::string>::success(std::string("hello"));
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value(), "hello");
}

TEST(ResultTest, Failure_String) {
    auto result = Result<std::string>::failure(ErrorCode::InvalidParameter);
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test copy constructor and assignment
TEST(ResultTest, CopyConstructor_Success) {
    auto result1 = Result<int>::success(42);
    auto result2 = result1;
    EXPECT_TRUE(result2.isSuccess());
    EXPECT_EQ(result2.value(), 42);
}

TEST(ResultTest, CopyConstructor_Failure) {
    auto result1 = Result<int>::failure(ErrorCode::OutOfMemory, "Test");
    auto result2 = result1;
    EXPECT_TRUE(result2.isFailure());
    EXPECT_EQ(result2.errorCode(), ErrorCode::OutOfMemory);
    EXPECT_STREQ(result2.errorMessage(), "Test");
}

TEST(ResultTest, CopyAssignment_Success) {
    auto result1 = Result<int>::success(42);
    auto result2 = Result<int>::failure(ErrorCode::InvalidParameter);
    result2 = result1;
    EXPECT_TRUE(result2.isSuccess());
    EXPECT_EQ(result2.value(), 42);
}

TEST(ResultTest, CopyAssignment_Failure) {
    auto result1 = Result<int>::failure(ErrorCode::OutOfMemory, "Test");
    auto result2 = Result<int>::success(100);
    result2 = result1;
    EXPECT_TRUE(result2.isFailure());
    EXPECT_EQ(result2.errorCode(), ErrorCode::OutOfMemory);
}

// Test move constructor and assignment
TEST(ResultTest, MoveConstructor_Success) {
    auto result1 = Result<std::string>::success(std::string("hello"));
    auto result2 = std::move(result1);
    EXPECT_TRUE(result2.isSuccess());
    EXPECT_EQ(result2.value(), "hello");
}

TEST(ResultTest, MoveConstructor_Failure) {
    auto result1 = Result<std::string>::failure(ErrorCode::OutOfMemory, "Test");
    auto result2 = std::move(result1);
    EXPECT_TRUE(result2.isFailure());
    EXPECT_EQ(result2.errorCode(), ErrorCode::OutOfMemory);
}

TEST(ResultTest, MoveAssignment_Success) {
    auto result1 = Result<std::string>::success(std::string("hello"));
    auto result2 = Result<std::string>::failure(ErrorCode::InvalidParameter);
    result2 = std::move(result1);
    EXPECT_TRUE(result2.isSuccess());
    EXPECT_EQ(result2.value(), "hello");
}

TEST(ResultTest, MoveAssignment_Failure) {
    auto result1 = Result<std::string>::failure(ErrorCode::OutOfMemory, "Test");
    auto result2 = Result<std::string>::success(std::string("world"));
    result2 = std::move(result1);
    EXPECT_TRUE(result2.isFailure());
    EXPECT_EQ(result2.errorCode(), ErrorCode::OutOfMemory);
}

// Test map function
TEST(ResultTest, Map_Success) {
    auto result1 = Result<int>::success(42);
    auto result2 = result1.map<double>([](int x) { return x * 2.0; });
    EXPECT_TRUE(result2.isSuccess());
    EXPECT_DOUBLE_EQ(result2.value(), 84.0);
}

TEST(ResultTest, Map_Failure) {
    auto result1 = Result<int>::failure(ErrorCode::InvalidParameter, "Test");
    auto result2 = result1.map<double>([](int x) { return x * 2.0; });
    EXPECT_TRUE(result2.isFailure());
    EXPECT_EQ(result2.errorCode(), ErrorCode::InvalidParameter);
    EXPECT_STREQ(result2.errorMessage(), "Test");
}

// Test andThen function
TEST(ResultTest, AndThen_Success) {
    auto result1 = Result<int>::success(42);
    auto result2 = result1.andThen<double>([](int x) { return Result<double>::success(x * 2.0); });
    EXPECT_TRUE(result2.isSuccess());
    EXPECT_DOUBLE_EQ(result2.value(), 84.0);
}

TEST(ResultTest, AndThen_FailureInChain) {
    auto result1 = Result<int>::success(42);
    auto result2 = result1.andThen<double>(
        [](int) { return Result<double>::failure(ErrorCode::OutOfRange, "Value too large"); });
    EXPECT_TRUE(result2.isFailure());
    EXPECT_EQ(result2.errorCode(), ErrorCode::OutOfRange);
    EXPECT_STREQ(result2.errorMessage(), "Value too large");
}

TEST(ResultTest, AndThen_FailureFromStart) {
    auto result1 = Result<int>::failure(ErrorCode::InvalidParameter, "Initial error");
    auto result2 = result1.andThen<double>([](int x) { return Result<double>::success(x * 2.0); });
    EXPECT_TRUE(result2.isFailure());
    EXPECT_EQ(result2.errorCode(), ErrorCode::InvalidParameter);
    EXPECT_STREQ(result2.errorMessage(), "Initial error");
}

// Test chaining multiple operations
TEST(ResultTest, ChainedOperations) {
    auto result = Result<int>::success(10)
                      .map<int>([](int x) { return x + 5; })
                      .andThen<int>([](int x) {
                          if (x > 10) {
                              return Result<int>::success(x * 2);
                          } else {
                              return Result<int>::failure(ErrorCode::OutOfRange);
                          }
                      })
                      .map<int>([](int x) { return x + 1; });

    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value(), 31);  // (10 + 5) * 2 + 1 = 31
}

TEST(ResultTest, ChainedOperations_FailureInMiddle) {
    auto result = Result<int>::success(5)
                      .map<int>([](int x) { return x + 2; })
                      .andThen<int>([](int x) {
                          if (x > 10) {
                              return Result<int>::success(x * 2);
                          } else {
                              return Result<int>::failure(ErrorCode::OutOfRange, "Value too small");
                          }
                      })
                      .map<int>([](int x) { return x + 1; });

    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::OutOfRange);
    EXPECT_STREQ(result.errorMessage(), "Value too small");
}

// Test Result<void> specialization
TEST(ResultVoidTest, Success) {
    auto result = Result<void>::success();
    EXPECT_TRUE(result.isSuccess());
    EXPECT_FALSE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::Success);
}

TEST(ResultVoidTest, Failure) {
    auto result = Result<void>::failure(ErrorCode::InvalidParameter, "Test error");
    EXPECT_FALSE(result.isSuccess());
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
    EXPECT_STREQ(result.errorMessage(), "Test error");
}

TEST(ResultVoidTest, FailureWithoutMessage) {
    auto result = Result<void>::failure(ErrorCode::OutOfMemory);
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::OutOfMemory);
    EXPECT_EQ(result.errorMessage(), nullptr);
}

// Test practical usage examples
TEST(ResultTest, PracticalExample_DivisionByZero) {
    auto safeDivide = [](double a, double b) -> Result<double> {
        if (b == 0.0) {
            return Result<double>::failure(ErrorCode::DivisionByZero, "Cannot divide by zero");
        }
        return Result<double>::success(a / b);
    };

    auto result1 = safeDivide(10.0, 2.0);
    EXPECT_TRUE(result1.isSuccess());
    EXPECT_DOUBLE_EQ(result1.value(), 5.0);

    auto result2 = safeDivide(10.0, 0.0);
    EXPECT_TRUE(result2.isFailure());
    EXPECT_EQ(result2.errorCode(), ErrorCode::DivisionByZero);
}

TEST(ResultTest, PracticalExample_VectorNormalization) {
    struct Vec3 {
        double x, y, z;
        double lengthSquared() const { return x * x + y * y + z * z; }
    };

    auto safeNormalize = [](const Vec3& v) -> Result<Vec3> {
        double lenSq = v.lengthSquared();
        if (lenSq < 1e-10) {
            return Result<Vec3>::failure(ErrorCode::NormalizationOfZeroVector,
                                         "Cannot normalize zero vector");
        }
        double len = std::sqrt(lenSq);
        return Result<Vec3>::success(Vec3{v.x / len, v.y / len, v.z / len});
    };

    Vec3 v1{3.0, 4.0, 0.0};
    auto result1 = safeNormalize(v1);
    EXPECT_TRUE(result1.isSuccess());
    EXPECT_NEAR(result1.value().x, 0.6, 1e-6);
    EXPECT_NEAR(result1.value().y, 0.8, 1e-6);

    Vec3 v2{0.0, 0.0, 0.0};
    auto result2 = safeNormalize(v2);
    EXPECT_TRUE(result2.isFailure());
    EXPECT_EQ(result2.errorCode(), ErrorCode::NormalizationOfZeroVector);
}
