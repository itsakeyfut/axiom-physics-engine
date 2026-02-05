#include "axiom/gpu/vk_compute_pipeline.hpp"

#include "axiom/core/error_code.hpp"
#include "axiom/core/logger.hpp"
#include "axiom/gpu/vk_descriptor.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_shader.hpp"

#include <cstring>
#include <fstream>

namespace axiom::gpu {

// ============================================================================
// ComputePipeline Implementation
// ============================================================================

ComputePipeline::ComputePipeline(VkContext* context, VkPipeline pipeline, VkPipelineLayout layout)
    : context_(context), pipeline_(pipeline), layout_(layout) {}

ComputePipeline::~ComputePipeline() {
    if (context_ && context_->getDevice()) {
        if (pipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(context_->getDevice(), pipeline_, nullptr);
            pipeline_ = VK_NULL_HANDLE;
        }
        if (layout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(context_->getDevice(), layout_, nullptr);
            layout_ = VK_NULL_HANDLE;
        }
    }
}

void ComputePipeline::bind(VkCommandBuffer cmd) const noexcept {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
}

void ComputePipeline::dispatch(VkCommandBuffer cmd, uint32_t groupCountX, uint32_t groupCountY,
                               uint32_t groupCountZ) const noexcept {
    vkCmdDispatch(cmd, groupCountX, groupCountY, groupCountZ);
}

void ComputePipeline::dispatchIndirect(VkCommandBuffer cmd, VkBuffer buffer,
                                       VkDeviceSize offset) const noexcept {
    vkCmdDispatchIndirect(cmd, buffer, offset);
}

// ============================================================================
// ComputePipelineBuilder Implementation
// ============================================================================

ComputePipelineBuilder::ComputePipelineBuilder(VkContext* context) : context_(context) {}

ComputePipelineBuilder& ComputePipelineBuilder::setShader(const ShaderModule& shader) {
    shader_ = &shader;
    return *this;
}

ComputePipelineBuilder&
ComputePipelineBuilder::setDescriptorSetLayout(const DescriptorSetLayout& layout) {
    descriptorSetLayouts_.clear();
    descriptorSetLayouts_.push_back(layout.get());
    return *this;
}

ComputePipelineBuilder&
ComputePipelineBuilder::addDescriptorSetLayout(const DescriptorSetLayout& layout) {
    descriptorSetLayouts_.push_back(layout.get());
    return *this;
}

ComputePipelineBuilder&
ComputePipelineBuilder::setPushConstantRange(const PushConstantRange& range) {
    pushConstantRange_ = range;
    return *this;
}

ComputePipelineBuilder&
ComputePipelineBuilder::addSpecializationConstant(const SpecializationConstant& constant) {
    VkSpecializationMapEntry entry{};
    entry.constantID = constant.constantId;
    entry.offset = constant.offset;
    entry.size = constant.size;
    specializationEntries_.push_back(entry);
    return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::setSpecializationData(const void* data,
                                                                      size_t size) {
    specializationData_.resize(size);
    std::memcpy(specializationData_.data(), data, size);
    return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::setPipelineCache(VkPipelineCache cache) {
    pipelineCache_ = cache;
    return *this;
}

core::Result<std::unique_ptr<ComputePipeline>> ComputePipelineBuilder::build() {
    // Validate context
    if (!context_) {
        return core::Result<std::unique_ptr<ComputePipeline>>::failure(
            core::ErrorCode::InvalidParameter, "VkContext is null");
    }

    // Validate shader
    if (!shader_) {
        return core::Result<std::unique_ptr<ComputePipeline>>::failure(
            core::ErrorCode::InvalidParameter, "Compute shader not set");
    }

    if (shader_->getStage() != ShaderStage::Compute) {
        return core::Result<std::unique_ptr<ComputePipeline>>::failure(
            core::ErrorCode::InvalidParameter, "Shader must be a compute shader");
    }

    VkDevice device = context_->getDevice();

    // Create pipeline layout
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts_.size());
    layoutInfo.pSetLayouts = descriptorSetLayouts_.empty() ? nullptr : descriptorSetLayouts_.data();

    // Setup push constant range
    VkPushConstantRange pushConstantRangeVk{};
    if (pushConstantRange_.has_value()) {
        pushConstantRangeVk.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRangeVk.offset = pushConstantRange_->offset;
        pushConstantRangeVk.size = pushConstantRange_->size;

        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstantRangeVk;
    }

    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkResult result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout);
    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("VkComputePipeline", "Failed to create pipeline layout: %d",
                        static_cast<int>(result));
        return core::Result<std::unique_ptr<ComputePipeline>>::failure(
            core::ErrorCode::GPU_OPERATION_FAILED, "Failed to create pipeline layout");
    }

    // Setup shader stage
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = shader_->getStageFlags();
    shaderStageInfo.module = shader_->get();
    shaderStageInfo.pName = shader_->getEntryPoint();

    // Setup specialization info if present
    VkSpecializationInfo specializationInfo{};
    if (!specializationEntries_.empty() && !specializationData_.empty()) {
        specializationInfo.mapEntryCount = static_cast<uint32_t>(specializationEntries_.size());
        specializationInfo.pMapEntries = specializationEntries_.data();
        specializationInfo.dataSize = specializationData_.size();
        specializationInfo.pData = specializationData_.data();
        shaderStageInfo.pSpecializationInfo = &specializationInfo;
    }

    // Create compute pipeline
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = shaderStageInfo;
    pipelineInfo.layout = layout;

