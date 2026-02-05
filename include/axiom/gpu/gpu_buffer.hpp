#pragma once

#include "axiom/core/logger.hpp"
#include "axiom/core/result.hpp"
#include "axiom/gpu/vk_memory.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace axiom::gpu {

// Forward declarations
class VkContext;
class VkMemoryManager;

/// High-level GPU buffer abstraction for simplified buffer management
/// This class wraps VkMemoryManager to provide convenient buffer operations
/// including staging-based uploads/downloads, persistent mapping, and resizing.
///
/// Features:
/// - Automatic staging buffer management for CPU-GPU transfers
/// - Persistent mapping support for frequently updated buffers
/// - Type-safe typed buffer variants
/// - Resize support for dynamic buffers
///
/// Example usage:
/// @code
/// GpuBuffer storageBuffer(memManager, 1024 * 1024,
///                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
///                        MemoryUsage::GpuOnly);
///
/// std::vector<float> data(256);
/// storageBuffer.upload(data.data(), data.size() * sizeof(float));
/// @endcode
class GpuBuffer {
public:
    /// Create a GPU buffer with specified size and usage
    /// @param memManager Memory manager for allocation (must outlive this buffer)
    /// @param size Buffer size in bytes
    /// @param usage Buffer usage flags (e.g., STORAGE_BUFFER, VERTEX_BUFFER)
    /// @param memoryUsage Memory usage pattern (GpuOnly, CpuToGpu, etc.)
    GpuBuffer(VkMemoryManager* memManager, VkDeviceSize size, VkBufferUsageFlags usage,
              MemoryUsage memoryUsage);

    /// Destructor - automatically cleans up buffer and staging resources
    ~GpuBuffer();

    // Non-copyable (owns GPU resources)
    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;

    // Movable
    GpuBuffer(GpuBuffer&& other) noexcept;
    GpuBuffer& operator=(GpuBuffer&& other) noexcept;

    /// Upload data from CPU to GPU
    /// For GpuOnly buffers, this uses a staging buffer and command submission.
    /// For CPU-accessible buffers, this directly copies to mapped memory.
    /// @param data Pointer to source data
    /// @param size Size of data in bytes
    /// @param offset Offset in buffer to write to (in bytes)
    /// @return Result indicating success or failure
    core::Result<void> upload(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

    /// Download data from GPU to CPU
    /// For GpuOnly buffers, this uses a staging buffer and command submission.
    /// For CPU-accessible buffers, this directly copies from mapped memory.
    /// @param data Pointer to destination buffer
    /// @param size Size of data in bytes
    /// @param offset Offset in buffer to read from (in bytes)
    /// @return Result indicating success or failure
    core::Result<void> download(void* data, VkDeviceSize size, VkDeviceSize offset = 0);

    /// Map buffer memory to CPU address space
    /// Only valid for CPU-accessible buffers (CpuToGpu, GpuToCpu, CpuOnly).
    /// @return Result containing mapped pointer or error code
    core::Result<void*> map();

    /// Unmap buffer memory
    void unmap();

    /// Resize the buffer to a new size
    /// This creates a new buffer and destroys the old one. Data is not preserved.
    /// @param newSize New buffer size in bytes
    /// @return Result indicating success or failure
    core::Result<void> resize(VkDeviceSize newSize);

    /// Get the underlying Vulkan buffer handle
    /// @return VkBuffer handle
    VkBuffer getBuffer() const noexcept { return buffer_.buffer; }

    /// Get the buffer size in bytes
    /// @return Size in bytes
    VkDeviceSize getSize() const noexcept { return size_; }

    /// Get the buffer usage flags
    /// @return Usage flags
    VkBufferUsageFlags getUsage() const noexcept { return usage_; }

    /// Get the memory usage pattern
    /// @return Memory usage type
    MemoryUsage getMemoryUsage() const noexcept { return memoryUsage_; }

    /// Check if buffer is currently mapped
    /// @return true if mapped
    bool isMapped() const noexcept { return mappedPtr_ != nullptr; }

protected:
    VkMemoryManager* memManager_;     ///< Memory manager (not owned)
    VkMemoryManager::Buffer buffer_;  ///< Vulkan buffer with allocation
    VkDeviceSize size_;               ///< Buffer size in bytes
    VkBufferUsageFlags usage_;        ///< Buffer usage flags
    MemoryUsage memoryUsage_;         ///< Memory usage pattern
    void* mappedPtr_;                 ///< Mapped pointer (null if not mapped)

    /// Create a staging buffer for data transfer
    /// @param size Size of staging buffer
    /// @param forUpload true for CPU-to-GPU, false for GPU-to-CPU
    /// @return Result containing staging buffer or error
    core::Result<VkMemoryManager::Buffer> createStagingBuffer(VkDeviceSize size, bool forUpload);

    /// Copy data between buffers using a command buffer
    /// @param srcBuffer Source buffer
    /// @param dstBuffer Destination buffer
    /// @param size Size to copy in bytes
    /// @param srcOffset Source offset in bytes
    /// @param dstOffset Destination offset in bytes
    /// @return Result indicating success or failure
    core::Result<void> copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
                                  VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);
};

