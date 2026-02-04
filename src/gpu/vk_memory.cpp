#include "axiom/gpu/vk_memory.hpp"

#include "axiom/core/assert.hpp"
#include "axiom/gpu/vk_instance.hpp"

#include <cstdio>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace axiom::gpu {

VkMemoryManager::VkMemoryManager(VkContext* context) : context_(context), allocator_(nullptr) {
    AXIOM_ASSERT(context != nullptr, "VkContext must not be null");
}

VkMemoryManager::~VkMemoryManager() {
    if (allocator_ != nullptr) {
        vmaDestroyAllocator(static_cast<VmaAllocator>(allocator_));
        allocator_ = nullptr;
    }
}

core::Result<std::unique_ptr<VkMemoryManager>> VkMemoryManager::create(VkContext* context) {
    if (context == nullptr) {
        return core::Result<std::unique_ptr<VkMemoryManager>>::failure(
            core::ErrorCode::InvalidParameter, "VkContext must not be null");
    }

    auto manager = std::unique_ptr<VkMemoryManager>(new VkMemoryManager(context));

    auto result = manager->initialize();
    if (result.isFailure()) {
        return core::Result<std::unique_ptr<VkMemoryManager>>::failure(result.errorCode(),
                                                                       result.errorMessage());
    }

    return core::Result<std::unique_ptr<VkMemoryManager>>::success(std::move(manager));
}

core::Result<void> VkMemoryManager::initialize() {
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_1;
    allocatorInfo.physicalDevice = context_->getPhysicalDevice();
    allocatorInfo.device = context_->getDevice();
    allocatorInfo.instance = context_->getInstance();

    VmaAllocator allocator;
    VkResult result = vmaCreateAllocator(&allocatorInfo, &allocator);
    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to create VMA allocator");
    }

    allocator_ = allocator;
    return core::Result<void>::success();
}

int VkMemoryManager::toVmaMemoryUsage(MemoryUsage usage) const noexcept {
    switch (usage) {
    case MemoryUsage::GpuOnly:
        return VMA_MEMORY_USAGE_GPU_ONLY;
    case MemoryUsage::CpuToGpu:
        return VMA_MEMORY_USAGE_CPU_TO_GPU;
    case MemoryUsage::GpuToCpu:
        return VMA_MEMORY_USAGE_GPU_TO_CPU;
    case MemoryUsage::CpuOnly:
        return VMA_MEMORY_USAGE_CPU_ONLY;
    default:
        return VMA_MEMORY_USAGE_UNKNOWN;
    }
}

core::Result<VkMemoryManager::Buffer> VkMemoryManager::createBuffer(const BufferCreateInfo& info) {
    // Create buffer create info
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = info.size;
    bufferInfo.usage = info.usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create VMA allocation info
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = static_cast<VmaMemoryUsage>(toVmaMemoryUsage(info.memoryUsage));

    // Enable persistent mapping if requested
    if (info.persistentMapping) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    Buffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;

    VkResult result = vmaCreateBuffer(static_cast<VmaAllocator>(allocator_), &bufferInfo,
                                      &allocInfo, &buffer.buffer, &allocation, &allocationInfo);

    if (result != VK_SUCCESS) {
        return core::Result<Buffer>::failure(core::ErrorCode::BufferAllocationFailed,
                                             "Failed to create buffer");
    }

    buffer.allocation = allocation;

    // Store mapped pointer if persistent mapping was requested
    if (info.persistentMapping) {
        buffer.mappedPtr = allocationInfo.pMappedData;
    }

    return core::Result<Buffer>::success(buffer);
}

void VkMemoryManager::destroyBuffer(Buffer& buffer) {
    if (buffer.buffer != VK_NULL_HANDLE && buffer.allocation != nullptr) {
        vmaDestroyBuffer(static_cast<VmaAllocator>(allocator_), buffer.buffer,
                         static_cast<VmaAllocation>(buffer.allocation));
        buffer.buffer = VK_NULL_HANDLE;
        buffer.allocation = nullptr;
        buffer.mappedPtr = nullptr;
    }
}