    VkPipeline pipeline = VK_NULL_HANDLE;
    result = vkCreateComputePipelines(device, pipelineCache_, 1, &pipelineInfo, nullptr, &pipeline);

    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("VkComputePipeline", "Failed to create compute pipeline: %d",
                        static_cast<int>(result));
        vkDestroyPipelineLayout(device, layout, nullptr);
        return core::Result<std::unique_ptr<ComputePipeline>>::failure(
            core::ErrorCode::GPU_OPERATION_FAILED, "Failed to create compute pipeline");
    }

    AXIOM_LOG_INFO("VkComputePipeline", "Compute pipeline created successfully");

    // Create ComputePipeline object
    auto computePipeline = std::unique_ptr<ComputePipeline>(
        new ComputePipeline(context_, pipeline, layout));

    return core::Result<std::unique_ptr<ComputePipeline>>::success(std::move(computePipeline));
}

// ============================================================================
// PipelineCache Implementation
// ============================================================================

PipelineCache::PipelineCache(VkContext* context) : context_(context) {}

PipelineCache::~PipelineCache() {
    if (context_ && context_->getDevice() && cache_ != VK_NULL_HANDLE) {
        vkDestroyPipelineCache(context_->getDevice(), cache_, nullptr);
        cache_ = VK_NULL_HANDLE;
    }
}

core::Result<std::unique_ptr<PipelineCache>> PipelineCache::create(VkContext* context) {
    if (!context) {
        return core::Result<std::unique_ptr<PipelineCache>>::failure(
            core::ErrorCode::InvalidParameter, "VkContext is null");
    }

    auto cache = std::unique_ptr<PipelineCache>(new PipelineCache(context));

    auto initResult = cache->initialize();
    if (initResult.isFailure()) {
        return core::Result<std::unique_ptr<PipelineCache>>::failure(initResult.errorCode(),
                                                                     initResult.errorMessage());
    }

    return core::Result<std::unique_ptr<PipelineCache>>::success(std::move(cache));
}

core::Result<void> PipelineCache::initialize() {
    VkPipelineCacheCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    createInfo.initialDataSize = 0;
    createInfo.pInitialData = nullptr;

    VkResult result = vkCreatePipelineCache(context_->getDevice(), &createInfo, nullptr, &cache_);

    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("PipelineCache", "Failed to create pipeline cache: %d",
                        static_cast<int>(result));
        return core::Result<void>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                           "Failed to create pipeline cache");
    }

    AXIOM_LOG_INFO("PipelineCache", "Pipeline cache created successfully");
    return core::Result<void>::success();
}

core::Result<void> PipelineCache::save(const std::string& path) {
    if (cache_ == VK_NULL_HANDLE) {
        return core::Result<void>::failure(core::ErrorCode::GPU_INVALID_OPERATION,
                                           "Pipeline cache not initialized");
    }

    // Get cache data size
    size_t dataSize = 0;
    VkResult result = vkGetPipelineCacheData(context_->getDevice(), cache_, &dataSize, nullptr);

    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("PipelineCache", "Failed to get pipeline cache data size: %d",
                        static_cast<int>(result));
        return core::Result<void>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                           "Failed to get pipeline cache data size");
    }

    if (dataSize == 0) {
        AXIOM_LOG_INFO("PipelineCache", "Pipeline cache is empty, skipping save");
        return core::Result<void>::success();
    }

    // Get cache data
    std::vector<uint8_t> cacheData(dataSize);
    result = vkGetPipelineCacheData(context_->getDevice(), cache_, &dataSize, cacheData.data());

    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("PipelineCache", "Failed to get pipeline cache data: %d",
                        static_cast<int>(result));
        return core::Result<void>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                           "Failed to get pipeline cache data");
    }

    // Write to file
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        AXIOM_LOG_ERROR("PipelineCache", "Failed to open pipeline cache file for writing: %s",
                        path.c_str());
        return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                           "Failed to open file for writing");
    }

    file.write(reinterpret_cast<const char*>(cacheData.data()),
               static_cast<std::streamsize>(dataSize));
    file.close();

    AXIOM_LOG_INFO("PipelineCache", "Pipeline cache saved to %s (%zu bytes)", path.c_str(),
                   dataSize);
    return core::Result<void>::success();
}

core::Result<void> PipelineCache::load(const std::string& path) {
    // Open file
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        AXIOM_LOG_INFO("PipelineCache",
                       "Pipeline cache file not found: %s (this is normal on first run)",
                       path.c_str());
        return core::Result<void>::success();  // Not an error if file doesn't exist
    }

    // Get file size
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize <= 0) {
        AXIOM_LOG_INFO("PipelineCache", "Pipeline cache file is empty: %s", path.c_str());
        return core::Result<void>::success();
    }

    // Read file data
    std::vector<uint8_t> cacheData(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(cacheData.data()), fileSize)) {
        AXIOM_LOG_ERROR("PipelineCache", "Failed to read pipeline cache file: %s", path.c_str());
        return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                           "Failed to read pipeline cache file");
    }
    file.close();

    // Destroy existing cache
    if (cache_ != VK_NULL_HANDLE) {
        vkDestroyPipelineCache(context_->getDevice(), cache_, nullptr);
        cache_ = VK_NULL_HANDLE;
    }

    // Create new cache with loaded data
    VkPipelineCacheCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    createInfo.initialDataSize = cacheData.size();
    createInfo.pInitialData = cacheData.data();

    VkResult result = vkCreatePipelineCache(context_->getDevice(), &createInfo, nullptr, &cache_);

    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("PipelineCache", "Failed to create pipeline cache from loaded data: %d",
                        static_cast<int>(result));
        // Create empty cache as fallback
        return initialize();
    }

    AXIOM_LOG_INFO("PipelineCache", "Pipeline cache loaded from %s (%lld bytes)", path.c_str(),
                   static_cast<long long>(fileSize));
    return core::Result<void>::success();
}

}  // namespace axiom::gpu
