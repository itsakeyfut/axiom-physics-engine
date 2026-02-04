#include "axiom/gpu/vk_shader.hpp"

#include "axiom/core/logger.hpp"
#include "axiom/gpu/vk_instance.hpp"

#include <fstream>

namespace axiom::gpu {

namespace {

/// SPIR-V magic number (0x07230203)
constexpr uint32_t SPIRV_MAGIC = 0x07230203;

/// Read binary file into vector
/// @param path File path
/// @return Result containing file data or error code
core::Result<std::vector<uint32_t>> readBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        AXIOM_LOG_ERROR("VkShader", "Failed to open shader file: %s", path.c_str());
        return core::Result<std::vector<uint32_t>>::failure(core::ErrorCode::InvalidParameter,
                                                            "Failed to open shader file");
    }

    // Get file size
    const size_t fileSize = static_cast<size_t>(file.tellg());
    if (fileSize == 0) {
        AXIOM_LOG_ERROR("VkShader", "Shader file is empty: %s", path.c_str());
        return core::Result<std::vector<uint32_t>>::failure(core::ErrorCode::InvalidParameter,
                                                            "Shader file is empty");
    }

    // SPIR-V must be 4-byte aligned
    if (fileSize % 4 != 0) {
        AXIOM_LOG_ERROR("VkShader", "Shader file size is not 4-byte aligned: %s (size: %zu)",
                        path.c_str(), fileSize);
        return core::Result<std::vector<uint32_t>>::failure(
            core::ErrorCode::InvalidParameter, "Shader file size is not 4-byte aligned");
    }

    // Read file into buffer
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(fileSize));

    if (!file) {
        AXIOM_LOG_ERROR("VkShader", "Failed to read shader file: %s", path.c_str());
        return core::Result<std::vector<uint32_t>>::failure(core::ErrorCode::InvalidParameter,
                                                            "Failed to read shader file");
    }

    return core::Result<std::vector<uint32_t>>::success(std::move(buffer));
}

}  // namespace

// ========================================
// ShaderModule implementation
// ========================================

ShaderModule::ShaderModule(VkContext* context, ShaderStage stage)
    : context_(context), module_(VK_NULL_HANDLE), stage_(stage) {}

ShaderModule::~ShaderModule() {
    if (module_ != VK_NULL_HANDLE && context_ != nullptr) {
        vkDestroyShaderModule(context_->getDevice(), module_, nullptr);
        module_ = VK_NULL_HANDLE;
    }
}

core::Result<std::unique_ptr<ShaderModule>>
ShaderModule::createFromFile(VkContext* context, const std::string& spirvPath, ShaderStage stage) {
    if (context == nullptr) {
        AXIOM_LOG_ERROR("VkShader", "Context is null");
        return core::Result<std::unique_ptr<ShaderModule>>::failure(
            core::ErrorCode::InvalidParameter, "Context cannot be null");
    }

    // Read SPIR-V file
    auto readResult = readBinaryFile(spirvPath);
    if (readResult.isFailure()) {
        return core::Result<std::unique_ptr<ShaderModule>>::failure(readResult.errorCode(),
                                                                    readResult.errorMessage());
    }

    auto spirvCode = readResult.value();

    // Validate SPIR-V
    auto validateResult = validateSpirv(spirvCode);
    if (validateResult.isFailure()) {
        return core::Result<std::unique_ptr<ShaderModule>>::failure(validateResult.errorCode(),
                                                                    validateResult.errorMessage());
    }

    // Create shader module
    auto shader = std::unique_ptr<ShaderModule>(new ShaderModule(context, stage));
    shader->sourcePath_ = spirvPath;

    auto initResult = shader->initFromSpirv(spirvCode);
    if (initResult.isFailure()) {
        return core::Result<std::unique_ptr<ShaderModule>>::failure(initResult.errorCode(),
                                                                    initResult.errorMessage());
    }

    AXIOM_LOG_INFO("VkShader", "Loaded shader from file: %s", spirvPath.c_str());
    return core::Result<std::unique_ptr<ShaderModule>>::success(std::move(shader));
}

