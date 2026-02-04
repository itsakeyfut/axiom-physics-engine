#include "axiom/gpu/vk_swapchain.hpp"

#include "axiom/core/assert.hpp"
#include "axiom/gpu/vk_instance.hpp"

#include <algorithm>
#include <iostream>
#include <limits>

namespace axiom::gpu {

// Constructor
Swapchain::Swapchain(VkContext* context, const SwapchainConfig& config)
    : context_(context), config_(config) {
    AXIOM_ASSERT(context != nullptr, "VkContext cannot be null");
    AXIOM_ASSERT(config.surface != VK_NULL_HANDLE, "Surface cannot be null");
    AXIOM_ASSERT(config.width > 0 && config.height > 0, "Invalid dimensions");
}

// Destructor
Swapchain::~Swapchain() {
    cleanup();
}

// Move constructor
Swapchain::Swapchain(Swapchain&& other) noexcept
    : context_(other.context_),
      config_(other.config_),
      swapchain_(other.swapchain_),
      format_(other.format_),
      extent_(other.extent_),
      images_(std::move(other.images_)),
      imageViews_(std::move(other.imageViews_)) {
    // Reset other's handles to prevent double-free
    other.swapchain_ = VK_NULL_HANDLE;
    other.imageViews_.clear();
}

// Move assignment
Swapchain& Swapchain::operator=(Swapchain&& other) noexcept {
    if (this != &other) {
        // Clean up existing resources
        cleanup();

        // Move resources
        context_ = other.context_;
        config_ = other.config_;
        swapchain_ = other.swapchain_;
        format_ = other.format_;
        extent_ = other.extent_;
        images_ = std::move(other.images_);
        imageViews_ = std::move(other.imageViews_);

        // Reset other's handles
        other.swapchain_ = VK_NULL_HANDLE;
        other.imageViews_.clear();
    }
    return *this;
}

// Create swapchain
core::Result<Swapchain> Swapchain::create(VkContext* context, const SwapchainConfig& config) {
    if (!context) {
        return core::Result<Swapchain>::failure(core::ErrorCode::InvalidParameter,
                                                "VkContext cannot be null");
    }

    if (config.surface == VK_NULL_HANDLE) {
        return core::Result<Swapchain>::failure(core::ErrorCode::InvalidParameter,
                                                "Surface cannot be null");
    }

    if (config.width == 0 || config.height == 0) {
        return core::Result<Swapchain>::failure(core::ErrorCode::InvalidParameter,
                                                "Invalid dimensions");
    }

    Swapchain swapchain(context, config);
    auto result = swapchain.createSwapchain();
    if (!result.isSuccess()) {
        return core::Result<Swapchain>::failure(result.errorCode(), result.errorMessage());
    }

    return core::Result<Swapchain>::success(std::move(swapchain));
}

// Create swapchain implementation
core::Result<void> Swapchain::createSwapchain() {
    VkDevice device = context_->getDevice();
    VkPhysicalDevice physicalDevice = context_->getPhysicalDevice();

    // Query surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, config_.surface, &capabilities);

    // Query surface formats
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, config_.surface, &formatCount, nullptr);
    if (formatCount == 0) {
        return core::Result<void>::failure(core::ErrorCode::GPU_INVALID_OPERATION,
                                           "No surface formats available");
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, config_.surface, &formatCount,
                                         formats.data());

    // Query present modes
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, config_.surface, &presentModeCount,
                                              nullptr);
    if (presentModeCount == 0) {
        return core::Result<void>::failure(core::ErrorCode::GPU_INVALID_OPERATION,
                                           "No present modes available");
    }

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, config_.surface, &presentModeCount,
                                              presentModes.data());

    // Choose swapchain settings
    VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(formats);
    VkPresentModeKHR presentMode = choosePresentMode(presentModes);
    VkExtent2D extent = chooseExtent(capabilities);

    // Determine image count (min + 1 for triple buffering)
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    // Create swapchain
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = config_.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Handle queue family indices
    uint32_t queueFamilyIndices[] = {context_->getGraphicsQueueFamily(),
                                     context_->getComputeQueueFamily()};

    if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain_);
    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::GPU_INVALID_OPERATION,
                                           "Failed to create swapchain");
    }

    // Store format and extent
    format_ = surfaceFormat.format;
    extent_ = extent;

    // Retrieve swapchain images
    uint32_t actualImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain_, &actualImageCount, nullptr);
    images_.resize(actualImageCount);
    vkGetSwapchainImagesKHR(device, swapchain_, &actualImageCount, images_.data());

    // Create image views
    imageViews_.resize(images_.size());
    for (size_t i = 0; i < images_.size(); ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = images_[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format_;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(device, &viewInfo, nullptr, &imageViews_[i]);
        if (result != VK_SUCCESS) {
            // Clean up already created image views
            for (size_t j = 0; j < i; ++j) {
                vkDestroyImageView(device, imageViews_[j], nullptr);
            }
            vkDestroySwapchainKHR(device, swapchain_, nullptr);
            return core::Result<void>::failure(core::ErrorCode::GPU_INVALID_OPERATION,
                                               "Failed to create image views");
        }
    }

    // TODO: Add logging when logger is implemented (Issue #022)
    // AXIOM_LOG_INFO("GPU", "Swapchain created: {}x{}, format={}, imageCount={}", ...);

    return core::Result<void>::success();
}

