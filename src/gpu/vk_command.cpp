#include "axiom/gpu/vk_command.hpp"

#include "axiom/core/assert.hpp"
#include "axiom/core/logger.hpp"
#include "axiom/gpu/vk_instance.hpp"

#include <cstring>

namespace axiom::gpu {

namespace {

// Helper function to create a fence for synchronization
VkFence createFence(VkDevice device) {
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0;  // No flags - fence starts unsignaled

    VkFence fence = VK_NULL_HANDLE;
    VkResult result = vkCreateFence(device, &fenceInfo, nullptr, &fence);
    if (result != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    return fence;
}

}  // namespace

//==============================================================================
// CommandPool Implementation
//==============================================================================

CommandPool::CommandPool(VkContext* context, uint32_t queueFamily, VkCommandPoolCreateFlags flags)
    : context_(context), pool_(VK_NULL_HANDLE), queueFamily_(queueFamily) {
    AXIOM_ASSERT(context != nullptr, "Context cannot be null");

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = flags;
    poolInfo.queueFamilyIndex = queueFamily;

    VkResult result = vkCreateCommandPool(context_->getDevice(), &poolInfo, nullptr, &pool_);

    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("GPU", "Failed to create command pool: VkResult=%d", result);
        pool_ = VK_NULL_HANDLE;
    }
}

CommandPool::~CommandPool() {
    if (pool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(context_->getDevice(), pool_, nullptr);
    }
}

VkCommandBuffer CommandPool::allocate(VkCommandBufferLevel level) {
    if (pool_ == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool_;
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkResult result = vkAllocateCommandBuffers(context_->getDevice(), &allocInfo, &commandBuffer);

    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("GPU", "Failed to allocate command buffer: VkResult=%d", result);
        return VK_NULL_HANDLE;
    }

    return commandBuffer;
}

std::vector<VkCommandBuffer> CommandPool::allocateMultiple(uint32_t count,
                                                           VkCommandBufferLevel level) {
    if (pool_ == VK_NULL_HANDLE || count == 0) {
        return {};
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool_;
    allocInfo.level = level;
    allocInfo.commandBufferCount = count;

    std::vector<VkCommandBuffer> commandBuffers(count);
    VkResult result = vkAllocateCommandBuffers(context_->getDevice(), &allocInfo,
                                               commandBuffers.data());

    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("GPU", "Failed to allocate command buffers: VkResult=%d", result);
        return {};
    }

    return commandBuffers;
}

void CommandPool::free(VkCommandBuffer buffer) {
    if (pool_ != VK_NULL_HANDLE && buffer != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(context_->getDevice(), pool_, 1, &buffer);
    }
}

void CommandPool::free(const std::vector<VkCommandBuffer>& buffers) {
    if (pool_ != VK_NULL_HANDLE && !buffers.empty()) {
        vkFreeCommandBuffers(context_->getDevice(), pool_, static_cast<uint32_t>(buffers.size()),
                             buffers.data());
    }
}

void CommandPool::reset(bool releaseResources) {
    if (pool_ != VK_NULL_HANDLE) {
        VkCommandPoolResetFlags flags = releaseResources
                                            ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT
                                            : 0;
        vkResetCommandPool(context_->getDevice(), pool_, flags);
    }
}

//==============================================================================
// CommandBuffer Implementation
//==============================================================================

CommandBuffer::CommandBuffer(VkContext* context, VkCommandBuffer buffer, uint32_t queueFamily)
    : context_(context), buffer_(buffer), queueFamily_(queueFamily) {
    AXIOM_ASSERT(context != nullptr, "Context cannot be null");
    AXIOM_ASSERT(buffer != VK_NULL_HANDLE, "Buffer cannot be null");
}

CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
    : context_(other.context_), buffer_(other.buffer_), queueFamily_(other.queueFamily_) {
    other.buffer_ = VK_NULL_HANDLE;
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other) noexcept {
    if (this != &other) {
        context_ = other.context_;
        buffer_ = other.buffer_;
        queueFamily_ = other.queueFamily_;
        other.buffer_ = VK_NULL_HANDLE;
    }
    return *this;
}

core::Result<void> CommandBuffer::begin(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;
    beginInfo.pInheritanceInfo = nullptr;  // Only needed for secondary command buffers

    VkResult result = vkBeginCommandBuffer(buffer_, &beginInfo);
    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to begin command buffer recording");
    }

    return core::Result<void>::success();
}