core::Result<std::unique_ptr<ShaderModule>>
ShaderModule::createFromCode(VkContext* context, const std::vector<uint32_t>& spirvCode,
                             ShaderStage stage) {
    if (context == nullptr) {
        AXIOM_LOG_ERROR("VkShader", "Context is null");
        return core::Result<std::unique_ptr<ShaderModule>>::failure(
            core::ErrorCode::InvalidParameter, "Context cannot be null");
    }

    // Validate SPIR-V
    auto validateResult = validateSpirv(spirvCode);
    if (validateResult.isFailure()) {
        return core::Result<std::unique_ptr<ShaderModule>>::failure(validateResult.errorCode(),
                                                                    validateResult.errorMessage());
    }

    // Create shader module
    auto shader = std::unique_ptr<ShaderModule>(new ShaderModule(context, stage));

    auto initResult = shader->initFromSpirv(spirvCode);
    if (initResult.isFailure()) {
        return core::Result<std::unique_ptr<ShaderModule>>::failure(initResult.errorCode(),
                                                                    initResult.errorMessage());
    }

    AXIOM_LOG_INFO("VkShader", "Created shader from bytecode");
    return core::Result<std::unique_ptr<ShaderModule>>::success(std::move(shader));
}

core::Result<void> ShaderModule::initFromSpirv(const std::vector<uint32_t>& spirvCode) {
    // Store SPIR-V code
    spirvCode_ = spirvCode;

    // Create Vulkan shader module
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvCode_.size() * sizeof(uint32_t);
    createInfo.pCode = spirvCode_.data();

    VkResult result = vkCreateShaderModule(context_->getDevice(), &createInfo, nullptr, &module_);
    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("VkShader", "Failed to create shader module: VkResult = %d", result);
        return core::Result<void>::failure(core::ErrorCode::ShaderCompilationFailed,
                                           "Failed to create VkShaderModule");
    }

    return core::Result<void>::success();
}

core::Result<void> ShaderModule::validateSpirv(const std::vector<uint32_t>& spirvCode) {
    // Check minimum size (SPIR-V header is at least 5 words)
    if (spirvCode.size() < 5) {
        AXIOM_LOG_ERROR("VkShader", "SPIR-V bytecode is too small: %zu bytes",
                        spirvCode.size() * sizeof(uint32_t));
        return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                           "SPIR-V bytecode is too small");
    }

    // Check magic number
    if (spirvCode[0] != SPIRV_MAGIC) {
        AXIOM_LOG_ERROR("VkShader", "Invalid SPIR-V magic number: 0x%08X (expected 0x%08X)",
                        spirvCode[0], SPIRV_MAGIC);
        return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                           "Invalid SPIR-V magic number");
    }

    return core::Result<void>::success();
}

VkShaderStageFlagBits ShaderModule::getStageFlags() const noexcept {
    return shaderStageToVk(stage_);
}

VkShaderStageFlagBits ShaderModule::shaderStageToVk(ShaderStage stage) noexcept {
    switch (stage) {
    case ShaderStage::Vertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::Fragment:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderStage::Compute:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    case ShaderStage::Geometry:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    case ShaderStage::TessControl:
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case ShaderStage::TessEvaluation:
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    default:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    }
}

std::vector<BindingInfo> ShaderModule::getBindings() const {
    // Shader reflection is an optional feature (requires SPIRV-Cross)
    // For now, return empty vector
    // TODO: Implement shader reflection using SPIRV-Cross
    return {};
}

std::optional<PushConstantInfo> ShaderModule::getPushConstantInfo() const {
    // Shader reflection is an optional feature (requires SPIRV-Cross)
    // For now, return nullopt
    // TODO: Implement shader reflection using SPIRV-Cross
    return std::nullopt;
}

// ========================================
// ShaderCompiler implementation
// ========================================

bool ShaderCompiler::isSlangInitialized_ = false;

