#include "axiom/core/error_code.hpp"

namespace axiom::core {

const char* errorCodeToString(ErrorCode code) noexcept {
    switch (code) {
    case ErrorCode::Success:
        return "Success";

    // Math errors (100-199)
    case ErrorCode::DivisionByZero:
        return "Division by zero";
    case ErrorCode::NormalizationOfZeroVector:
        return "Cannot normalize zero vector";
    case ErrorCode::InvalidMatrixInversion:
        return "Matrix is singular and cannot be inverted";
    case ErrorCode::InvalidQuaternion:
        return "Quaternion is invalid (not normalized or contains NaN)";

    // Memory errors (200-299)
    case ErrorCode::OutOfMemory:
        return "Out of memory";
    case ErrorCode::InvalidAllocation:
        return "Invalid allocation parameters";
    case ErrorCode::NullPointer:
        return "Unexpected null pointer";

    // Collision errors (300-399)
    case ErrorCode::InvalidShape:
        return "Invalid collision shape";
    case ErrorCode::GJKFailedToConverge:
        return "GJK algorithm failed to converge";
    case ErrorCode::EPAFailedToConverge:
        return "EPA algorithm failed to converge";

    // Dynamics errors (400-499)
    case ErrorCode::InvalidRigidBody:
        return "Invalid rigid body properties";
    case ErrorCode::ConstraintSolverFailure:
        return "Constraint solver failed to converge";

    // GPU errors (500-599)
    case ErrorCode::VulkanInitializationFailed:
        return "Vulkan initialization failed";
    case ErrorCode::ShaderCompilationFailed:
        return "Shader compilation failed";
    case ErrorCode::BufferAllocationFailed:
        return "GPU buffer allocation failed";
    case ErrorCode::GPU_INVALID_OPERATION:
        return "Invalid GPU operation or state";
    case ErrorCode::GPU_TIMEOUT:
        return "GPU operation timed out";
    case ErrorCode::GPU_OPERATION_FAILED:
        return "GPU operation failed";

    // Validation errors (600-699)
    case ErrorCode::InvalidParameter:
        return "Invalid parameter";
    case ErrorCode::OutOfRange:
        return "Value out of range";

    default:
        return "Unknown error";
    }
}

ErrorCategory getErrorCategory(ErrorCode code) noexcept {
    int codeValue = static_cast<int>(code);

    if (codeValue == 0) {
        return ErrorCategory::None;
    } else if (codeValue >= 100 && codeValue < 200) {
        return ErrorCategory::Math;
    } else if (codeValue >= 200 && codeValue < 300) {
        return ErrorCategory::Memory;
    } else if (codeValue >= 300 && codeValue < 400) {
        return ErrorCategory::Collision;
    } else if (codeValue >= 400 && codeValue < 500) {
        return ErrorCategory::Dynamics;
    } else if (codeValue >= 500 && codeValue < 600) {
        return ErrorCategory::GPU;
    } else if (codeValue >= 600 && codeValue < 700) {
        return ErrorCategory::Validation;
    } else {
        return ErrorCategory::None;
    }
}

const char* errorCategoryToString(ErrorCategory category) noexcept {
    switch (category) {
    case ErrorCategory::None:
        return "None";
    case ErrorCategory::Math:
        return "Math";
    case ErrorCategory::Memory:
        return "Memory";
    case ErrorCategory::Collision:
        return "Collision";
    case ErrorCategory::Dynamics:
        return "Dynamics";
    case ErrorCategory::SoftBody:
        return "SoftBody";
    case ErrorCategory::Fluid:
        return "Fluid";
    case ErrorCategory::GPU:
        return "GPU";
    case ErrorCategory::IO:
        return "IO";
    case ErrorCategory::Validation:
        return "Validation";
    default:
        return "Unknown";
    }
}

}  // namespace axiom::core
