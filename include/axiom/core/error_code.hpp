#pragma once

namespace axiom::core {

/// Error categories for different subsystems
enum class ErrorCategory {
    None,       ///< No error
    Math,       ///< Mathematical operations (division by zero, normalization of zero vector)
    Memory,     ///< Memory allocation failures
    Collision,  ///< Collision detection errors
    Dynamics,   ///< Rigid body simulation errors
    SoftBody,   ///< Soft body simulation errors
    Fluid,      ///< Fluid simulation errors
    GPU,        ///< GPU compute errors
    IO,         ///< File I/O errors
    Validation  ///< Input validation errors
};

/// Specific error codes for different failure modes
/// Error codes are organized by category in ranges of 100
enum class ErrorCode {
    Success = 0,  ///< No error occurred

    // Math errors (100-199)
    DivisionByZero = 100,       ///< Attempted division by zero
    NormalizationOfZeroVector,  ///< Attempted to normalize a zero-length vector
    InvalidMatrixInversion,     ///< Matrix is singular and cannot be inverted
    InvalidQuaternion,          ///< Quaternion is invalid (not normalized or contains NaN)

    // Memory errors (200-299)
    OutOfMemory = 200,  ///< Memory allocation failed
    InvalidAllocation,  ///< Invalid allocation parameters (size, alignment)
    NullPointer,        ///< Unexpected null pointer encountered

    // Collision errors (300-399)
    InvalidShape = 300,   ///< Collision shape is invalid or malformed
    GJKFailedToConverge,  ///< GJK algorithm failed to converge
    EPAFailedToConverge,  ///< EPA algorithm failed to converge

    // Dynamics errors (400-499)
    InvalidRigidBody = 400,   ///< Rigid body has invalid properties
    ConstraintSolverFailure,  ///< Constraint solver failed to converge

    // GPU errors (500-599)
    VulkanInitializationFailed = 500,  ///< Vulkan initialization failed
    ShaderCompilationFailed,           ///< Shader compilation failed
    BufferAllocationFailed,            ///< GPU buffer allocation failed

    // Validation errors (600-699)
    InvalidParameter = 600,  ///< Function parameter is invalid
    OutOfRange,              ///< Value is out of valid range

    // Add more error codes as needed
};

/// Convert error code to human-readable string
/// @param code The error code to convert
/// @return A static string describing the error (valid for the lifetime of the program)
const char* errorCodeToString(ErrorCode code) noexcept;

/// Get error category from error code
/// @param code The error code to categorize
/// @return The category this error code belongs to
ErrorCategory getErrorCategory(ErrorCode code) noexcept;

/// Convert error category to human-readable string
/// @param category The error category to convert
/// @return A static string describing the category (valid for the lifetime of the program)
const char* errorCategoryToString(ErrorCategory category) noexcept;

}  // namespace axiom::core
