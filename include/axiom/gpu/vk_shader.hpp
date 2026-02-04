#pragma once

#include "axiom/core/result.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace axiom::gpu {

// Forward declaration
class VkContext;

/// Shader stage enumeration
/// Maps to Vulkan shader stage flags for different pipeline stages
enum class ShaderStage {
    Vertex,          ///< Vertex shader stage
    Fragment,        ///< Fragment (pixel) shader stage
    Compute,         ///< Compute shader stage
    Geometry,        ///< Geometry shader stage (optional)
    TessControl,     ///< Tessellation control shader stage (optional)
    TessEvaluation,  ///< Tessellation evaluation shader stage (optional)
};

/// Descriptor binding information extracted from shader reflection
struct BindingInfo {
    uint32_t binding;           ///< Binding number
    VkDescriptorType type;      ///< Descriptor type (uniform, storage, etc.)
    uint32_t count;             ///< Array size (1 for non-arrays)
    VkShaderStageFlags stages;  ///< Shader stages that use this binding
};

/// Push constant information extracted from shader reflection
struct PushConstantInfo {
    uint32_t offset;            ///< Offset in bytes
    uint32_t size;              ///< Size in bytes
    VkShaderStageFlags stages;  ///< Shader stages that use this push constant
};

/// Vulkan shader module wrapper
/// This class manages a VkShaderModule and provides utilities for shader loading,
/// validation, and optional reflection. Shaders must be in SPIR-V format.
///
/// Features:
/// - Load SPIR-V from file or memory
/// - Validate SPIR-V magic number and alignment
/// - Automatic cleanup of Vulkan resources
/// - Optional shader reflection (future extension)
///
/// Example usage:
/// @code
/// auto shaderResult = ShaderModule::createFromFile(
///     context,
///     "shaders/particle_update.comp.spv",
///     ShaderStage::Compute
/// );
///
/// if (shaderResult.isSuccess()) {
///     auto shader = std::move(shaderResult.value());
///
///     // Use in pipeline creation
///     VkPipelineShaderStageCreateInfo stageInfo{};
///     stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
///     stageInfo.stage = shader->getStageFlags();
///     stageInfo.module = shader->get();
///     stageInfo.pName = shader->getEntryPoint();
/// }
/// @endcode
class ShaderModule {
public:
    /// Create shader module from SPIR-V file
    /// @param context Valid VkContext pointer (must outlive this shader)
    /// @param spirvPath Path to .spv file
    /// @param stage Shader stage
    /// @return Result containing unique_ptr to ShaderModule or error code
    static core::Result<std::unique_ptr<ShaderModule>>
    createFromFile(VkContext* context, const std::string& spirvPath, ShaderStage stage);

    /// Create shader module from SPIR-V bytecode in memory
    /// @param context Valid VkContext pointer (must outlive this shader)
    /// @param spirvCode SPIR-V bytecode (must be uint32_t aligned)
    /// @param stage Shader stage
    /// @return Result containing unique_ptr to ShaderModule or error code
    static core::Result<std::unique_ptr<ShaderModule>>
    createFromCode(VkContext* context, const std::vector<uint32_t>& spirvCode, ShaderStage stage);

    /// Destructor - cleans up VkShaderModule
    ~ShaderModule();

    // Non-copyable, non-movable
    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;
    ShaderModule(ShaderModule&&) = delete;
    ShaderModule& operator=(ShaderModule&&) = delete;

    /// Get the Vulkan shader module handle
    /// @return VkShaderModule handle
    VkShaderModule get() const noexcept { return module_; }

    /// Get the shader stage
    /// @return ShaderStage enum
    ShaderStage getStage() const noexcept { return stage_; }

    /// Get Vulkan shader stage flags
    /// @return VkShaderStageFlagBits for use in pipeline creation
    VkShaderStageFlagBits getStageFlags() const noexcept;

    /// Get the entry point name
    /// @return Entry point name (always "main")
    const char* getEntryPoint() const noexcept { return "main"; }

    /// Get the SPIR-V bytecode
    /// @return Vector of uint32_t containing SPIR-V bytecode
    const std::vector<uint32_t>& getSpirv() const noexcept { return spirvCode_; }

    /// Get shader source file path (if loaded from file)
    /// @return Source file path, or empty string if created from memory
    const std::string& getSourcePath() const noexcept { return sourcePath_; }

    /// Get descriptor binding information (requires reflection, optional feature)
    /// @return Vector of binding information
    std::vector<BindingInfo> getBindings() const;

    /// Get push constant information (requires reflection, optional feature)
    /// @return Optional push constant info, nullopt if not present
    std::optional<PushConstantInfo> getPushConstantInfo() const;

private:
    /// Private constructor - use create methods instead
    ShaderModule(VkContext* context, ShaderStage stage);