core::Result<void> CommandBuffer::end() {
    VkResult result = vkEndCommandBuffer(buffer_);
    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to end command buffer recording");
    }

    return core::Result<void>::success();
}

core::Result<void> CommandBuffer::submit(VkQueue queue, const SubmitInfo& info) {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Wait semaphores
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(info.waitSemaphores.size());
    submitInfo.pWaitSemaphores = info.waitSemaphores.empty() ? nullptr : info.waitSemaphores.data();
    submitInfo.pWaitDstStageMask = info.waitStages.empty() ? nullptr : info.waitStages.data();

    // Command buffer
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer_;

    // Signal semaphores
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(info.signalSemaphores.size());
    submitInfo.pSignalSemaphores = info.signalSemaphores.empty() ? nullptr
                                                                 : info.signalSemaphores.data();

    VkResult result = vkQueueSubmit(queue, 1, &submitInfo, info.fence);
    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to submit command buffer");
    }

    return core::Result<void>::success();
}

core::Result<void> CommandBuffer::submitAndWait(VkQueue queue) {
    // Create a fence for synchronization
    VkFence fence = createFence(context_->getDevice());
    if (fence == VK_NULL_HANDLE) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to create fence for command buffer submission");
    }

    // Submit with fence
    SubmitInfo info{};
    info.fence = fence;
    auto submitResult = submit(queue, info);

    if (submitResult.isFailure()) {
        vkDestroyFence(context_->getDevice(), fence, nullptr);
        return submitResult;
    }

    // Wait for fence
    VkResult result = vkWaitForFences(context_->getDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(context_->getDevice(), fence, nullptr);

    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to wait for command buffer completion");
    }

    return core::Result<void>::success();
}

core::Result<void> CommandBuffer::reset(bool releaseResources) {
    VkCommandBufferResetFlags flags = releaseResources
                                          ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT
                                          : 0;

    VkResult result = vkResetCommandBuffer(buffer_, flags);
    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to reset command buffer");
    }

    return core::Result<void>::success();
}

//==============================================================================
// OneTimeCommand Implementation
//==============================================================================

OneTimeCommand::OneTimeCommand(VkContext* context, VkQueue queue, uint32_t queueFamily)
    : context_(context), queue_(queue), pool_(VK_NULL_HANDLE), buffer_(VK_NULL_HANDLE) {
    AXIOM_ASSERT(context != nullptr, "Context cannot be null");
    AXIOM_ASSERT(queue != VK_NULL_HANDLE, "Queue cannot be null");

    // Create transient command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;  // Hint for short-lived buffers
    poolInfo.queueFamilyIndex = queueFamily;

    VkResult result = vkCreateCommandPool(context_->getDevice(), &poolInfo, nullptr, &pool_);
    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("GPU", "Failed to create transient command pool: VkResult=%d", result);
        return;
    }

    // Allocate command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(context_->getDevice(), &allocInfo, &buffer_);
    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("GPU", "Failed to allocate one-time command buffer: VkResult=%d", result);
        vkDestroyCommandPool(context_->getDevice(), pool_, nullptr);
        pool_ = VK_NULL_HANDLE;
        return;
    }

    // Begin recording immediately
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(buffer_, &beginInfo);
    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("GPU", "Failed to begin one-time command buffer: VkResult=%d", result);
        vkDestroyCommandPool(context_->getDevice(), pool_, nullptr);
        pool_ = VK_NULL_HANDLE;
        buffer_ = VK_NULL_HANDLE;
    }
}

OneTimeCommand::~OneTimeCommand() {
    if (buffer_ != VK_NULL_HANDLE) {
        // End recording
        VkResult result = vkEndCommandBuffer(buffer_);
        if (result != VK_SUCCESS) {
            AXIOM_LOG_ERROR("GPU", "Failed to end one-time command buffer: VkResult=%d", result);
        } else {
            // Submit and wait
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &buffer_;

            result = vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
            if (result != VK_SUCCESS) {
                AXIOM_LOG_ERROR("GPU", "Failed to submit one-time command buffer: VkResult=%d",
                                result);
            } else {
                // Wait for queue to finish
                result = vkQueueWaitIdle(queue_);
                if (result != VK_SUCCESS) {
                    AXIOM_LOG_ERROR("GPU", "Failed to wait for queue idle: VkResult=%d", result);
                }
            }
        }
    }

    // Cleanup resources
    if (pool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(context_->getDevice(), pool_, nullptr);
    }
}

}  // namespace axiom::gpu