core::Result<VkMemoryManager::Image> VkMemoryManager::createImage(const ImageCreateInfo& info) {
    // Create image create info
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = info.extent;
    imageInfo.mipLevels = info.mipLevels;
    imageInfo.arrayLayers = info.arrayLayers;
    imageInfo.format = info.format;
    imageInfo.tiling = info.tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = info.usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create VMA allocation info
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = static_cast<VmaMemoryUsage>(toVmaMemoryUsage(info.memoryUsage));

    Image image;
    VmaAllocation allocation;
    VkResult result = vmaCreateImage(static_cast<VmaAllocator>(allocator_), &imageInfo, &allocInfo,
                                     &image.image, &allocation, nullptr);

    if (result != VK_SUCCESS) {
        return core::Result<Image>::failure(core::ErrorCode::BufferAllocationFailed,
                                            "Failed to create image");
    }

    image.allocation = allocation;
    return core::Result<Image>::success(image);
}

void VkMemoryManager::destroyImage(Image& image) {
    if (image.image != VK_NULL_HANDLE && image.allocation != nullptr) {
        vmaDestroyImage(static_cast<VmaAllocator>(allocator_), image.image,
                        static_cast<VmaAllocation>(image.allocation));
        image.image = VK_NULL_HANDLE;
        image.allocation = nullptr;
    }
}

core::Result<void*> VkMemoryManager::mapMemory(const Buffer& buffer) {
    void* data = nullptr;
    VkResult result = vmaMapMemory(static_cast<VmaAllocator>(allocator_),
                                   static_cast<VmaAllocation>(buffer.allocation), &data);

    if (result != VK_SUCCESS) {
        return core::Result<void*>::failure(core::ErrorCode::InvalidParameter,
                                            "Failed to map buffer memory");
    }

    return core::Result<void*>::success(data);
}

void VkMemoryManager::unmapMemory(const Buffer& buffer) {
    vmaUnmapMemory(static_cast<VmaAllocator>(allocator_),
                   static_cast<VmaAllocation>(buffer.allocation));
}

VkMemoryManager::MemoryStats VkMemoryManager::getStats() const {
    VmaTotalStatistics vmaStats;
    vmaCalculateStatistics(static_cast<VmaAllocator>(allocator_), &vmaStats);

    MemoryStats stats;
    stats.usedBytes = vmaStats.total.statistics.allocationBytes;
    stats.allocatedBytes = vmaStats.total.statistics.blockBytes;
    stats.allocationCount = vmaStats.total.statistics.allocationCount;
    stats.blockCount = vmaStats.total.statistics.blockCount;

    return stats;
}

void VkMemoryManager::printStats() const {
    auto stats = getStats();

    fprintf(stdout, "\n========================================\n");
    fprintf(stdout, "VULKAN MEMORY STATISTICS\n");
    fprintf(stdout, "========================================\n");
    fprintf(stdout, "Used memory:       %llu bytes (%.2f MB)\n",
            static_cast<unsigned long long>(stats.usedBytes),
            static_cast<double>(stats.usedBytes) / (1024.0 * 1024.0));
    fprintf(stdout, "Allocated memory:  %llu bytes (%.2f MB)\n",
            static_cast<unsigned long long>(stats.allocatedBytes),
            static_cast<double>(stats.allocatedBytes) / (1024.0 * 1024.0));
    fprintf(stdout, "Allocations:       %u\n", stats.allocationCount);
    fprintf(stdout, "Memory blocks:     %u\n", stats.blockCount);

    if (stats.allocatedBytes > 0) {
        double utilization = (static_cast<double>(stats.usedBytes) /
                              static_cast<double>(stats.allocatedBytes)) *
                             100.0;
        fprintf(stdout, "Utilization:       %.2f%%\n", utilization);
    }

    fprintf(stdout, "========================================\n\n");
}

}  // namespace axiom::gpu
