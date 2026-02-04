#pragma once

#include "axiom/core/result.hpp"

#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace axiom::gpu {

// Forward declaration
class VkContext;

/// Fence for GPU-CPU synchronization
/// Fences are used to synchronize the CPU with GPU operations. The CPU can wait
/// on a fence to ensure GPU work has completed before proceeding.
///
/// Example usage:
/// @code
/// Fence fence(context);
/// // Submit command buffer with fence
/// vkQueueSubmit(queue, 1, &submitInfo, fence.get());
/// fence.wait();  // Wait for GPU to finish
/// @endcode
class Fence {
public:
    /// Create a fence
    /// @param context Valid VkContext pointer (must outlive this fence)
    /// @param signaled If true, fence is created in signaled state (already completed)
    Fence(VkContext* context, bool signaled = false);

    /// Destructor - destroys the fence
    ~Fence();

    // Non-copyable, movable
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;
    Fence(Fence&& other) noexcept;
    Fence& operator=(Fence&& other) noexcept;

    /// Wait for the fence to be signaled
    /// Blocks the calling thread until the fence becomes signaled or timeout occurs.
    /// @param timeout Timeout in nanoseconds (UINT64_MAX = infinite)
    /// @return Result indicating success or timeout
    core::Result<void> wait(uint64_t timeout = UINT64_MAX);

    /// Reset the fence to unsignaled state
    /// Must be called before the fence can be reused for another submit.
    /// @return Result indicating success or failure
    core::Result<void> reset();

    /// Check if the fence is currently signaled
    /// This is a non-blocking query.
    /// @return true if fence is signaled, false otherwise
    bool isSignaled() const;

    /// Get the underlying VkFence handle
    /// @return Vulkan fence handle
    VkFence get() const noexcept { return fence_; }

    /// Implicit conversion to VkFence for direct use with Vulkan API
    operator VkFence() const noexcept { return fence_; }

private:
    VkContext* context_;  ///< Vulkan context (not owned)
    VkFence fence_;       ///< Vulkan fence handle
};

/// Pool of reusable fences
/// Manages a collection of fences that can be acquired and released for reuse.
/// This reduces the overhead of creating and destroying fences frequently.
///
/// Example usage:
/// @code
/// FencePool pool(context);
/// Fence* fence = pool.acquire();
/// // Use fence...
/// fence->wait();
/// pool.release(fence);  // Return to pool for reuse
/// @endcode
class FencePool {
public:
    /// Create a fence pool
    /// @param context Valid VkContext pointer (must outlive this pool)
    explicit FencePool(VkContext* context);

    /// Destructor - destroys all fences in the pool
    ~FencePool();

    // Non-copyable, non-movable
    FencePool(const FencePool&) = delete;
    FencePool& operator=(const FencePool&) = delete;
    FencePool(FencePool&&) = delete;
    FencePool& operator=(FencePool&&) = delete;

    /// Acquire a fence from the pool
    /// If no fences are available, a new one is created.
    /// The acquired fence is guaranteed to be in reset (unsignaled) state.
    /// @return Pointer to a fence (never null)
    Fence* acquire();

    /// Release a fence back to the pool
    /// The fence is automatically reset and made available for reuse.
    /// @param fence Fence to release (must have been acquired from this pool)
    void release(Fence* fence);

    /// Get the total number of fences in the pool (available + in-use)
    /// @return Total fence count
    size_t getTotalCount() const noexcept { return fences_.size(); }

    /// Get the number of available fences
    /// @return Available fence count
    size_t getAvailableCount() const noexcept { return availableFences_.size(); }

private:
    VkContext* context_;                          ///< Vulkan context (not owned)
    std::vector<std::unique_ptr<Fence>> fences_;  ///< All fences owned by pool
    std::vector<Fence*> availableFences_;         ///< Available fences for reuse
};

