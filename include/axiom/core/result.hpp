#pragma once

#include "axiom/core/error_code.hpp"

#include <type_traits>
#include <utility>

namespace axiom::core {

/// Result type for operations that may fail
/// Provides a type-safe way to return either a value or an error without exceptions
/// This is similar to Rust's Result<T, E> or C++23's std::expected
/// @tparam T The type of the success value
template <typename T>
class Result {
public:
    /// Create a successful result with a value
    /// @param value The success value to store
    /// @return A Result containing the success value
    static Result success(T value) {
        Result result;
        result.success_ = true;
        result.errorCode_ = ErrorCode::Success;
        result.errorMessage_ = nullptr;
        new (&result.value_) T(std::move(value));
        return result;
    }

    /// Create a failed result with an error code
    /// @param code The error code indicating the failure
    /// @param message Optional error message (must be a static string or have static lifetime)
    /// @return A Result containing the error
    static Result failure(ErrorCode code, const char* message = nullptr) {
        Result result;
        result.success_ = false;
        result.errorCode_ = code;
        result.errorMessage_ = message;
        return result;
    }

    /// Default constructor creates a failure result
    Result() : success_(false), errorCode_(ErrorCode::Success), errorMessage_(nullptr) {}

    /// Copy constructor
    Result(const Result& other)
        : success_(other.success_),
          errorCode_(other.errorCode_),
          errorMessage_(other.errorMessage_) {
        if (success_ && !std::is_trivially_copyable_v<T>) {
            new (&value_) T(other.value_);
        } else if (success_) {
            value_ = other.value_;
        }
    }

    /// Move constructor
    Result(Result&& other) noexcept
        : success_(other.success_),
          errorCode_(other.errorCode_),
          errorMessage_(other.errorMessage_) {
        if (success_ && !std::is_trivially_move_constructible_v<T>) {
            new (&value_) T(std::move(other.value_));
        } else if (success_) {
            value_ = std::move(other.value_);
        }
    }

    /// Copy assignment
    Result& operator=(const Result& other) {
        if (this != &other) {
            if (success_ && !std::is_trivially_destructible_v<T>) {
                value_.~T();
            }
            success_ = other.success_;
            errorCode_ = other.errorCode_;
            errorMessage_ = other.errorMessage_;
            if (success_ && !std::is_trivially_copyable_v<T>) {
                new (&value_) T(other.value_);
            } else if (success_) {
                value_ = other.value_;
            }
        }
        return *this;
    }

