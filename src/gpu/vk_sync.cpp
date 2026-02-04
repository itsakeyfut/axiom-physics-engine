#include "axiom/gpu/vk_sync.hpp"

#include "axiom/core/error_code.hpp"
#include "axiom/gpu/vk_instance.hpp"

#include <algorithm>
#include <utility>

namespace axiom::gpu {

// ============================================================================
// Fence Implementation
// ============================================================================

Fence::Fence(VkContext* context, bool signaled) : context_(context), fence_(VK_NULL_HANDLE) {
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VkResult result = vkCreateFence(context_->getDevice(), &fenceInfo, nullptr, &fence_);
    if (result != VK_SUCCESS) {
        // In constructor, we cannot return Result, so we create an unsignaled fence
        // Errors will be caught during usage
        fence_ = VK_NULL_HANDLE;
    }
}

Fence::~Fence() {
    if (fence_ != VK_NULL_HANDLE && context_ != nullptr) {
        vkDestroyFence(context_->getDevice(), fence_, nullptr);
    }
}

Fence::Fence(Fence&& other) noexcept : context_(other.context_), fence_(other.fence_) {
    other.context_ = nullptr;
    other.fence_ = VK_NULL_HANDLE;
}

Fence& Fence::operator=(Fence&& other) noexcept {
    if (this != &other) {
        // Clean up existing fence
        if (fence_ != VK_NULL_HANDLE && context_ != nullptr) {
            vkDestroyFence(context_->getDevice(), fence_, nullptr);
        }

        // Move from other
        context_ = other.context_;
        fence_ = other.fence_;

        // Reset other
        other.context_ = nullptr;
        other.fence_ = VK_NULL_HANDLE;
    }
    return *this;
}

core::Result<void> Fence::wait(uint64_t timeout) {
    if (fence_ == VK_NULL_HANDLE) {
        return core::Result<void>::failure(core::ErrorCode::GPU_INVALID_OPERATION,
                                           "Fence::wait: Fence is not initialized");
    }

    VkResult result = vkWaitForFences(context_->getDevice(), 1, &fence_, VK_TRUE, timeout);

    if (result == VK_TIMEOUT) {
        return core::Result<void>::failure(core::ErrorCode::GPU_TIMEOUT,
                                           "Fence::wait: Timeout waiting for fence");
    }

    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                           "Fence::wait: Failed to wait for fence");
    }

    return core::Result<void>::success();
}

core::Result<void> Fence::reset() {
    if (fence_ == VK_NULL_HANDLE) {
        return core::Result<void>::failure(core::ErrorCode::GPU_INVALID_OPERATION,
                                           "Fence::reset: Fence is not initialized");
    }

    VkResult result = vkResetFences(context_->getDevice(), 1, &fence_);

    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                           "Fence::reset: Failed to reset fence");
    }

    return core::Result<void>::success();
}

bool Fence::isSignaled() const {
    if (fence_ == VK_NULL_HANDLE) {
        return false;
    }

    VkResult result = vkGetFenceStatus(context_->getDevice(), fence_);
    return result == VK_SUCCESS;
}

// ============================================================================
// FencePool Implementation
// ============================================================================

FencePool::FencePool(VkContext* context) : context_(context) {}

FencePool::~FencePool() {
    // unique_ptr will automatically clean up all fences
    fences_.clear();
    availableFences_.clear();
}

Fence* FencePool::acquire() {
    // Try to reuse an available fence
    if (!availableFences_.empty()) {
        Fence* fence = availableFences_.back();
        availableFences_.pop_back();

        // Reset the fence before returning
        fence->reset();

        return fence;
    }

    // No available fences, create a new one
    auto fence = std::make_unique<Fence>(context_, false);
    Fence* fencePtr = fence.get();
    fences_.push_back(std::move(fence));

    return fencePtr;
}

void FencePool::release(Fence* fence) {
    if (fence == nullptr) {
        return;
    }

    // Verify the fence belongs to this pool
    auto it = std::find_if(
        fences_.begin(), fences_.end(),
        [fence](const std::unique_ptr<Fence>& ptr) { return ptr.get() == fence; });

    if (it == fences_.end()) {
        // Fence doesn't belong to this pool, ignore
        return;
    }

    // Reset the fence and add to available list
    fence->reset();
    availableFences_.push_back(fence);
}

// ============================================================================
// Semaphore Implementation
// ============================================================================

Semaphore::Semaphore(VkContext* context) : context_(context), semaphore_(VK_NULL_HANDLE) {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkResult result = vkCreateSemaphore(context_->getDevice(), &semaphoreInfo, nullptr,
                                        &semaphore_);

    if (result != VK_SUCCESS) {
        semaphore_ = VK_NULL_HANDLE;
    }
}

Semaphore::~Semaphore() {
    if (semaphore_ != VK_NULL_HANDLE && context_ != nullptr) {
        vkDestroySemaphore(context_->getDevice(), semaphore_, nullptr);
    }
}

Semaphore::Semaphore(Semaphore&& other) noexcept
    : context_(other.context_), semaphore_(other.semaphore_) {
    other.context_ = nullptr;
    other.semaphore_ = VK_NULL_HANDLE;
}

Semaphore& Semaphore::operator=(Semaphore&& other) noexcept {
    if (this != &other) {
        // Clean up existing semaphore
        if (semaphore_ != VK_NULL_HANDLE && context_ != nullptr) {
            vkDestroySemaphore(context_->getDevice(), semaphore_, nullptr);
        }

        // Move from other
        context_ = other.context_;
        semaphore_ = other.semaphore_;

        // Reset other
        other.context_ = nullptr;
        other.semaphore_ = VK_NULL_HANDLE;
    }
    return *this;
}

