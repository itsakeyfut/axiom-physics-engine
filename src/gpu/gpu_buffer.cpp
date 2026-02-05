#include "axiom/gpu/gpu_buffer.hpp"

#include "axiom/core/logger.hpp"
#include "axiom/gpu/vk_command.hpp"
#include "axiom/gpu/vk_instance.hpp"

#include <cstring>

namespace axiom::gpu {

GpuBuffer::GpuBuffer(VkMemoryManager* memManager, VkDeviceSize size, VkBufferUsageFlags usage,
                     MemoryUsage memoryUsage)
    : memManager_(memManager),
      buffer_(),
      size_(size),
      usage_(usage),
      memoryUsage_(memoryUsage),
      mappedPtr_(nullptr) {
    if (!memManager_) {
        AXIOM_LOG_ERROR("GpuBuffer", "Memory manager is null");
        return;
    }

    // Add TRANSFER flags if using GpuOnly memory (will need staging)
    if (memoryUsage_ == MemoryUsage::GpuOnly) {
        usage_ |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }

    // Create the buffer
    VkMemoryManager::BufferCreateInfo info{
        .size = size_, .usage = usage_, .memoryUsage = memoryUsage_, .persistentMapping = false};

    auto result = memManager_->createBuffer(info);
    if (result.isFailure()) {
        AXIOM_LOG_ERROR("GpuBuffer", "Failed to create buffer: %s", result.errorMessage());
        return;
    }

    buffer_ = result.value();
}

GpuBuffer::~GpuBuffer() {
    if (mappedPtr_) {
        unmap();
    }

    if (buffer_.buffer != VK_NULL_HANDLE && memManager_) {
        memManager_->destroyBuffer(buffer_);
    }
}

GpuBuffer::GpuBuffer(GpuBuffer&& other) noexcept
    : memManager_(other.memManager_),
      buffer_(other.buffer_),
      size_(other.size_),
      usage_(other.usage_),
      memoryUsage_(other.memoryUsage_),
      mappedPtr_(other.mappedPtr_) {
    // Clear other's state to prevent double-free
    other.buffer_ = {};
    other.mappedPtr_ = nullptr;
    other.size_ = 0;
}

GpuBuffer& GpuBuffer::operator=(GpuBuffer&& other) noexcept {
    if (this != &other) {
        // Clean up current resources
        if (mappedPtr_) {
            unmap();
        }
        if (buffer_.buffer != VK_NULL_HANDLE && memManager_) {
            memManager_->destroyBuffer(buffer_);
        }

        // Move from other
        memManager_ = other.memManager_;
        buffer_ = other.buffer_;
        size_ = other.size_;
        usage_ = other.usage_;
        memoryUsage_ = other.memoryUsage_;
        mappedPtr_ = other.mappedPtr_;

        // Clear other's state
        other.buffer_ = {};
        other.mappedPtr_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

core::Result<void> GpuBuffer::upload(const void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (!data) {
        return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                           "Data pointer is null");
    }

    if (offset + size > size_) {
        return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                           "Upload range exceeds buffer size");
    }

    if (buffer_.buffer == VK_NULL_HANDLE) {
        return core::Result<void>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                           "Buffer is not initialized");
    }

    // For CPU-accessible buffers, directly copy to mapped memory
    if (memoryUsage_ == MemoryUsage::CpuToGpu || memoryUsage_ == MemoryUsage::CpuOnly) {
        auto mapResult = memManager_->mapMemory(buffer_);
        if (mapResult.isFailure()) {
            return core::Result<void>::failure(mapResult.errorCode(), mapResult.errorMessage());
        }

        void* mappedData = mapResult.value();
        std::memcpy(static_cast<uint8_t*>(mappedData) + offset, data, size);
        memManager_->unmapMemory(buffer_);

        return core::Result<void>::success();
    }

    // For GPU-only buffers, use staging buffer
    auto stagingResult = createStagingBuffer(size, true);
    if (stagingResult.isFailure()) {
        return core::Result<void>::failure(stagingResult.errorCode(), stagingResult.errorMessage());
    }

    auto staging = stagingResult.value();

    // Copy data to staging buffer
    auto mapResult = memManager_->mapMemory(staging);
    if (mapResult.isFailure()) {
        memManager_->destroyBuffer(staging);
        return core::Result<void>::failure(mapResult.errorCode(), mapResult.errorMessage());
    }

    std::memcpy(mapResult.value(), data, size);
    memManager_->unmapMemory(staging);

    // Copy from staging to device buffer
    auto copyResult = copyBuffer(staging.buffer, buffer_.buffer, size, 0, offset);

    // Clean up staging buffer
    memManager_->destroyBuffer(staging);

    return copyResult;
}

