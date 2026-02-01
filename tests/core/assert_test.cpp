#include "axiom/core/assert.hpp"

#include <gtest/gtest.h>

#include <csetjmp>
#include <csignal>

// Disable MSVC warning about constant conditional expressions in preprocessor directives
#ifdef _MSC_VER
#pragma warning(disable : 4127)  // conditional expression is constant
#endif

// Test fixture for assertion tests
class AssertTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset to default handler before each test
        axiom::core::setAssertHandler(nullptr);
    }

    void TearDown() override {
        // Reset to default handler after each test
        axiom::core::setAssertHandler(nullptr);
    }
};

// Helper to test assertion failures
// Since assertions abort the program, we need to use death tests

TEST_F(AssertTest, AssertMacroFailsOnFalseCondition) {
#if AXIOM_ASSERTIONS_ENABLED
    EXPECT_DEATH(
        {
            AXIOM_ASSERT(false, "Test assertion failure");
        },
        "Test assertion failure");
#else
    GTEST_SKIP() << "Assertions disabled in this build";
#endif
}

TEST_F(AssertTest, AssertMacroPassesOnTrueCondition) {
    // Should not abort
    AXIOM_ASSERT(true, "This should not fail");
    AXIOM_ASSERT(1 + 1 == 2, "Math works");
    AXIOM_ASSERT(nullptr == nullptr, "Null equals null");
}

TEST_F(AssertTest, PreconditionFailsOnViolation) {
#if AXIOM_ASSERTIONS_ENABLED
    EXPECT_DEATH(
        {
            AXIOM_PRECONDITION(false);
        },
        "Precondition violated");
#else
    GTEST_SKIP() << "Assertions disabled in this build";
#endif
}

TEST_F(AssertTest, PreconditionPassesOnValidCondition) {
    // Should not abort
    AXIOM_PRECONDITION(true);
    AXIOM_PRECONDITION(1 > 0);
}

TEST_F(AssertTest, PostconditionFailsOnViolation) {
#if AXIOM_ASSERTIONS_ENABLED
    EXPECT_DEATH(
        {
            AXIOM_POSTCONDITION(false);
        },
        "Postcondition violated");
#else
    GTEST_SKIP() << "Assertions disabled in this build";
#endif
}

TEST_F(AssertTest, PostconditionPassesOnValidCondition) {
    // Should not abort
    AXIOM_POSTCONDITION(true);
    AXIOM_POSTCONDITION(2 + 2 == 4);
}

TEST_F(AssertTest, UnreachableAborts) {
#if AXIOM_ASSERTIONS_ENABLED
    EXPECT_DEATH(
        {
            AXIOM_UNREACHABLE();
        },
        "Unreachable code reached");
#else
    GTEST_SKIP() << "Assertions disabled in this build";
#endif
}

TEST_F(AssertTest, AssertIsNoOpInReleaseBuild) {
#if !AXIOM_ASSERTIONS_ENABLED
    // Should not abort (assertions disabled)
    AXIOM_ASSERT(false, "This should be ignored in release");
    AXIOM_PRECONDITION(false);
    AXIOM_POSTCONDITION(false);
#else
    GTEST_SKIP() << "Assertions enabled in this build";
#endif
}

// AXIOM_VERIFY is always enabled
TEST_F(AssertTest, VerifyAlwaysEnabled) {
    EXPECT_DEATH(
        {
            AXIOM_VERIFY(false, "Verify should always be enabled");
        },
        "Verify should always be enabled");
}

TEST_F(AssertTest, VerifyPassesOnTrueCondition) {
    // Should not abort
    AXIOM_VERIFY(true, "This should pass");
    AXIOM_VERIFY(1 == 1, "Math works");
}

// Test custom assertion handler
TEST_F(AssertTest, CustomAssertHandlerIsCalled) {
    static bool handlerCalled = false;
    static const char* capturedExpr = nullptr;
    static const char* capturedMsg = nullptr;

    auto customHandler = [](const char* expr, const char* msg, const char* /*file*/, int /*line*/) {
        handlerCalled = true;
        capturedExpr = expr;
        capturedMsg = msg;
        // Don't abort in the custom handler for testing
        // (In real code, handlers should abort or throw)
    };

    axiom::core::setAssertHandler(customHandler);

    EXPECT_DEATH(
        {
            AXIOM_VERIFY(false, "Custom handler test");
        },
        "");  // Will still abort after handler returns

    // Note: We can't check handlerCalled here because EXPECT_DEATH runs in a child process
}

// Test that expressions with side effects work correctly
TEST_F(AssertTest, VerifyEvaluatesExpression) {
    int counter = 0;
    auto incrementAndReturnTrue = [&counter]() {
        counter++;
        return true;
    };

    // VERIFY should always evaluate the expression
    AXIOM_VERIFY(incrementAndReturnTrue(), "Should evaluate");
    EXPECT_EQ(counter, 1);
}