/// Binary semaphore for GPU-GPU synchronization
/// Semaphores coordinate operations between different queues on the GPU.
/// They are used in queue submissions to signal when work is complete and
/// wait for dependencies before starting new work.
///
/// Example usage:
/// @code
/// Semaphore semaphore(context);
/// // Transfer queue signals semaphore
/// vkQueueSubmit(transferQueue, 1, &transferSubmit, VK_NULL_HANDLE);
/// // Compute queue waits on semaphore
/// vkQueueSubmit(computeQueue, 1, &computeSubmit, VK_NULL_HANDLE);
/// @endcode
class Semaphore {
public:
    /// Create a binary semaphore
    /// @param context Valid VkContext pointer (must outlive this semaphore)
    explicit Semaphore(VkContext* context);

    /// Destructor - destroys the semaphore
    ~Semaphore();

    // Non-copyable, movable
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;
    Semaphore(Semaphore&& other) noexcept;
    Semaphore& operator=(Semaphore&& other) noexcept;

    /// Get the underlying VkSemaphore handle
    /// @return Vulkan semaphore handle
    VkSemaphore get() const noexcept { return semaphore_; }

    /// Implicit conversion to VkSemaphore for direct use with Vulkan API
    operator VkSemaphore() const noexcept { return semaphore_; }

private:
    VkContext* context_;     ///< Vulkan context (not owned)
    VkSemaphore semaphore_;  ///< Vulkan semaphore handle
};

/// Timeline semaphore for value-based synchronization (Vulkan 1.2+)
/// Timeline semaphores maintain a monotonically increasing counter value.
/// They allow more flexible synchronization patterns compared to binary semaphores,
/// such as waiting for specific operations to complete or signaling completion of work.
///
/// Example usage:
/// @code
/// TimelineSemaphore semaphore(context, 0);
/// // Signal value 1 when first operation completes
/// vkQueueSubmit(...);
/// semaphore.wait(1);  // Wait for value to reach 1
/// // Signal value 2 when second operation completes
/// vkQueueSubmit(...);
/// uint64_t currentValue = semaphore.getValue();
/// @endcode
class TimelineSemaphore {
public:
    /// Create a timeline semaphore
    /// @param context Valid VkContext pointer (must outlive this semaphore)
    /// @param initialValue Initial counter value (defaults to 0)
    TimelineSemaphore(VkContext* context, uint64_t initialValue = 0);

    /// Destructor - destroys the semaphore
    ~TimelineSemaphore();

    // Non-copyable, movable
    TimelineSemaphore(const TimelineSemaphore&) = delete;
    TimelineSemaphore& operator=(const TimelineSemaphore&) = delete;
    TimelineSemaphore(TimelineSemaphore&& other) noexcept;
    TimelineSemaphore& operator=(TimelineSemaphore&& other) noexcept;

    /// Signal the semaphore with a new value
    /// The value must be greater than the current semaphore value.
    /// This is a host (CPU) signal operation.
    /// @param value New counter value
    /// @return Result indicating success or failure
    core::Result<void> signal(uint64_t value);

    /// Wait for the semaphore to reach or exceed a value
    /// Blocks the calling thread until the semaphore value reaches the specified value.
    /// @param value Value to wait for
    /// @param timeout Timeout in nanoseconds (UINT64_MAX = infinite)
    /// @return Result indicating success or timeout
    core::Result<void> wait(uint64_t value, uint64_t timeout = UINT64_MAX);

    /// Get the current semaphore counter value
    /// This is a non-blocking query.
    /// @return Current counter value
    uint64_t getValue() const;

    /// Get the underlying VkSemaphore handle
    /// @return Vulkan semaphore handle
    VkSemaphore get() const noexcept { return semaphore_; }

    /// Implicit conversion to VkSemaphore for direct use with Vulkan API
    operator VkSemaphore() const noexcept { return semaphore_; }

private:
    VkContext* context_;     ///< Vulkan context (not owned)
    VkSemaphore semaphore_;  ///< Vulkan timeline semaphore handle
};