core::Result<void> GpuBuffer::download(void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (!data) {
        return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                           "Data pointer is null");
    }

    if (offset + size > size_) {
        return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                           "Download range exceeds buffer size");
    }

    if (buffer_.buffer == VK_NULL_HANDLE) {
        return core::Result<void>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                           "Buffer is not initialized");
    }

    // For CPU-accessible buffers, directly copy from mapped memory
    if (memoryUsage_ == MemoryUsage::GpuToCpu || memoryUsage_ == MemoryUsage::CpuOnly ||
        memoryUsage_ == MemoryUsage::CpuToGpu) {
        auto mapResult = memManager_->mapMemory(buffer_);
        if (mapResult.isFailure()) {
            return core::Result<void>::failure(mapResult.errorCode(), mapResult.errorMessage());
        }

        void* mappedData = mapResult.value();
        std::memcpy(data, static_cast<const uint8_t*>(mappedData) + offset, size);
        memManager_->unmapMemory(buffer_);

        return core::Result<void>::success();
    }

    // For GPU-only buffers, use staging buffer
    auto stagingResult = createStagingBuffer(size, false);
    if (stagingResult.isFailure()) {
        return core::Result<void>::failure(stagingResult.errorCode(), stagingResult.errorMessage());
    }

    auto staging = stagingResult.value();

    // Copy from device buffer to staging
    auto copyResult = copyBuffer(buffer_.buffer, staging.buffer, size, offset, 0);
    if (copyResult.isFailure()) {
        memManager_->destroyBuffer(staging);
        return copyResult;
    }

    // Copy data from staging buffer to CPU
    auto mapResult = memManager_->mapMemory(staging);
    if (mapResult.isFailure()) {
        memManager_->destroyBuffer(staging);
        return core::Result<void>::failure(mapResult.errorCode(), mapResult.errorMessage());
    }

    std::memcpy(data, mapResult.value(), size);
    memManager_->unmapMemory(staging);

    // Clean up staging buffer
    memManager_->destroyBuffer(staging);

    return core::Result<void>::success();
}

core::Result<void*> GpuBuffer::map() {
    if (buffer_.buffer == VK_NULL_HANDLE) {
        return core::Result<void*>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                            "Buffer is not initialized");
    }

    if (mappedPtr_) {
        // Already mapped, return existing pointer
        return core::Result<void*>::success(mappedPtr_);
    }

    // Check if buffer is CPU-accessible
    if (memoryUsage_ == MemoryUsage::GpuOnly) {
        return core::Result<void*>::failure(core::ErrorCode::InvalidParameter,
                                            "Cannot map GPU-only buffer");
    }

    auto result = memManager_->mapMemory(buffer_);
    if (result.isSuccess()) {
        mappedPtr_ = result.value();
    }
    return result;
}

void GpuBuffer::unmap() {
    if (mappedPtr_) {
        memManager_->unmapMemory(buffer_);
        mappedPtr_ = nullptr;
    }
}

core::Result<void> GpuBuffer::resize(VkDeviceSize newSize) {
    if (newSize == 0) {
        return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                           "New size cannot be zero");
    }

    // Unmap if currently mapped
    bool wasMapped = mappedPtr_ != nullptr;
    if (wasMapped) {
        unmap();
    }

    // Destroy old buffer
    if (buffer_.buffer != VK_NULL_HANDLE) {
        memManager_->destroyBuffer(buffer_);
        buffer_ = {};
    }

    // Create new buffer with new size
    size_ = newSize;

    VkMemoryManager::BufferCreateInfo info{
        .size = size_, .usage = usage_, .memoryUsage = memoryUsage_, .persistentMapping = false};

    auto result = memManager_->createBuffer(info);
    if (result.isFailure()) {
        return core::Result<void>::failure(result.errorCode(), result.errorMessage());
    }

    buffer_ = result.value();

    // Remap if it was mapped before
    if (wasMapped) {
        auto mapResult = map();
        if (mapResult.isFailure()) {
            AXIOM_LOG_ERROR("GpuBuffer", "Failed to remap buffer after resize: %s",
                            mapResult.errorMessage());
        }
    }

    return core::Result<void>::success();
}

core::Result<VkMemoryManager::Buffer> GpuBuffer::createStagingBuffer(VkDeviceSize size,
                                                                     bool forUpload) {
    VkBufferUsageFlags stagingUsage = forUpload ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                                                : VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VkMemoryManager::BufferCreateInfo info{.size = size,
                                           .usage = stagingUsage,
                                           .memoryUsage = forUpload ? MemoryUsage::CpuToGpu
                                                                    : MemoryUsage::GpuToCpu,
                                           .persistentMapping = false};

    return memManager_->createBuffer(info);
}

core::Result<void> GpuBuffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
                                         VkDeviceSize srcOffset, VkDeviceSize dstOffset) {
    // Get VkContext from memory manager
    VkContext* context = memManager_->getContext();
    if (!context) {
        return core::Result<void>::failure(core::ErrorCode::InvalidParameter, "VkContext is null");
    }

    // Use one-time command buffer for the copy operation
    VkQueue transferQueue = context->getTransferQueue();
    uint32_t transferFamily = context->getTransferQueueFamily();

    OneTimeCommand cmd(context, transferQueue, transferFamily);

    // Record copy command
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;

    vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

    // Command is automatically submitted and waited on destruction
    return core::Result<void>::success();
}

}  // namespace axiom::gpu
