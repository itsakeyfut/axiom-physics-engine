#include "axiom/gpu/vk_descriptor.hpp"

#include "axiom/core/logger.hpp"
#include "axiom/gpu/vk_instance.hpp"

#include <algorithm>

namespace axiom::gpu {

// ========================================
// DescriptorSetLayout implementation
// ========================================

DescriptorSetLayout::DescriptorSetLayout(VkContext* context) : context_(context) {}

DescriptorSetLayout::~DescriptorSetLayout() {
    if (layout_ != VK_NULL_HANDLE && context_ != nullptr) {
        vkDestroyDescriptorSetLayout(context_->getDevice(), layout_, nullptr);
        layout_ = VK_NULL_HANDLE;
    }
}

core::Result<std::unique_ptr<DescriptorSetLayout>>
DescriptorSetLayout::create(VkContext* context,
                            const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
    if (context == nullptr) {
        AXIOM_LOG_ERROR("VkDescriptor", "Context is null");
        return core::Result<std::unique_ptr<DescriptorSetLayout>>::failure(
            core::ErrorCode::InvalidParameter, "Context cannot be null");
    }

    if (bindings.empty()) {
        AXIOM_LOG_ERROR("VkDescriptor", "Cannot create descriptor set layout with no bindings");
        return core::Result<std::unique_ptr<DescriptorSetLayout>>::failure(
            core::ErrorCode::InvalidParameter,
            "Descriptor set layout must have at least one binding");
    }

    auto layout = std::unique_ptr<DescriptorSetLayout>(new DescriptorSetLayout(context));

    auto initResult = layout->initialize(bindings);
    if (initResult.isFailure()) {
        return core::Result<std::unique_ptr<DescriptorSetLayout>>::failure(
            initResult.errorCode(), initResult.errorMessage());
    }

    AXIOM_LOG_INFO("VkDescriptor", "Created descriptor set layout with %zu bindings",
                   bindings.size());
    return core::Result<std::unique_ptr<DescriptorSetLayout>>::success(std::move(layout));
}

core::Result<void>
DescriptorSetLayout::initialize(const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
    bindings_ = bindings;

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = static_cast<uint32_t>(bindings_.size());
    createInfo.pBindings = bindings_.data();

    VkResult result = vkCreateDescriptorSetLayout(context_->getDevice(), &createInfo, nullptr,
                                                  &layout_);
    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("VkDescriptor", "Failed to create descriptor set layout (VkResult: %d)",
                        static_cast<int>(result));
        return core::Result<void>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                           "Failed to create descriptor set layout");
    }

    return core::Result<void>::success();
}

// ========================================
// DescriptorSetLayoutBuilder implementation
// ========================================

DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder(VkContext* context) : context_(context) {}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::addBinding(uint32_t binding,
                                                                   VkDescriptorType type,
                                                                   VkShaderStageFlags stages,
                                                                   uint32_t count) {
    // Check for duplicate bindings
    auto it = std::find_if(
        bindings_.begin(), bindings_.end(),
        [binding](const VkDescriptorSetLayoutBinding& b) { return b.binding == binding; });

    if (it != bindings_.end()) {
        AXIOM_LOG_DEBUG("VkDescriptor",
                        "Binding %u already exists in layout, it will be overwritten", binding);
        *it = VkDescriptorSetLayoutBinding{binding, type, count, stages, nullptr};
    } else {
        bindings_.push_back(VkDescriptorSetLayoutBinding{binding, type, count, stages, nullptr});
    }

    return *this;
}

core::Result<std::unique_ptr<DescriptorSetLayout>> DescriptorSetLayoutBuilder::build() {
    if (bindings_.empty()) {
        AXIOM_LOG_ERROR("VkDescriptor", "Cannot build descriptor set layout with no bindings");
        return core::Result<std::unique_ptr<DescriptorSetLayout>>::failure(
            core::ErrorCode::InvalidParameter,
            "Descriptor set layout must have at least one binding");
    }

    return DescriptorSetLayout::create(context_, bindings_);
}

