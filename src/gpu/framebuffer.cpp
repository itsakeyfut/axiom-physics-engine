#include "axiom/gpu/framebuffer.hpp"

#include "axiom/core/logger.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_memory.hpp"
#include "axiom/gpu/vk_swapchain.hpp"

namespace axiom::gpu {

// ============================================================================
// Framebuffer Implementation
// ============================================================================

Framebuffer::Framebuffer(VkContext* context, VkMemoryManager* memManager, const Config& config)
    : context_(context), memManager_(memManager), config_(config), extent_(config.extent) {}

core::Result<std::unique_ptr<Framebuffer>>
Framebuffer::create(VkContext* context, VkMemoryManager* memManager, const Config& config) {
    if (!context) {
        return core::Result<std::unique_ptr<Framebuffer>>::failure(
            core::ErrorCode::InvalidParameter, "VkContext is null");
    }
    if (!memManager) {
        return core::Result<std::unique_ptr<Framebuffer>>::failure(
            core::ErrorCode::InvalidParameter, "VkMemoryManager is null");
    }
    if (config.extent.width == 0 || config.extent.height == 0) {
        return core::Result<std::unique_ptr<Framebuffer>>::failure(
            core::ErrorCode::InvalidParameter, "Invalid framebuffer extent");
    }
    if (!config.createColorAttachment && !config.createDepthAttachment) {
        return core::Result<std::unique_ptr<Framebuffer>>::failure(
            core::ErrorCode::InvalidParameter, "At least one attachment must be created");
    }

    auto framebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(context, memManager, config));

    auto initResult = framebuffer->initialize();
    if (initResult.isFailure()) {
        return core::Result<std::unique_ptr<Framebuffer>>::failure(
            core::ErrorCode::InvalidParameter, initResult.errorMessage());
    }

    return core::Result<std::unique_ptr<Framebuffer>>::success(std::move(framebuffer));
}

Framebuffer::~Framebuffer() {
    cleanup();
}

core::Result<void> Framebuffer::initialize() {
    if (config_.createColorAttachment) {
        auto result = createColorAttachment();
        if (result.isFailure()) {
            cleanup();
            return result;
        }
    }

    if (config_.createDepthAttachment) {
        auto result = createDepthAttachment();
        if (result.isFailure()) {
            cleanup();
            return result;
        }
    }

    return core::Result<void>::success();
}

core::Result<void> Framebuffer::createColorAttachment() {
    // Create color image
    VkMemoryManager::ImageCreateInfo imageInfo{};
    imageInfo.extent = {extent_.width, extent_.height, 1};
    imageInfo.format = config_.colorFormat;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.memoryUsage = MemoryUsage::GpuOnly;

    auto imageResult = memManager_->createImage(imageInfo);
    if (imageResult.isFailure()) {
        return core::Result<void>::failure(imageResult.errorCode(), "Failed to create color image");
    }

    colorImage_ = imageResult.value();

    // Create color image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = colorImage_.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = config_.colorFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkResult vkResult = vkCreateImageView(context_->getDevice(), &viewInfo, nullptr, &colorView_);
    if (vkResult != VK_SUCCESS) {
        memManager_->destroyImage(colorImage_);
        return core::Result<void>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                           "Failed to create color image view");
    }

    return core::Result<void>::success();
}

core::Result<void> Framebuffer::createDepthAttachment() {
    // Create depth image
    VkMemoryManager::ImageCreateInfo imageInfo{};
    imageInfo.extent = {extent_.width, extent_.height, 1};
    imageInfo.format = config_.depthFormat;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.memoryUsage = MemoryUsage::GpuOnly;

    auto imageResult = memManager_->createImage(imageInfo);
    if (imageResult.isFailure()) {
        return core::Result<void>::failure(imageResult.errorCode(), "Failed to create depth image");
    }

    depthImage_ = imageResult.value();

    // Determine aspect mask based on format
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (config_.depthFormat == VK_FORMAT_D16_UNORM_S8_UINT ||
        config_.depthFormat == VK_FORMAT_D24_UNORM_S8_UINT ||
        config_.depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    // Create depth image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage_.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = config_.depthFormat;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkResult vkResult = vkCreateImageView(context_->getDevice(), &viewInfo, nullptr, &depthView_);
    if (vkResult != VK_SUCCESS) {
        memManager_->destroyImage(depthImage_);
        return core::Result<void>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                           "Failed to create depth image view");
    }

    return core::Result<void>::success();
}

void Framebuffer::cleanup() noexcept {
    VkDevice device = context_->getDevice();

    if (colorView_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device, colorView_, nullptr);
        colorView_ = VK_NULL_HANDLE;
    }

    if (depthView_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device, depthView_, nullptr);
        depthView_ = VK_NULL_HANDLE;
    }

    if (colorImage_.image != VK_NULL_HANDLE) {
        memManager_->destroyImage(colorImage_);
        colorImage_ = {};
    }

    if (depthImage_.image != VK_NULL_HANDLE) {
        memManager_->destroyImage(depthImage_);
        depthImage_ = {};
    }

    colorLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    depthLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
}

core::Result<void> Framebuffer::resize(VkExtent2D newExtent) {
    if (newExtent.width == 0 || newExtent.height == 0) {
        return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                           "Invalid extent for resize");
    }

    // Clean up old resources
    cleanup();

    // Update extent
    extent_ = newExtent;

    // Recreate attachments
    return initialize();
}