    /// Move assignment
    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            if (success_ && !std::is_trivially_destructible_v<T>) {
                value_.~T();
            }
            success_ = other.success_;
            errorCode_ = other.errorCode_;
            errorMessage_ = other.errorMessage_;
            if (success_ && !std::is_trivially_move_constructible_v<T>) {
                new (&value_) T(std::move(other.value_));
            } else if (success_) {
                value_ = std::move(other.value_);
            }
        }
        return *this;
    }

    /// Destructor
    ~Result() {
        if (success_ && !std::is_trivially_destructible_v<T>) {
            value_.~T();
        }
    }

    /// Check if the operation succeeded
    /// @return true if the result contains a value, false if it contains an error
    bool isSuccess() const noexcept { return success_; }

    /// Check if the operation failed
    /// @return true if the result contains an error, false if it contains a value
    bool isFailure() const noexcept { return !success_; }

    /// Get the value (only valid if isSuccess() == true)
    /// @return Reference to the success value
    /// @pre isSuccess() must be true
    T& value() & {
        // In debug builds, we could add an assertion here
        return value_;
    }

    /// Get the value (only valid if isSuccess() == true)
    /// @return Const reference to the success value
    /// @pre isSuccess() must be true
    const T& value() const& {
        // In debug builds, we could add an assertion here
        return value_;
    }

    /// Get the value by moving (only valid if isSuccess() == true)
    /// @return Moved success value
    /// @pre isSuccess() must be true
    T&& value() && {
        // In debug builds, we could add an assertion here
        return std::move(value_);
    }

    /// Get the error code (only valid if isFailure() == true)
    /// @return The error code
    ErrorCode errorCode() const noexcept { return errorCode_; }

    /// Get the error message (only valid if isFailure() == true)
    /// @return The error message, or nullptr if no message was provided
    const char* errorMessage() const noexcept { return errorMessage_; }

    /// Unwrap the value or return a default value
    /// @param defaultValue The value to return if this Result is a failure
    /// @return The success value if isSuccess(), otherwise defaultValue
    T valueOr(const T& defaultValue) const& { return success_ ? value_ : defaultValue; }

    /// Unwrap the value or return a default value (rvalue version)
    /// @param defaultValue The value to return if this Result is a failure
    /// @return The success value if isSuccess(), otherwise defaultValue
    T valueOr(T&& defaultValue) && {
        return success_ ? std::move(value_) : std::move(defaultValue);
    }

    /// Map the value to another type if successful
    /// If this Result is a failure, returns a Result<U> with the same error
    /// @tparam U The result type of the mapping function
    /// @tparam Func The type of the mapping function (T -> U)
    /// @param func The function to apply to the value
    /// @return Result<U> containing either the mapped value or the error
    template <typename U, typename Func>
    Result<U> map(Func func) const& {
        if (success_) {
            return Result<U>::success(func(value_));
        } else {
            return Result<U>::failure(errorCode_, errorMessage_);
        }
    }

    /// Map the value to another type if successful (rvalue version)
    /// @tparam U The result type of the mapping function
    /// @tparam Func The type of the mapping function (T -> U)
    /// @param func The function to apply to the value
    /// @return Result<U> containing either the mapped value or the error
    template <typename U, typename Func>
    Result<U> map(Func func) && {
        if (success_) {
            return Result<U>::success(func(std::move(value_)));
        } else {
            return Result<U>::failure(errorCode_, errorMessage_);
        }
    }

    /// Chain operations that return Result (monadic bind)
    /// If this Result is a failure, returns a Result<U> with the same error
    /// @tparam U The result type of the chained operation
    /// @tparam Func The type of the chaining function (T -> Result<U>)
    /// @param func The function to apply to the value
    /// @return Result<U> from the chained operation or the error
    template <typename U, typename Func>
    Result<U> andThen(Func func) const& {
        if (success_) {
            return func(value_);
        } else {
            return Result<U>::failure(errorCode_, errorMessage_);
        }
    }

    /// Chain operations that return Result (rvalue version)
    /// @tparam U The result type of the chained operation
    /// @tparam Func The type of the chaining function (T -> Result<U>)
    /// @param func The function to apply to the value
    /// @return Result<U> from the chained operation or the error
    template <typename U, typename Func>
    Result<U> andThen(Func func) && {
        if (success_) {
            return func(std::move(value_));
        } else {
            return Result<U>::failure(errorCode_, errorMessage_);
        }
    }

private:
    bool success_;              ///< Whether the operation succeeded
    ErrorCode errorCode_;       ///< Error code (valid only if !success_)
    const char* errorMessage_;  ///< Optional error message (valid only if !success_)
    union {
        T value_;     ///< Success value (valid only if success_)
        char dummy_;  ///< Dummy byte for empty union when T is void
    };
};

/// Specialization for void (operations with no return value)
template <>
class Result<void> {
public:
    /// Create a successful result
    /// @return A successful Result<void>
    static Result success() {
        Result result;
        result.success_ = true;
        result.errorCode_ = ErrorCode::Success;
        result.errorMessage_ = nullptr;
        return result;
    }

    /// Create a failed result with an error code
    /// @param code The error code indicating the failure
    /// @param message Optional error message (must be a static string or have static lifetime)
    /// @return A failed Result<void>
    static Result failure(ErrorCode code, const char* message = nullptr) {
        Result result;
        result.success_ = false;
        result.errorCode_ = code;
        result.errorMessage_ = message;
        return result;
    }

    /// Default constructor creates a failure result
    Result() : success_(false), errorCode_(ErrorCode::Success), errorMessage_(nullptr) {}

    /// Check if the operation succeeded
    /// @return true if the operation succeeded, false otherwise
    bool isSuccess() const noexcept { return success_; }

    /// Check if the operation failed
    /// @return true if the operation failed, false otherwise
    bool isFailure() const noexcept { return !success_; }

    /// Get the error code (only valid if isFailure() == true)
    /// @return The error code
    ErrorCode errorCode() const noexcept { return errorCode_; }

    /// Get the error message (only valid if isFailure() == true)
    /// @return The error message, or nullptr if no message was provided
    const char* errorMessage() const noexcept { return errorMessage_; }

private:
    bool success_;              ///< Whether the operation succeeded
    ErrorCode errorCode_;       ///< Error code (valid only if !success_)
    const char* errorMessage_;  ///< Optional error message (valid only if !success_)
};

}  // namespace axiom::core
