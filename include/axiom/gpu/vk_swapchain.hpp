#pragma once

#include "axiom/core/result.hpp"

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace axiom::gpu {

// Forward declaration
class VkContext;

/// Configuration for swapchain creation
/// This structure contains all parameters needed to create and configure
/// a Vulkan swapchain for presenting rendered images to a window surface.
struct SwapchainConfig {
    /// The window surface to present to
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    /// Desired width of the swapchain images
    uint32_t width = 0;

    /// Desired height of the swapchain images
    uint32_t height = 0;

    /// Preferred present mode (e.g., MAILBOX for triple buffering)
    /// Common modes: FIFO (vsync), MAILBOX (triple buffer), IMMEDIATE (no sync)
    VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    /// Enable vsync (forces FIFO present mode if true)
    bool vsync = true;
};

/// Result of acquiring the next swapchain image
struct AcquireResult {
    /// Index of the acquired image in the swapchain
    uint32_t imageIndex = 0;

    /// True if swapchain needs to be recreated (e.g., window resized)
    bool needsResize = false;
};

/// Information for presenting a swapchain image
struct PresentInfo {
    /// Index of the image to present
    uint32_t imageIndex = 0;

    /// Semaphores to wait on before presenting
    std::vector<VkSemaphore> waitSemaphores;
};

/// Manages a Vulkan swapchain for window presentation
///
/// The swapchain is responsible for managing a series of image buffers that are
/// presented to the window surface. It handles double/triple buffering to prevent
/// screen tearing and provides synchronization primitives for rendering.
///
/// Key responsibilities:
/// - Create and configure the swapchain with appropriate format, present mode, and extent
/// - Manage swapchain images and image views
/// - Handle image acquisition and presentation
/// - Recreate swapchain on window resize or other invalidation events
///
/// Example usage:
/// @code
/// SwapchainConfig config{
///     .surface = window->getSurface(),
///     .width = 1920,
///     .height = 1080,
///     .vsync = true
/// };
///
/// auto swapchainResult = Swapchain::create(context, config);
/// if (swapchainResult.isSuccess()) {
///     Swapchain& swapchain = *swapchainResult.value();
///
///     // Acquire next image
///     auto acquireResult = swapchain.acquireNextImage(imageAvailableSemaphore);
///     if (!acquireResult.needsResize) {
///         // Render to swapchain.getImage(acquireResult.imageIndex)
///         // ...
///
///         // Present
///         swapchain.present(graphicsQueue, {
///             .imageIndex = acquireResult.imageIndex,
///             .waitSemaphores = {renderFinishedSemaphore}
///         });
///     }
/// }
/// @endcode
class Swapchain {
public:
    /// Create a new swapchain
    /// @param context The Vulkan context containing device and queue information
    /// @param config Configuration parameters for the swapchain
    /// @return Result containing the swapchain on success, or error code on failure
    static core::Result<Swapchain> create(VkContext* context, const SwapchainConfig& config);

    /// Destructor - cleans up swapchain resources
    ~Swapchain();

    // Disable copy operations (Vulkan handles are not copyable)
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    // Enable move operations
    Swapchain(Swapchain&& other) noexcept;
    Swapchain& operator=(Swapchain&& other) noexcept;

    /// Acquire the next available image from the swapchain
    /// This function obtains the index of the next image to render to.
    /// It will wait up to the specified timeout for an image to become available.
    ///
    /// @param signalSemaphore Semaphore to signal when the image is available
    /// @param timeout Maximum time to wait in nanoseconds (default: UINT64_MAX)
    /// @return AcquireResult containing the image index and resize flag
    AcquireResult acquireNextImage(VkSemaphore signalSemaphore, uint64_t timeout = UINT64_MAX);

    /// Present the rendered image to the surface
    /// This queues the specified image for presentation to the window.
    /// The operation will wait for all specified semaphores before presenting.
    ///
    /// @param queue The presentation queue (usually the graphics queue)
    /// @param info Presentation information (image index and wait semaphores)
    /// @return true on success, false if swapchain needs resize
    bool present(VkQueue queue, const PresentInfo& info);

    /// Resize the swapchain
    /// This recreates the swapchain with new dimensions. Should be called
    /// when acquireNextImage or present indicates a resize is needed.
    ///
    /// @param width New width in pixels
    /// @param height New height in pixels
    /// @return Result indicating success or failure
    core::Result<void> resize(uint32_t width, uint32_t height);

    /// Get the raw swapchain handle
    /// @return The VkSwapchainKHR handle
    VkSwapchainKHR get() const noexcept { return swapchain_; }

    /// Get the swapchain image format
    /// @return The format of swapchain images (e.g., VK_FORMAT_B8G8R8A8_SRGB)
    VkFormat getFormat() const noexcept { return format_; }

    /// Get the swapchain extent (dimensions)
    /// @return The extent structure containing width and height
    VkExtent2D getExtent() const noexcept { return extent_; }

    /// Get the number of swapchain images
    /// @return Number of images in the swapchain (typically 2-3)
    uint32_t getImageCount() const noexcept { return static_cast<uint32_t>(images_.size()); }

    /// Get a swapchain image by index
    /// @param index Image index (must be < getImageCount())
    /// @return The VkImage handle
    VkImage getImage(uint32_t index) const { return images_[index]; }

    /// Get a swapchain image view by index
    /// @param index Image index (must be < getImageCount())
    /// @return The VkImageView handle
    VkImageView getImageView(uint32_t index) const { return imageViews_[index]; }

private:
    /// Private constructor - use create() instead
    /// @param context The Vulkan context
    /// @param config Swapchain configuration
    Swapchain(VkContext* context, const SwapchainConfig& config);

    /// Create the swapchain and associated resources
    /// @return Result indicating success or failure
    core::Result<void> createSwapchain();

    /// Cleanup all swapchain resources
    void cleanup();

    /// Choose the best surface format from available formats
    /// Prefers sRGB color space with BGRA8 format
    /// @param formats Available surface formats
    /// @return The selected surface format
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;

    /// Choose the best present mode from available modes
    /// Respects vsync setting and preferredPresentMode from config
    /// @param modes Available present modes
    /// @return The selected present mode
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes) const;

    /// Choose the swapchain extent based on surface capabilities
    /// Clamps the desired width/height to surface min/max capabilities
    /// @param capabilities Surface capabilities
    /// @return The selected extent
    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    // Context and configuration
    VkContext* context_ = nullptr;
    SwapchainConfig config_;

    // Swapchain resources
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat format_ = VK_FORMAT_UNDEFINED;
    VkExtent2D extent_ = {0, 0};

    // Images and image views
    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
};

}  // namespace axiom::gpu