// Cleanup
void Swapchain::cleanup() {
    if (!context_) {
        return;
    }

    VkDevice device = context_->getDevice();

    // Destroy image views
    for (VkImageView imageView : imageViews_) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, imageView, nullptr);
        }
    }
    imageViews_.clear();

    // Destroy swapchain (images are owned by swapchain and destroyed automatically)
    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }

    images_.clear();
}

// Acquire next image
AcquireResult Swapchain::acquireNextImage(VkSemaphore signalSemaphore, uint64_t timeout) {
    AXIOM_ASSERT(swapchain_ != VK_NULL_HANDLE, "Swapchain not initialized");
    AXIOM_ASSERT(signalSemaphore != VK_NULL_HANDLE, "Signal semaphore cannot be null");

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(context_->getDevice(), swapchain_, timeout,
                                            signalSemaphore, VK_NULL_HANDLE, &imageIndex);

    AcquireResult acquireResult;
    acquireResult.imageIndex = imageIndex;

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        acquireResult.needsResize = true;
        // TODO: Add logging when logger is implemented (Issue #022)
        // AXIOM_LOG_WARN("GPU", "Swapchain out of date or suboptimal, resize needed");
    } else if (result != VK_SUCCESS) {
        // TODO: Add logging when logger is implemented (Issue #022)
        // AXIOM_LOG_ERROR("GPU", "Failed to acquire swapchain image: {}", ...);
        acquireResult.needsResize = true;
    }

    return acquireResult;
}

// Present
bool Swapchain::present(VkQueue queue, const PresentInfo& info) {
    AXIOM_ASSERT(swapchain_ != VK_NULL_HANDLE, "Swapchain not initialized");
    AXIOM_ASSERT(queue != VK_NULL_HANDLE, "Queue cannot be null");
    AXIOM_ASSERT(info.imageIndex < images_.size(), "Invalid image index");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(info.waitSemaphores.size());
    presentInfo.pWaitSemaphores = info.waitSemaphores.data();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &info.imageIndex;
    presentInfo.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(queue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // TODO: Add logging when logger is implemented (Issue #022)
        // AXIOM_LOG_WARN("GPU", "Swapchain out of date during present, resize needed");
        return false;
    } else if (result != VK_SUCCESS) {
        // TODO: Add logging when logger is implemented (Issue #022)
        // AXIOM_LOG_ERROR("GPU", "Failed to present swapchain image: {}", ...);
        return false;
    }

    return true;
}

// Resize
core::Result<void> Swapchain::resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                           "Invalid dimensions for resize");
    }

    // TODO: Add logging when logger is implemented (Issue #022)
    // AXIOM_LOG_INFO("GPU", "Resizing swapchain from {}x{} to {}x{}", ...);

    // Wait for device to be idle before recreating swapchain
    vkDeviceWaitIdle(context_->getDevice());

    // Clean up old swapchain
    cleanup();

    // Update configuration
    config_.width = width;
    config_.height = height;

    // Recreate swapchain
    return createSwapchain();
}

// Choose surface format
VkSurfaceFormatKHR
Swapchain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const {
    // Prefer BGRA8 SRGB
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    // Fallback to RGBA8 SRGB
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_R8G8B8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    // If no suitable format found, return the first available
    // TODO: Add logging when logger is implemented (Issue #022)
    // AXIOM_LOG_WARN("GPU", "Preferred sRGB format not found, using first available format");
    return formats[0];
}

// Choose present mode
VkPresentModeKHR Swapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& modes) const {
    // If vsync is enabled, always use FIFO (guaranteed to be available)
    if (config_.vsync) {
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    // Check if preferred mode is available
    for (const auto& mode : modes) {
        if (mode == config_.preferredPresentMode) {
            return mode;
        }
    }

    // Try MAILBOX for triple buffering without vsync
    for (const auto& mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }

    // Try IMMEDIATE for lowest latency
    for (const auto& mode : modes) {
        if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            return mode;
        }
    }

    // Fallback to FIFO (always available)
    return VK_PRESENT_MODE_FIFO_KHR;
}

// Choose extent
VkExtent2D Swapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
    // If currentExtent is special value, surface size is determined by swapchain extent
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    // Otherwise, choose extent within min/max bounds
    VkExtent2D actualExtent = {config_.width, config_.height};

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height);

    return actualExtent;
}

}  // namespace axiom::gpu