/// Type-safe buffer wrapper for strongly-typed data
/// Provides convenience methods for uploading vectors and arrays of a specific type.
///
/// Example usage:
/// @code
/// struct Vertex { Vec3 pos; Vec3 normal; Vec2 uv; };
/// TypedBuffer<Vertex> vertexBuffer(memManager, 1000,
///                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
///                                  MemoryUsage::GpuOnly);
///
/// std::vector<Vertex> vertices = /* ... */;
/// vertexBuffer.upload(vertices);
/// @endcode
template <typename T>
class TypedBuffer : public GpuBuffer {
public:
    /// Create a typed buffer for a specific element count
    /// @param memManager Memory manager for allocation
    /// @param count Number of elements
    /// @param usage Buffer usage flags
    /// @param memoryUsage Memory usage pattern
    TypedBuffer(VkMemoryManager* memManager, size_t count, VkBufferUsageFlags usage,
                MemoryUsage memoryUsage)
        : GpuBuffer(memManager, count * sizeof(T), usage, memoryUsage), count_(count) {}

    /// Upload data from a std::vector
    /// @param data Vector containing the data to upload
    /// @return Result indicating success or failure
    core::Result<void> upload(const std::vector<T>& data) {
        if (data.size() > count_) {
            return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                               "Data size exceeds buffer capacity");
        }
        return GpuBuffer::upload(data.data(), data.size() * sizeof(T));
    }

    /// Upload data from an array
    /// @param data Pointer to array
    /// @param count Number of elements to upload
    /// @param offset Element offset in buffer
    /// @return Result indicating success or failure
    core::Result<void> upload(const T* data, size_t count, size_t offset = 0) {
        if (offset + count > count_) {
            return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                               "Upload range exceeds buffer capacity");
        }
        return GpuBuffer::upload(data, count * sizeof(T), offset * sizeof(T));
    }

    /// Download data to a std::vector
    /// @param data Vector to receive the data (will be resized to count)
    /// @return Result indicating success or failure
    core::Result<void> download(std::vector<T>& data) {
        data.resize(count_);
        return GpuBuffer::download(data.data(), count_ * sizeof(T));
    }

    /// Download data to an array
    /// @param data Pointer to destination array
    /// @param count Number of elements to download
    /// @param offset Element offset in buffer
    /// @return Result indicating success or failure
    core::Result<void> download(T* data, size_t count, size_t offset = 0) {
        if (offset + count > count_) {
            return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                               "Download range exceeds buffer capacity");
        }
        return GpuBuffer::download(data, count * sizeof(T), offset * sizeof(T));
    }

    /// Map buffer and return typed pointer
    /// @return Result containing typed pointer or error
    core::Result<T*> mapTyped() {
        auto result = GpuBuffer::map();
        if (result.isFailure()) {
            return core::Result<T*>::failure(result.errorCode(), result.errorMessage());
        }
        return core::Result<T*>::success(static_cast<T*>(result.value()));
    }

    /// Get the element count
    /// @return Number of elements in buffer
    size_t getCount() const noexcept { return count_; }

    /// Resize buffer to new element count
    /// @param newCount New number of elements
    /// @return Result indicating success or failure
    core::Result<void> resize(size_t newCount) {
        auto result = GpuBuffer::resize(newCount * sizeof(T));
        if (result.isSuccess()) {
            count_ = newCount;
        }
        return result;
    }