/// Insert an image memory barrier into a command buffer
/// Image barriers are used to:
/// - Transition image layouts (e.g., UNDEFINED -> GENERAL)
/// - Synchronize memory access between different pipeline stages
/// - Transfer ownership between queue families
///
/// Example usage:
/// @code
/// imageBarrier(cmdBuf, image,
///     VK_IMAGE_LAYOUT_UNDEFINED,
///     VK_IMAGE_LAYOUT_GENERAL,
///     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
///     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
///     0,
///     VK_ACCESS_SHADER_WRITE_BIT);
/// @endcode
///
/// @param cmd Command buffer to record the barrier into
/// @param image Image to apply barrier to
/// @param oldLayout Current image layout
/// @param newLayout Target image layout
/// @param srcStage Source pipeline stage (what needs to finish before)
/// @param dstStage Destination pipeline stage (what waits for the barrier)
/// @param srcAccess Source access mask (how memory was accessed before)
/// @param dstAccess Destination access mask (how memory will be accessed after)
/// @param aspectMask Image aspect mask (defaults to COLOR)
/// @param baseMipLevel Base mip level (defaults to 0)
/// @param levelCount Number of mip levels (defaults to 1)
/// @param baseArrayLayer Base array layer (defaults to 0)
/// @param layerCount Number of array layers (defaults to 1)
void imageBarrier(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout,
                  VkImageLayout newLayout, VkPipelineStageFlags srcStage,
                  VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                  VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                  uint32_t baseMipLevel = 0, uint32_t levelCount = 1, uint32_t baseArrayLayer = 0,
                  uint32_t layerCount = 1);

/// Insert a buffer memory barrier into a command buffer
/// Buffer barriers synchronize memory access to buffer regions between
/// different pipeline stages or transfer ownership between queue families.
///
/// Example usage:
/// @code
/// bufferBarrier(cmdBuf, buffer, 0, VK_WHOLE_SIZE,
///     VK_PIPELINE_STAGE_TRANSFER_BIT,
///     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
///     VK_ACCESS_TRANSFER_WRITE_BIT,
///     VK_ACCESS_SHADER_READ_BIT);
/// @endcode
///
/// @param cmd Command buffer to record the barrier into
/// @param buffer Buffer to apply barrier to
/// @param offset Byte offset into the buffer
/// @param size Size of the memory range in bytes (VK_WHOLE_SIZE for entire buffer)
/// @param srcStage Source pipeline stage
/// @param dstStage Destination pipeline stage
/// @param srcAccess Source access mask
/// @param dstAccess Destination access mask
/// @param srcQueueFamily Source queue family (defaults to VK_QUEUE_FAMILY_IGNORED)
/// @param dstQueueFamily Destination queue family (defaults to VK_QUEUE_FAMILY_IGNORED)
void bufferBarrier(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size,
                   VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                   VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                   uint32_t srcQueueFamily = VK_QUEUE_FAMILY_IGNORED,
                   uint32_t dstQueueFamily = VK_QUEUE_FAMILY_IGNORED);

/// Insert a generic memory barrier into a command buffer
/// Memory barriers provide global memory synchronization without being tied
/// to specific resources. They ensure all memory operations of specified types
/// are visible between pipeline stages.
///
/// Example usage:
/// @code
/// memoryBarrier(cmdBuf,
///     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
///     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
///     VK_ACCESS_SHADER_WRITE_BIT,
///     VK_ACCESS_SHADER_READ_BIT);
/// @endcode
///
/// @param cmd Command buffer to record the barrier into
/// @param srcStage Source pipeline stage
/// @param dstStage Destination pipeline stage
/// @param srcAccess Source access mask
/// @param dstAccess Destination access mask
void memoryBarrier(VkCommandBuffer cmd, VkPipelineStageFlags srcStage,
                   VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess);

}  // namespace axiom::gpu