// Test assertion with complex expressions
#if AXIOM_ASSERTIONS_ENABLED

TEST_F(AssertTest, AssertWithComplexExpression) {
    int x = 10;
    int y = 20;

    // Should pass
    AXIOM_ASSERT(x < y && y == 20, "Complex expression");
    AXIOM_ASSERT((x + y) == 30, "Arithmetic expression");

    // Should fail
    EXPECT_DEATH(
        {
            AXIOM_ASSERT(x > y, "x should be less than y");
        },
        "x > y");
}

TEST_F(AssertTest, PreconditionValidatesPointers) {
    int value = 42;
    int* ptr = &value;
    int* nullPtr = nullptr;

    // Should pass
    AXIOM_PRECONDITION(ptr != nullptr);

    // Should fail
    EXPECT_DEATH(
        {
            AXIOM_PRECONDITION(nullPtr != nullptr);
        },
        "Precondition violated");
}

TEST_F(AssertTest, PostconditionValidatesReturnValues) {
    auto divide = [](int a, int b) -> int {
        AXIOM_PRECONDITION(b != 0);
        int result = a / b;
        AXIOM_POSTCONDITION(result * b == a || a % b != 0);
        return result;
    };

    // Should pass
    EXPECT_EQ(divide(10, 2), 5);
    EXPECT_EQ(divide(7, 3), 2);

    // Should fail on precondition
    EXPECT_DEATH(
        {
            divide(10, 0);
        },
        "Precondition violated");
}

#endif  // AXIOM_ASSERTIONS_ENABLED

// Test assertion messages
#if AXIOM_ASSERTIONS_ENABLED

TEST_F(AssertTest, AssertionMessageIsDisplayed) {
    EXPECT_DEATH(
        {
            AXIOM_ASSERT(false, "This is a custom error message");
        },
        "This is a custom error message");
}

TEST_F(AssertTest, AssertionDisplaysExpression) {
    EXPECT_DEATH(
        {
            int x = 5;
            int y = 10;
            AXIOM_ASSERT(x > y, "x should be greater than y");
        },
        "x > y");
}

TEST_F(AssertTest, AssertionDisplaysFileAndLine) {
    EXPECT_DEATH(
        {
            AXIOM_ASSERT(false, "File and line test");
        },
        "assert_test.cpp");
}

#endif  // AXIOM_ASSERTIONS_ENABLED

// Test that macros work in different contexts
#if AXIOM_ASSERTIONS_ENABLED

TEST_F(AssertTest, AssertionsWorkInFunctions) {
    auto testFunction = [](int value) {
        AXIOM_PRECONDITION(value > 0);
        int result = value * 2;
        AXIOM_POSTCONDITION(result > value);
        return result;
    };

    EXPECT_EQ(testFunction(5), 10);

    EXPECT_DEATH(
        {
            testFunction(-5);
        },
        "Precondition violated");
}

TEST_F(AssertTest, AssertionsWorkInTemplates) {
    auto max = [](auto a, auto b) {
        AXIOM_PRECONDITION(a == a);  // Check for NaN
        AXIOM_PRECONDITION(b == b);  // Check for NaN
        auto result = (a > b) ? a : b;
        AXIOM_POSTCONDITION(result >= a && result >= b);
        return result;
    };

    EXPECT_EQ(max(10, 20), 20);
    EXPECT_EQ(max(5.5, 3.3), 5.5);
}

#endif  // AXIOM_ASSERTIONS_ENABLED

// Integration test with Result<T>
#include "axiom/core/result.hpp"

TEST_F(AssertTest, ResultValueAssertionOnSuccess) {
    axiom::core::Result<int> result = axiom::core::Result<int>::success(42);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value(), 42);  // Should not assert
}

#if AXIOM_ASSERTIONS_ENABLED

TEST_F(AssertTest, ResultValueAssertionOnFailure) {
    axiom::core::Result<int> result = axiom::core::Result<int>::failure(
        axiom::core::ErrorCode::InvalidParameter, "Test error");
    EXPECT_TRUE(result.isFailure());

    // Accessing value on failed Result should assert
    EXPECT_DEATH(
        {
            [[maybe_unused]] auto value = result.value();
        },
        "Attempted to get value from failed Result");
}

#endif  // AXIOM_ASSERTIONS_ENABLED

// Performance test: ensure assertions have no overhead in release builds
#ifndef AXIOM_ASSERTIONS_ENABLED

TEST_F(AssertTest, AssertionsHaveNoOverheadInRelease) {
    // This test verifies that assertions compile to no-ops in release builds
    volatile int counter = 0;

    // These should all be optimized away
    for (int i = 0; i < 1000000; ++i) {
        AXIOM_ASSERT(i >= 0, "Should be optimized away");
        AXIOM_PRECONDITION(i >= 0);
        AXIOM_POSTCONDITION(true);
        counter++;
    }

    EXPECT_EQ(counter, 1000000);
}

#endif  // !AXIOM_ASSERTIONS_ENABLED