// ========================================
// DescriptorPool implementation
// ========================================

DescriptorPool::DescriptorPool(VkContext* context) : context_(context) {}

DescriptorPool::~DescriptorPool() {
    if (pool_ != VK_NULL_HANDLE && context_ != nullptr) {
        vkDestroyDescriptorPool(context_->getDevice(), pool_, nullptr);
        pool_ = VK_NULL_HANDLE;
    }
}

core::Result<std::unique_ptr<DescriptorPool>>
DescriptorPool::create(VkContext* context, const std::vector<PoolSize>& sizes, uint32_t maxSets) {
    if (context == nullptr) {
        AXIOM_LOG_ERROR("VkDescriptor", "Context is null");
        return core::Result<std::unique_ptr<DescriptorPool>>::failure(
            core::ErrorCode::InvalidParameter, "Context cannot be null");
    }

    if (sizes.empty()) {
        AXIOM_LOG_ERROR("VkDescriptor", "Cannot create descriptor pool with no pool sizes");
        return core::Result<std::unique_ptr<DescriptorPool>>::failure(
            core::ErrorCode::InvalidParameter, "Descriptor pool must have at least one pool size");
    }

    if (maxSets == 0) {
        AXIOM_LOG_ERROR("VkDescriptor", "Cannot create descriptor pool with maxSets = 0");
        return core::Result<std::unique_ptr<DescriptorPool>>::failure(
            core::ErrorCode::InvalidParameter, "maxSets must be greater than 0");
    }

    auto pool = std::unique_ptr<DescriptorPool>(new DescriptorPool(context));

    auto initResult = pool->initialize(sizes, maxSets);
    if (initResult.isFailure()) {
        return core::Result<std::unique_ptr<DescriptorPool>>::failure(initResult.errorCode(),
                                                                      initResult.errorMessage());
    }

    AXIOM_LOG_INFO("VkDescriptor", "Created descriptor pool with maxSets=%u", maxSets);
    return core::Result<std::unique_ptr<DescriptorPool>>::success(std::move(pool));
}

core::Result<void> DescriptorPool::initialize(const std::vector<PoolSize>& sizes,
                                              uint32_t maxSets) {
    maxSets_ = maxSets;

    // Convert PoolSize to VkDescriptorPoolSize
    std::vector<VkDescriptorPoolSize> vkPoolSizes;
    vkPoolSizes.reserve(sizes.size());
    for (const auto& size : sizes) {
        vkPoolSizes.push_back({size.type, size.count});
    }

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;  // Allow reset
    createInfo.maxSets = maxSets;
    createInfo.poolSizeCount = static_cast<uint32_t>(vkPoolSizes.size());
    createInfo.pPoolSizes = vkPoolSizes.data();

    VkResult result = vkCreateDescriptorPool(context_->getDevice(), &createInfo, nullptr, &pool_);
    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("VkDescriptor", "Failed to create descriptor pool (VkResult: %d)",
                        static_cast<int>(result));
        return core::Result<void>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                           "Failed to create descriptor pool");
    }

    return core::Result<void>::success();
}

core::Result<VkDescriptorSet> DescriptorPool::allocate(const DescriptorSetLayout& layout) {
    VkDescriptorSetLayout layouts[] = {layout.get()};

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;

    VkDescriptorSet descriptorSet;
    VkResult result = vkAllocateDescriptorSets(context_->getDevice(), &allocInfo, &descriptorSet);
    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("VkDescriptor", "Failed to allocate descriptor set (VkResult: %d)",
                        static_cast<int>(result));
        return core::Result<VkDescriptorSet>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                                      "Failed to allocate descriptor set");
    }

    return core::Result<VkDescriptorSet>::success(descriptorSet);
}

