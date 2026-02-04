#pragma once

#include "axiom/core/result.hpp"

#include <cstdint>
#include <memory>
#include <vulkan/vulkan.h>

namespace axiom::gpu {

// Forward declaration
class VkContext;

/// Memory usage patterns for Vulkan resources
enum class MemoryUsage {
    GpuOnly,   ///< Device local memory (fastest for GPU, not accessible by CPU)
    CpuToGpu,  ///< Staging buffer for CPU-to-GPU transfers
    GpuToCpu,  ///< Readback buffer for GPU-to-CPU transfers
    CpuOnly,   ///< Host visible and coherent memory (accessible by CPU, slower for GPU)
};

/// Vulkan memory manager using VMA (Vulkan Memory Allocator)
/// This class provides a high-level interface for GPU memory allocation and management.
/// It wraps the VMA library to provide efficient suballocation, defragmentation,
/// and automatic memory type selection.
///
/// Features:
/// - Automatic memory type selection based on usage patterns
/// - Efficient suballocation from large memory blocks
/// - Memory mapping support for CPU-accessible buffers
/// - Memory statistics and debugging information
/// - Proper cleanup and leak detection
///
/// Example usage:
/// @code
/// auto memManager = VkMemoryManager::create(context.get());
/// if (memManager.isSuccess()) {
///     auto& manager = memManager.value();
///
///     // Create GPU-only storage buffer
///     VkMemoryManager::BufferCreateInfo info{
///         .size = 1024 * 1024,
///         .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
///         .memoryUsage = MemoryUsage::GpuOnly
///     };
///
///     auto bufferResult = manager->createBuffer(info);
///     if (bufferResult.isSuccess()) {
///         auto buffer = bufferResult.value();
///         // Use buffer...
///         manager->destroyBuffer(buffer);
///     }
/// }
/// @endcode
class VkMemoryManager {
public:
    /// Buffer resource with associated memory allocation
    struct Buffer {
        VkBuffer buffer = VK_NULL_HANDLE;  ///< Vulkan buffer handle
        void* allocation = nullptr;        ///< VMA allocation handle (opaque)
        void* mappedPtr = nullptr;         ///< Persistently mapped pointer (if applicable)
    };

    /// Buffer creation parameters
    struct BufferCreateInfo {
        VkDeviceSize size;            ///< Size in bytes
        VkBufferUsageFlags usage;     ///< Buffer usage flags (e.g., STORAGE_BUFFER, TRANSFER_SRC)
        MemoryUsage memoryUsage;      ///< Memory usage pattern
        bool persistentMapping = false;  ///< Keep memory mapped after creation
    };

    /// Image resource with associated memory allocation
    struct Image {
        VkImage image = VK_NULL_HANDLE;  ///< Vulkan image handle
        void* allocation = nullptr;      ///< VMA allocation handle (opaque)
    };

    /// Image creation parameters
    struct ImageCreateInfo {
        VkExtent3D extent;                   ///< Image dimensions (width, height, depth)
        VkFormat format;                     ///< Pixel format
        VkImageUsageFlags usage;             ///< Image usage flags
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;  ///< Tiling mode
        uint32_t mipLevels = 1;              ///< Number of mipmap levels
        uint32_t arrayLayers = 1;            ///< Number of array layers
        MemoryUsage memoryUsage = MemoryUsage::GpuOnly;  ///< Memory usage pattern
    };

    /// Memory statistics
    struct MemoryStats {
        VkDeviceSize usedBytes = 0;       ///< Bytes currently in use
        VkDeviceSize allocatedBytes = 0;  ///< Total bytes allocated from device
        uint32_t allocationCount = 0;     ///< Number of allocations
        uint32_t blockCount = 0;          ///< Number of memory blocks
    };

    /// Create a memory manager instance
    /// @param context Valid VkContext pointer (must outlive this manager)
    /// @return Result containing the memory manager or error code
    static core::Result<std::unique_ptr<VkMemoryManager>> create(VkContext* context);

    /// Destructor - cleans up VMA allocator and verifies no leaks
    ~VkMemoryManager();

    // Non-copyable, non-movable
    VkMemoryManager(const VkMemoryManager&) = delete;
    VkMemoryManager& operator=(const VkMemoryManager&) = delete;
    VkMemoryManager(VkMemoryManager&&) = delete;
    VkMemoryManager& operator=(VkMemoryManager&&) = delete;

    /// Create a buffer with associated memory
    /// @param info Buffer creation parameters
    /// @return Result containing the buffer or error code
    core::Result<Buffer> createBuffer(const BufferCreateInfo& info);

    /// Destroy a buffer and free its memory
    /// @param buffer Buffer to destroy (handles will be set to VK_NULL_HANDLE after destruction)
    void destroyBuffer(Buffer& buffer);

    /// Create an image with associated memory
    /// @param info Image creation parameters
    /// @return Result containing the image or error code
    core::Result<Image> createImage(const ImageCreateInfo& info);

    /// Destroy an image and free its memory
    /// @param image Image to destroy (handles will be set to VK_NULL_HANDLE after destruction)
    void destroyImage(Image& image);

    /// Map buffer memory to CPU address space
    /// @param buffer Buffer to map (must be CPU-accessible)
    /// @return Result containing mapped pointer or error code
    core::Result<void*> mapMemory(const Buffer& buffer);

    /// Unmap buffer memory
    /// @param buffer Buffer to unmap
    void unmapMemory(const Buffer& buffer);

    /// Get memory usage statistics
    /// @return Current memory statistics
    MemoryStats getStats() const;

    /// Print detailed memory statistics to stdout
    void printStats() const;

    /// Get the VMA allocator handle (for advanced use cases)
    /// @return VMA allocator handle (opaque)
    void* getAllocator() const noexcept { return allocator_; }

private:
    /// Private constructor - use create() instead
    explicit VkMemoryManager(VkContext* context);

    /// Initialize VMA allocator
    /// @return Result indicating success or failure
    core::Result<void> initialize();

    /// Convert MemoryUsage enum to VMA memory usage flags
    /// @param usage Memory usage pattern
    /// @return VMA memory usage flags (int)
    int toVmaMemoryUsage(MemoryUsage usage) const noexcept;

    VkContext* context_;  ///< Vulkan context (not owned)
    void* allocator_;     ///< VMA allocator handle (opaque)
};

}  // namespace axiom::gpu
