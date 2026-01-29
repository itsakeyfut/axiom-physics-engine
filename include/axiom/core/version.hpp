#pragma once

namespace axiom::core {

/// Major version number
constexpr int VERSION_MAJOR = 0;

/// Minor version number
constexpr int VERSION_MINOR = 1;

/// Patch version number
constexpr int VERSION_PATCH = 0;

/// Get version string (e.g., "0.1.0")
const char* getVersionString();

/// Get version as integer (major * 10000 + minor * 100 + patch)
constexpr int getVersionNumber() {
    return VERSION_MAJOR * 10000 + VERSION_MINOR * 100 + VERSION_PATCH;
}

}  // namespace axiom::core