    /// Initialize from SPIR-V bytecode
    /// @param spirvCode SPIR-V bytecode
    /// @return Result indicating success or failure
    core::Result<void> initFromSpirv(const std::vector<uint32_t>& spirvCode);

    /// Validate SPIR-V bytecode
    /// @param spirvCode SPIR-V bytecode to validate
    /// @return Result indicating success or failure
    static core::Result<void> validateSpirv(const std::vector<uint32_t>& spirvCode);

    /// Convert ShaderStage enum to VkShaderStageFlagBits
    /// @param stage ShaderStage enum
    /// @return Corresponding VkShaderStageFlagBits
    static VkShaderStageFlagBits shaderStageToVk(ShaderStage stage) noexcept;

    VkContext* context_;               ///< Vulkan context (not owned)
    VkShaderModule module_;            ///< Vulkan shader module handle
    ShaderStage stage_;                ///< Shader stage
    std::vector<uint32_t> spirvCode_;  ///< SPIR-V bytecode
    std::string sourcePath_;           ///< Source file path (if applicable)
};

/// Runtime Slang/HLSL to SPIR-V compiler (optional feature)
/// This class provides utilities for compiling Slang/HLSL source code to SPIR-V at runtime.
/// Requires Slang compiler integration.
///
/// Note: This is an optional feature. Most production code should use pre-compiled
/// SPIR-V shaders for better performance and reliability. We use Slang as our primary
/// shader language (a superset of HLSL).
class ShaderCompiler {
public:
    /// Compile Slang/HLSL source code to SPIR-V
    /// @param source Slang/HLSL source code
    /// @param stage Shader stage
    /// @param filename Source filename (for error messages)
    /// @return Result containing SPIR-V bytecode or error code
    static core::Result<std::vector<uint32_t>> compileSlang(const std::string& source,
                                                            ShaderStage stage,
                                                            const std::string& filename = "shader");

    /// Compile Slang/HLSL source file to SPIR-V
    /// @param path Path to Slang/HLSL source file
    /// @param stage Shader stage
    /// @return Result containing SPIR-V bytecode or error code
    static core::Result<std::vector<uint32_t>> compileSlangFromFile(const std::string& path,
                                                                    ShaderStage stage);

private:
    /// Initialize Slang compiler (called once)
    static void initializeSlang();

    /// Check if Slang is initialized
    static bool isSlangInitialized_;
};

/// Shader cache for avoiding duplicate loading
/// This singleton class provides caching for shader modules based on file path.
/// Helps avoid redundant I/O and shader module creation.
///
/// Example usage:
/// @code
/// auto& cache = ShaderCache::instance();
///
/// // First load - reads from disk
/// auto shader1 = cache.load(context, "shaders/test.comp.spv", ShaderStage::Compute);
///
/// // Second load - returns cached instance
/// auto shader2 = cache.load(context, "shaders/test.comp.spv", ShaderStage::Compute);
///
/// // shader1 == shader2 (same shared_ptr)
/// @endcode
class ShaderCache {
public:
    /// Get the singleton instance
    /// @return Reference to the global shader cache
    static ShaderCache& instance();

    /// Load shader from file (with caching)
    /// If the shader is already cached, returns the cached instance.
    /// Otherwise, loads from disk and caches it.
    /// @param context Valid VkContext pointer
    /// @param path Path to SPIR-V shader file
    /// @param stage Shader stage
    /// @return Result containing shared_ptr to ShaderModule or error code
    core::Result<std::shared_ptr<ShaderModule>> load(VkContext* context, const std::string& path,
                                                     ShaderStage stage);

    /// Clear all cached shaders
    /// This will invalidate all shared_ptrs if they are the only references
    void clear();

    /// Get number of cached shaders
    /// @return Number of shaders in cache
    size_t size() const noexcept { return cache_.size(); }

    /// Check if a shader is cached
    /// @param path Shader file path
    /// @return true if shader is in cache
    bool contains(const std::string& path) const;

private:
    /// Private constructor (singleton pattern)
    ShaderCache() = default;

    /// Destructor
    ~ShaderCache() = default;

    // Non-copyable, non-movable
    ShaderCache(const ShaderCache&) = delete;
    ShaderCache& operator=(const ShaderCache&) = delete;
    ShaderCache(ShaderCache&&) = delete;
    ShaderCache& operator=(ShaderCache&&) = delete;

    /// Cache storage: file path -> shader module
    std::unordered_map<std::string, std::shared_ptr<ShaderModule>> cache_;
};

}  // namespace axiom::gpu
