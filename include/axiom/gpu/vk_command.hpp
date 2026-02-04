#pragma once

#include "axiom/core/result.hpp"

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace axiom::gpu {

// Forward declaration
class VkContext;

/// Vulkan command pool for allocating command buffers
/// Command pools are NOT thread-safe and should be used from a single thread.
/// For multi-threaded command recording, create separate pools per thread.
///
/// Example usage:
/// @code
/// CommandPool pool(context, context->getComputeQueueFamily());
/// VkCommandBuffer cmdBuf = pool.allocate();
/// // Use command buffer...
/// pool.free(cmdBuf);
/// @endcode
class CommandPool {
public:
    /// Create a command pool for a specific queue family
    /// @param context Valid VkContext pointer (must outlive this pool)
    /// @param queueFamily Queue family index for this pool
    /// @param flags Creation flags (e.g., VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)
    CommandPool(VkContext* context, uint32_t queueFamily, VkCommandPoolCreateFlags flags = 0);

    /// Destructor - frees all allocated command buffers and destroys the pool
    ~CommandPool();

    // Non-copyable, non-movable
    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;
    CommandPool(CommandPool&&) = delete;
    CommandPool& operator=(CommandPool&&) = delete;

    /// Allocate a single command buffer
    /// @param level Command buffer level (PRIMARY or SECONDARY)
    /// @return Command buffer handle or VK_NULL_HANDLE on failure
    VkCommandBuffer allocate(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    /// Allocate multiple command buffers at once
    /// @param count Number of command buffers to allocate
    /// @param level Command buffer level (PRIMARY or SECONDARY)
    /// @return Vector of command buffer handles (empty on failure)
    std::vector<VkCommandBuffer>
    allocateMultiple(uint32_t count, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    /// Free a single command buffer
    /// @param buffer Command buffer to free
    void free(VkCommandBuffer buffer);

    /// Free multiple command buffers at once
    /// @param buffers Vector of command buffers to free
    void free(const std::vector<VkCommandBuffer>& buffers);

    /// Reset the entire command pool (invalidates all allocated command buffers)
    /// This is more efficient than resetting individual command buffers.
    /// @param releaseResources If true, return memory to the system
    void reset(bool releaseResources = false);

    /// Get the underlying VkCommandPool handle
    /// @return Vulkan command pool handle
    VkCommandPool get() const noexcept { return pool_; }

    /// Implicit conversion to VkCommandPool for direct use with Vulkan API
    operator VkCommandPool() const noexcept { return pool_; }

    /// Get the queue family index this pool is associated with
    /// @return Queue family index
    uint32_t getQueueFamily() const noexcept { return queueFamily_; }

private:
    VkContext* context_;    ///< Vulkan context (not owned)
    VkCommandPool pool_;    ///< Vulkan command pool handle
    uint32_t queueFamily_;  ///< Queue family index
};

/// High-level command buffer wrapper with recording and submission utilities
/// This class provides a convenient interface for recording and submitting commands.
///
/// Example usage:
/// @code
/// CommandBuffer cmd(context, cmdBuf, queueFamily);
/// cmd.begin();
/// vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
/// vkCmdDispatch(cmd, groupX, groupY, groupZ);
/// cmd.end();
/// cmd.submitAndWait(queue);
/// @endcode
class CommandBuffer {
public:
    /// Create a command buffer wrapper
    /// @param context Valid VkContext pointer (must outlive this wrapper)
    /// @param buffer Command buffer handle (must be valid)
    /// @param queueFamily Queue family index for submission
    CommandBuffer(VkContext* context, VkCommandBuffer buffer, uint32_t queueFamily);

    // Allow move operations
    CommandBuffer(CommandBuffer&& other) noexcept;
    CommandBuffer& operator=(CommandBuffer&& other) noexcept;

    // Non-copyable
    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

    /// Begin recording commands
    /// @param flags Usage flags (e.g., VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)
    /// @return Result indicating success or failure
    core::Result<void> begin(VkCommandBufferUsageFlags flags = 0);

    /// End recording commands
    /// @return Result indicating success or failure
    core::Result<void> end();

    /// Submission information for command buffer
    struct SubmitInfo {
        std::vector<VkSemaphore> waitSemaphores;       ///< Semaphores to wait on
        std::vector<VkPipelineStageFlags> waitStages;  ///< Pipeline stages to wait at
        std::vector<VkSemaphore> signalSemaphores;     ///< Semaphores to signal
        VkFence fence;                                 ///< Optional fence to signal (defaults to VK_NULL_HANDLE)
    };

    /// Submit command buffer to a queue
    /// @param queue Queue to submit to
    /// @param info Optional submission synchronization info
    /// @return Result indicating success or failure
    core::Result<void> submit(VkQueue queue, const SubmitInfo& info = {});

    /// Submit command buffer and wait for completion
    /// This is a convenience function that creates a fence, submits, and waits.
    /// @param queue Queue to submit to
    /// @return Result indicating success or failure
    core::Result<void> submitAndWait(VkQueue queue);

    /// Reset the command buffer for re-recording
    /// Note: The command pool must have been created with RESET_COMMAND_BUFFER_BIT
    /// @param releaseResources If true, return memory to the pool
    /// @return Result indicating success or failure
    core::Result<void> reset(bool releaseResources = false);

    /// Get the underlying VkCommandBuffer handle
    /// @return Vulkan command buffer handle
    VkCommandBuffer get() const noexcept { return buffer_; }

    /// Implicit conversion to VkCommandBuffer for direct use with Vulkan API
    operator VkCommandBuffer() const noexcept { return buffer_; }

    /// Get the queue family index
    /// @return Queue family index
    uint32_t getQueueFamily() const noexcept { return queueFamily_; }

private:
    VkContext* context_;      ///< Vulkan context (not owned)
    VkCommandBuffer buffer_;  ///< Command buffer handle (not owned, managed by pool)
    uint32_t queueFamily_;    ///< Queue family index
};

/// RAII wrapper for one-time command buffers
/// Automatically allocates, begins, ends, submits, and waits on destruction.
/// Perfect for one-off operations like buffer uploads or image layout transitions.
///
/// Example usage:
/// @code
/// {
///     OneTimeCommand cmd(context, context->getTransferQueue());
///     VkBufferCopy region{};
///     region.size = size;
///     vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &region);
/// }  // Command automatically submitted and waited on destruction
/// @endcode
class OneTimeCommand {
public:
    /// Create a one-time command buffer
    /// Automatically creates a transient command pool, allocates a buffer, and begins recording.
    /// @param context Valid VkContext pointer
    /// @param queue Queue to submit to on destruction
    /// @param queueFamily Queue family index (defaults to 0)
    OneTimeCommand(VkContext* context, VkQueue queue, uint32_t queueFamily);

    /// Destructor - ends recording, submits, and waits for completion
    /// Errors during submission are logged but not propagated (destructor cannot throw)
    ~OneTimeCommand();

    // Non-copyable, non-movable (RAII wrapper)
    OneTimeCommand(const OneTimeCommand&) = delete;
    OneTimeCommand& operator=(const OneTimeCommand&) = delete;
    OneTimeCommand(OneTimeCommand&&) = delete;
    OneTimeCommand& operator=(OneTimeCommand&&) = delete;

    /// Get the underlying VkCommandBuffer handle for recording
    /// @return Vulkan command buffer handle
    VkCommandBuffer get() const noexcept { return buffer_; }

    /// Implicit conversion to VkCommandBuffer for recording commands
    operator VkCommandBuffer() const noexcept { return buffer_; }

private:
    VkContext* context_;      ///< Vulkan context (not owned)
    VkQueue queue_;           ///< Queue for submission
    VkCommandPool pool_;      ///< Transient command pool
    VkCommandBuffer buffer_;  ///< Command buffer handle
};

}  // namespace axiom::gpu