core::Result<std::vector<uint32_t>> ShaderCompiler::compileSlang(const std::string& source,
                                                                 ShaderStage stage,
                                                                 const std::string& filename) {
    // Suppress unused parameter warnings
    (void)source;
    (void)stage;
    (void)filename;

    // Runtime Slang compilation is an optional feature (requires Slang compiler integration)
    // For now, return error
    // TODO: Implement runtime compilation using Slang compiler API
    AXIOM_LOG_ERROR("VkShader", "Runtime Slang compilation is not implemented");
    return core::Result<std::vector<uint32_t>>::failure(
        core::ErrorCode::ShaderCompilationFailed, "Runtime Slang compilation is not implemented");
}

core::Result<std::vector<uint32_t>> ShaderCompiler::compileSlangFromFile(const std::string& path,
                                                                         ShaderStage stage) {
    // Read Slang/HLSL file
    std::ifstream file(path, std::ios::ate);
    if (!file.is_open()) {
        AXIOM_LOG_ERROR("VkShader", "Failed to open Slang shader file: %s", path.c_str());
        return core::Result<std::vector<uint32_t>>::failure(core::ErrorCode::InvalidParameter,
                                                            "Failed to open Slang shader file");
    }

    // Get file size and read content
    const auto fileSize = file.tellg();
    if (fileSize <= 0) {
        AXIOM_LOG_ERROR("VkShader", "Slang shader file is empty: %s", path.c_str());
        return core::Result<std::vector<uint32_t>>::failure(core::ErrorCode::InvalidParameter,
                                                            "Slang shader file is empty");
    }

    file.seekg(0);
    std::string source(static_cast<size_t>(fileSize), '\0');
    file.read(&source[0], fileSize);

    if (!file) {
        AXIOM_LOG_ERROR("VkShader", "Failed to read Slang shader file: %s", path.c_str());
        return core::Result<std::vector<uint32_t>>::failure(core::ErrorCode::InvalidParameter,
                                                            "Failed to read Slang shader file");
    }

    return compileSlang(source, stage, path);
}

void ShaderCompiler::initializeSlang() {
    // TODO: Initialize Slang compiler library
    isSlangInitialized_ = true;
}

// ========================================
// ShaderCache implementation
// ========================================

ShaderCache& ShaderCache::instance() {
    static ShaderCache cache;
    return cache;
}

core::Result<std::shared_ptr<ShaderModule>>
ShaderCache::load(VkContext* context, const std::string& path, ShaderStage stage) {
    if (context == nullptr) {
        AXIOM_LOG_ERROR("VkShader", "Context is null");
        return core::Result<std::shared_ptr<ShaderModule>>::failure(
            core::ErrorCode::InvalidParameter, "Context cannot be null");
    }

    // Check if shader is already cached
    auto it = cache_.find(path);
    if (it != cache_.end()) {
        AXIOM_LOG_DEBUG("VkShader", "Shader cache hit: %s", path.c_str());
        return core::Result<std::shared_ptr<ShaderModule>>::success(it->second);
    }

    // Load shader from file
    auto loadResult = ShaderModule::createFromFile(context, path, stage);
    if (loadResult.isFailure()) {
        return core::Result<std::shared_ptr<ShaderModule>>::failure(loadResult.errorCode(),
                                                                    loadResult.errorMessage());
    }

    // Store in cache as shared_ptr
    auto shader = std::shared_ptr<ShaderModule>(std::move(loadResult.value()));
    cache_[path] = shader;

    AXIOM_LOG_DEBUG("VkShader", "Shader cached: %s (total cached: %zu)", path.c_str(),
                    cache_.size());
    return core::Result<std::shared_ptr<ShaderModule>>::success(shader);
}

void ShaderCache::clear() {
    const size_t count = cache_.size();
    cache_.clear();
    AXIOM_LOG_INFO("VkShader", "Shader cache cleared (%zu shaders removed)", count);
}

bool ShaderCache::contains(const std::string& path) const {
    return cache_.find(path) != cache_.end();
}

}  // namespace axiom::gpu