// ============================================================================
// TimelineSemaphore Implementation
// ============================================================================

TimelineSemaphore::TimelineSemaphore(VkContext* context, uint64_t initialValue)
    : context_(context), semaphore_(VK_NULL_HANDLE) {
    VkSemaphoreTypeCreateInfo timelineInfo{};
    timelineInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timelineInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineInfo.initialValue = initialValue;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = &timelineInfo;

    VkResult result = vkCreateSemaphore(context_->getDevice(), &semaphoreInfo, nullptr,
                                        &semaphore_);

    if (result != VK_SUCCESS) {
        semaphore_ = VK_NULL_HANDLE;
    }
}

TimelineSemaphore::~TimelineSemaphore() {
    if (semaphore_ != VK_NULL_HANDLE && context_ != nullptr) {
        vkDestroySemaphore(context_->getDevice(), semaphore_, nullptr);
    }
}

TimelineSemaphore::TimelineSemaphore(TimelineSemaphore&& other) noexcept
    : context_(other.context_), semaphore_(other.semaphore_) {
    other.context_ = nullptr;
    other.semaphore_ = VK_NULL_HANDLE;
}

TimelineSemaphore& TimelineSemaphore::operator=(TimelineSemaphore&& other) noexcept {
    if (this != &other) {
        // Clean up existing semaphore
        if (semaphore_ != VK_NULL_HANDLE && context_ != nullptr) {
            vkDestroySemaphore(context_->getDevice(), semaphore_, nullptr);
        }

        // Move from other
        context_ = other.context_;
        semaphore_ = other.semaphore_;

        // Reset other
        other.context_ = nullptr;
        other.semaphore_ = VK_NULL_HANDLE;
    }
    return *this;
}

core::Result<void> TimelineSemaphore::signal(uint64_t value) {
    if (semaphore_ == VK_NULL_HANDLE) {
        return core::Result<void>::failure(
            core::ErrorCode::GPU_INVALID_OPERATION,
            "TimelineSemaphore::signal: Semaphore is not initialized");
    }

    VkSemaphoreSignalInfo signalInfo{};
    signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
    signalInfo.semaphore = semaphore_;
    signalInfo.value = value;

    VkResult result = vkSignalSemaphore(context_->getDevice(), &signalInfo);

    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                           "TimelineSemaphore::signal: Failed to signal semaphore");
    }

    return core::Result<void>::success();
}

core::Result<void> TimelineSemaphore::wait(uint64_t value, uint64_t timeout) {
    if (semaphore_ == VK_NULL_HANDLE) {
        return core::Result<void>::failure(core::ErrorCode::GPU_INVALID_OPERATION,
                                           "TimelineSemaphore::wait: Semaphore is not initialized");
    }

    VkSemaphoreWaitInfo waitInfo{};
    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    waitInfo.semaphoreCount = 1;
    waitInfo.pSemaphores = &semaphore_;
    waitInfo.pValues = &value;

    VkResult result = vkWaitSemaphores(context_->getDevice(), &waitInfo, timeout);

    if (result == VK_TIMEOUT) {
        return core::Result<void>::failure(
            core::ErrorCode::GPU_TIMEOUT, "TimelineSemaphore::wait: Timeout waiting for semaphore");
    }

    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                           "TimelineSemaphore::wait: Failed to wait for semaphore");
    }

    return core::Result<void>::success();
}

uint64_t TimelineSemaphore::getValue() const {
    if (semaphore_ == VK_NULL_HANDLE) {
        return 0;
    }

    uint64_t value = 0;
    vkGetSemaphoreCounterValue(context_->getDevice(), semaphore_, &value);
    return value;
}

// ============================================================================
// Pipeline Barrier Utility Functions
// ============================================================================

void imageBarrier(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout,
                  VkImageLayout newLayout, VkPipelineStageFlags srcStage,
                  VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                  VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t levelCount,
                  uint32_t baseArrayLayer, uint32_t layerCount) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = baseMipLevel;
    barrier.subresourceRange.levelCount = levelCount;
    barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
    barrier.subresourceRange.layerCount = layerCount;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;

    vkCmdPipelineBarrier(cmd, srcStage, dstStage,
                         0,           // dependency flags
                         0, nullptr,  // memory barriers
                         0, nullptr,  // buffer barriers
                         1, &barrier  // image barriers
    );
}

void bufferBarrier(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size,
                   VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                   VkAccessFlags srcAccess, VkAccessFlags dstAccess, uint32_t srcQueueFamily,
                   uint32_t dstQueueFamily) {
    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;
    barrier.srcQueueFamilyIndex = srcQueueFamily;
    barrier.dstQueueFamilyIndex = dstQueueFamily;
    barrier.buffer = buffer;
    barrier.offset = offset;
    barrier.size = size;

    vkCmdPipelineBarrier(cmd, srcStage, dstStage,
                         0,            // dependency flags
                         0, nullptr,   // memory barriers
                         1, &barrier,  // buffer barriers
                         0, nullptr    // image barriers
    );
}

void memoryBarrier(VkCommandBuffer cmd, VkPipelineStageFlags srcStage,
                   VkPipelineStageFlags dstStage, VkAccessFlags srcAccess,
                   VkAccessFlags dstAccess) {
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;

    vkCmdPipelineBarrier(cmd, srcStage, dstStage,
                         0,            // dependency flags
                         1, &barrier,  // memory barriers
                         0, nullptr,   // buffer barriers
                         0, nullptr    // image barriers
    );
}

}  // namespace axiom::gpu