void Framebuffer::transitionColorLayout(VkCommandBuffer cmd, VkImageLayout newLayout) noexcept {
    if (colorImage_.image == VK_NULL_HANDLE) {
        return;
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = colorLayout_;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = colorImage_.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // Determine access masks and pipeline stages based on layouts
    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    // Source layout
    if (colorLayout_ == VK_IMAGE_LAYOUT_UNDEFINED) {
        barrier.srcAccessMask = 0;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    } else if (colorLayout_ == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (colorLayout_ == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (colorLayout_ == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (colorLayout_ == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    // Destination layout
    if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        barrier.dstAccessMask = 0;
        dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    colorLayout_ = newLayout;
}

void Framebuffer::transitionDepthLayout(VkCommandBuffer cmd, VkImageLayout newLayout) noexcept {
    if (depthImage_.image == VK_NULL_HANDLE) {
        return;
    }

    // Determine aspect mask based on format
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (config_.depthFormat == VK_FORMAT_D16_UNORM_S8_UINT ||
        config_.depthFormat == VK_FORMAT_D24_UNORM_S8_UINT ||
        config_.depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = depthLayout_;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = depthImage_.image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // Determine access masks and pipeline stages based on layouts
    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    // Source layout
    if (depthLayout_ == VK_IMAGE_LAYOUT_UNDEFINED) {
        barrier.srcAccessMask = 0;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    } else if (depthLayout_ == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
               depthLayout_ == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    } else if (depthLayout_ == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ||
               depthLayout_ == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (depthLayout_ == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    // Destination layout
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
        newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ||
               newLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    depthLayout_ = newLayout;
}

// ============================================================================
// SwapchainFramebuffer Implementation
// ============================================================================

SwapchainFramebuffer::SwapchainFramebuffer(VkContext* context, VkMemoryManager* memManager,
                                           Swapchain* swapchain)
    : context_(context), memManager_(memManager), swapchain_(swapchain) {}

core::Result<std::unique_ptr<SwapchainFramebuffer>>
SwapchainFramebuffer::create(VkContext* context, VkMemoryManager* memManager,
                             Swapchain* swapchain) {
    if (!context) {
        return core::Result<std::unique_ptr<SwapchainFramebuffer>>::failure(
            core::ErrorCode::InvalidParameter, "VkContext is null");
    }
    if (!memManager) {
        return core::Result<std::unique_ptr<SwapchainFramebuffer>>::failure(
            core::ErrorCode::InvalidParameter, "VkMemoryManager is null");
    }
    if (!swapchain) {
        return core::Result<std::unique_ptr<SwapchainFramebuffer>>::failure(
            core::ErrorCode::InvalidParameter, "Swapchain is null");
    }

    auto framebuffer = std::unique_ptr<SwapchainFramebuffer>(
        new SwapchainFramebuffer(context, memManager, swapchain));

    auto initResult = framebuffer->initialize();
    if (initResult.isFailure()) {
        return core::Result<std::unique_ptr<SwapchainFramebuffer>>::failure(
            core::ErrorCode::InvalidParameter, initResult.errorMessage());
    }

    return core::Result<std::unique_ptr<SwapchainFramebuffer>>::success(std::move(framebuffer));
}

SwapchainFramebuffer::~SwapchainFramebuffer() {
    cleanup();
}

core::Result<void> SwapchainFramebuffer::initialize() {
    return createDepthBuffer();
}

core::Result<void> SwapchainFramebuffer::createDepthBuffer() {
    VkExtent2D extent = swapchain_->getExtent();

    // Create depth image
    VkMemoryManager::ImageCreateInfo imageInfo{};
    imageInfo.extent = {extent.width, extent.height, 1};
    imageInfo.format = VK_FORMAT_D32_SFLOAT;  // Standard depth format
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.memoryUsage = MemoryUsage::GpuOnly;

    auto imageResult = memManager_->createImage(imageInfo);
    if (imageResult.isFailure()) {
        return core::Result<void>::failure(imageResult.errorCode(),
                                           "Failed to create swapchain depth image");
    }

    depthImage_ = imageResult.value();

    // Create depth image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage_.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkResult vkResult = vkCreateImageView(context_->getDevice(), &viewInfo, nullptr, &depthView_);
    if (vkResult != VK_SUCCESS) {
        memManager_->destroyImage(depthImage_);
        return core::Result<void>::failure(core::ErrorCode::GPU_OPERATION_FAILED,
                                           "Failed to create swapchain depth image view");
    }

    return core::Result<void>::success();
}

void SwapchainFramebuffer::cleanup() noexcept {
    VkDevice device = context_->getDevice();

    if (depthView_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device, depthView_, nullptr);
        depthView_ = VK_NULL_HANDLE;
    }

    if (depthImage_.image != VK_NULL_HANDLE) {
        memManager_->destroyImage(depthImage_);
        depthImage_ = {};
    }

    depthLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
}

VkImageView SwapchainFramebuffer::getColorView(uint32_t imageIndex) const {
    return swapchain_->getImageView(imageIndex);
}

core::Result<void> SwapchainFramebuffer::resize(VkExtent2D newExtent) {
    if (newExtent.width == 0 || newExtent.height == 0) {
        return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                           "Invalid extent for resize");
    }

    // Clean up old depth buffer
    cleanup();

    // Recreate depth buffer with new extent
    return createDepthBuffer();
}

void SwapchainFramebuffer::transitionDepthLayout(VkCommandBuffer cmd,
                                                 VkImageLayout newLayout) noexcept {
    if (depthImage_.image == VK_NULL_HANDLE) {
        return;
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = depthLayout_;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = depthImage_.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // Determine access masks and pipeline stages
    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    // Source layout
    if (depthLayout_ == VK_IMAGE_LAYOUT_UNDEFINED) {
        barrier.srcAccessMask = 0;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    } else if (depthLayout_ == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
               depthLayout_ == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    } else if (depthLayout_ == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ||
               depthLayout_ == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    // Destination layout
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
        newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ||
               newLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    depthLayout_ = newLayout;
}

}  // namespace axiom::gpu