private:
    size_t count_;  ///< Number of elements
};

/// Convenience alias for vertex buffers
/// Usage: VertexBuffer<MyVertexType> vbo(memManager, count);
template <typename Vertex>
using VertexBuffer = TypedBuffer<Vertex>;

/// Index buffer (typically uint32_t or uint16_t indices)
/// Usage: IndexBuffer ibo(memManager, indexCount);
using IndexBuffer = TypedBuffer<uint32_t>;

/// Index buffer with 16-bit indices (for smaller meshes)
using IndexBuffer16 = TypedBuffer<uint16_t>;

/// Uniform buffer with persistent mapping for frequent updates
/// This buffer is automatically mapped on creation for efficient per-frame updates.
///
/// Example usage:
/// @code
/// struct CameraUBO { Mat4 view; Mat4 proj; };
/// UniformBuffer<CameraUBO> cameraUBO(memManager);
/// cameraUBO.update(cameraData);  // Efficient per-frame update
/// @endcode
template <typename T>
class UniformBuffer : public TypedBuffer<T> {
public:
    /// Create a uniform buffer with persistent mapping
    /// @param memManager Memory manager for allocation
    UniformBuffer(VkMemoryManager* memManager)
        : TypedBuffer<T>(memManager, 1,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                         MemoryUsage::CpuToGpu) {
        // Persistent mapping for efficient updates
        auto mapResult = this->map();
        if (mapResult.isFailure()) {
            // Log error but don't throw - buffer is still usable via upload()
            AXIOM_LOG_ERROR("GpuBuffer", "Failed to map uniform buffer: %s",
                            mapResult.errorMessage());
        }
    }

    /// Destructor - automatically unmaps if still mapped
    ~UniformBuffer() {
        if (this->isMapped()) {
            this->unmap();
        }
    }

    /// Update uniform buffer data (efficient for per-frame updates)
    /// If buffer is mapped, directly copies to mapped memory.
    /// Otherwise, falls back to upload() with staging.
    /// @param data Reference to data to update
    /// @return Result indicating success or failure
    core::Result<void> update(const T& data) {
        if (this->isMapped()) {
            std::memcpy(this->mappedPtr_, &data, sizeof(T));
            return core::Result<void>::success();
        } else {
            // Fallback to upload if mapping failed
            return this->upload(&data, 1);
        }
    }
};

/// Storage buffer for compute shader read/write operations
/// Typically used for large datasets like particle systems or simulation data.
///
/// Example usage:
/// @code
/// struct Particle { Vec3 position; Vec3 velocity; };
/// StorageBuffer<Particle> particles(memManager, 10000);
/// particles.upload(initialData);
/// // Bind to compute shader descriptor set
/// @endcode
template <typename T>
class StorageBuffer : public TypedBuffer<T> {
public:
    /// Create a storage buffer
    /// @param memManager Memory manager for allocation
    /// @param count Number of elements
    StorageBuffer(VkMemoryManager* memManager, size_t count)
        : TypedBuffer<T>(memManager, count,
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                             VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                         MemoryUsage::GpuOnly) {}
};

/// Indirect buffer for indirect draw/dispatch commands
/// Used for GPU-driven rendering or compute dispatch.
///
/// Example usage:
/// @code
/// IndirectBuffer drawCommands(memManager, commandCount);
/// std::vector<VkDrawIndirectCommand> commands = /* ... */;
/// drawCommands.upload(commands);
/// vkCmdDrawIndirect(cmd, drawCommands.getBuffer(), 0, commandCount,
/// sizeof(VkDrawIndirectCommand));
/// @endcode
class IndirectBuffer : public TypedBuffer<uint32_t> {
public:
    /// Create an indirect buffer
    /// @param memManager Memory manager for allocation
    /// @param size Buffer size in bytes
    IndirectBuffer(VkMemoryManager* memManager, VkDeviceSize size)
        : TypedBuffer<uint32_t>(memManager, size / sizeof(uint32_t),
                                VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                MemoryUsage::GpuOnly) {}
};

}  // namespace axiom::gpu