core::Result<std::vector<VkDescriptorSet>>
DescriptorPool::allocateMultiple(const DescriptorSetLayout& layout, uint32_t count) {
    if (count == 0) {
        AXIOM_LOG_ERROR("VkDescriptor", "Cannot allocate 0 descriptor sets");
        return core::Result<std::vector<VkDescriptorSet>>::failure(
            core::ErrorCode::InvalidParameter, "Count must be greater than 0");
    }

    // Create array of identical layouts
    std::vector<VkDescriptorSetLayout> layouts(count, layout.get());

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool_;
    allocInfo.descriptorSetCount = count;
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> descriptorSets(count);
    VkResult result = vkAllocateDescriptorSets(context_->getDevice(), &allocInfo,
                                               descriptorSets.data());
    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("VkDescriptor", "Failed to allocate %u descriptor sets (VkResult: %d)",
                        count, static_cast<int>(result));
        return core::Result<std::vector<VkDescriptorSet>>::failure(
            core::ErrorCode::GPU_OPERATION_FAILED, "Failed to allocate descriptor sets");
    }

    return core::Result<std::vector<VkDescriptorSet>>::success(std::move(descriptorSets));
}

void DescriptorPool::reset() {
    VkResult result = vkResetDescriptorPool(context_->getDevice(), pool_, 0);
    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("VkDescriptor", "Failed to reset descriptor pool (VkResult: %d)",
                        static_cast<int>(result));
    } else {
        AXIOM_LOG_DEBUG("VkDescriptor", "Descriptor pool reset successfully");
    }
}

// ========================================
// DescriptorSet implementation
// ========================================

DescriptorSet::DescriptorSet(VkContext* context, VkDescriptorSet set)
    : context_(context), set_(set) {}

void DescriptorSet::bindBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize offset,
                               VkDeviceSize range, VkDescriptorType descriptorType) {
    // Store buffer info (must persist until update() is called)
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = offset;
    bufferInfo.range = range;
    bufferInfos_.push_back(bufferInfo);

    // Create write descriptor set
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set_;
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = descriptorType;
    write.pBufferInfo = &bufferInfos_.back();

    pendingWrites_.push_back(write);
}

void DescriptorSet::bindImage(uint32_t binding, VkImageView imageView, VkSampler sampler,
                              VkImageLayout layout) {
    // Store image info (must persist until update() is called)
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;
    imageInfo.imageLayout = layout;
    imageInfos_.push_back(imageInfo);

    // Create write descriptor set
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set_;
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfos_.back();

    pendingWrites_.push_back(write);
}

void DescriptorSet::bindStorageImage(uint32_t binding, VkImageView imageView,
                                     VkImageLayout layout) {
    // Store image info (must persist until update() is called)
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = imageView;
    imageInfo.sampler = VK_NULL_HANDLE;  // Storage images don't use samplers
    imageInfo.imageLayout = layout;
    imageInfos_.push_back(imageInfo);

    // Create write descriptor set
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set_;
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.pImageInfo = &imageInfos_.back();

    pendingWrites_.push_back(write);
}

void DescriptorSet::update() {
    if (pendingWrites_.empty()) {
        AXIOM_LOG_DEBUG("VkDescriptor", "update() called with no pending descriptor writes");
        return;
    }

    // Fix pointers in pending writes (vectors may have reallocated)
    size_t bufferIndex = 0;
    size_t imageIndex = 0;
    for (auto& write : pendingWrites_) {
        if (write.pBufferInfo != nullptr) {
            write.pBufferInfo = &bufferInfos_[bufferIndex++];
        }
        if (write.pImageInfo != nullptr) {
            write.pImageInfo = &imageInfos_[imageIndex++];
        }
    }

    // Apply all pending writes at once
    vkUpdateDescriptorSets(context_->getDevice(), static_cast<uint32_t>(pendingWrites_.size()),
                           pendingWrites_.data(), 0, nullptr);

    AXIOM_LOG_DEBUG("VkDescriptor", "Updated descriptor set with %zu writes",
                    pendingWrites_.size());

    // Clear pending writes and info storage
    pendingWrites_.clear();
    bufferInfos_.clear();
    imageInfos_.clear();
}

}  // namespace axiom::gpu
