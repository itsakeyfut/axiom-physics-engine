#pragma once

#include "axiom/core/result.hpp"
#include "axiom/gpu/vk_memory.hpp"

#include <cstdint>
#include <memory>
#include <vulkan/vulkan.h>

namespace axiom::gpu {

// Forward declarations
class VkContext;
class Swapchain;

/// Framebuffer for offscreen and onscreen rendering
/// Manages color and depth attachments with associated images and image views.
/// Uses VK_KHR_dynamic_rendering (no VkFramebuffer objects required).
///
/// Features:
/// - Color and depth attachment creation
/// - Arbitrary size and format configuration
/// - MSAA support
/// - Resize functionality
/// - Image layout transitions
///
/// Example usage:
/// @code
/// // Offscreen framebuffer for shadow map (depth only)
/// Framebuffer::Config shadowConfig{};
/// shadowConfig.extent = {2048, 2048};
/// shadowConfig.depthFormat = VK_FORMAT_D32_SFLOAT;
/// shadowConfig.createColorAttachment = false;
/// shadowConfig.createDepthAttachment = true;
///
/// auto shadowFBResult = Framebuffer::create(context, &memManager, shadowConfig);
/// if (shadowFBResult.isSuccess()) {
///     Framebuffer& shadowFB = *shadowFBResult.value();
///
///     // Render shadow map
///     RenderPassInfo rpInfo{};
///     AttachmentInfo depthAttachment{};
///     depthAttachment.imageView = shadowFB.getDepthView();
///     depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
///     depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
///     depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
///     depthAttachment.clearValue.depthStencil = {1.0f, 0};
///     rpInfo.depthAttachment = depthAttachment;
///     rpInfo.renderArea = {{0, 0}, shadowConfig.extent};
///
///     RenderPass::begin(cmd, rpInfo);
///     // Render scene from light's perspective...
///     RenderPass::end(cmd);
/// }
///
/// // Offscreen rendering with color and depth
/// Framebuffer::Config offscreenConfig{};
/// offscreenConfig.extent = {1920, 1080};
/// offscreenConfig.colorFormat = VK_FORMAT_R8G8B8A8_SRGB;
/// offscreenConfig.depthFormat = VK_FORMAT_D32_SFLOAT;
/// offscreenConfig.createColorAttachment = true;
/// offscreenConfig.createDepthAttachment = true;
/// offscreenConfig.samples = VK_SAMPLE_COUNT_4_BIT;  // 4x MSAA
///
/// auto offscreenFBResult = Framebuffer::create(context, &memManager, offscreenConfig);
/// // Use for rendering, then resize if needed
/// offscreenFB->resize({2560, 1440});
/// @endcode
class Framebuffer {
public:
    /// Configuration for framebuffer creation
    struct Config {
        VkExtent2D extent = {0, 0};                             ///< Framebuffer dimensions
        VkFormat colorFormat = VK_FORMAT_R8G8B8A8_SRGB;         ///< Color attachment format
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;            ///< Depth attachment format
        bool createColorAttachment = true;                      ///< Create color attachment
        bool createDepthAttachment = true;                      ///< Create depth attachment
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;  ///< MSAA sample count
    };

    /// Create a framebuffer with specified configuration
    /// @param context Vulkan context (must outlive this framebuffer)
    /// @param memManager Memory manager for allocation (must outlive this framebuffer)
    /// @param config Framebuffer configuration
    /// @return Result containing the framebuffer or error code
    static core::Result<std::unique_ptr<Framebuffer>>
    create(VkContext* context, VkMemoryManager* memManager, const Config& config);

    /// Destructor - cleans up all resources
    ~Framebuffer();

    // Non-copyable, non-movable
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&&) = delete;
    Framebuffer& operator=(Framebuffer&&) = delete;

    /// Get color attachment image view
    /// @return Color image view handle (VK_NULL_HANDLE if not created)
    VkImageView getColorView() const noexcept { return colorView_; }

    /// Get depth attachment image view
    /// @return Depth image view handle (VK_NULL_HANDLE if not created)
    VkImageView getDepthView() const noexcept { return depthView_; }

    /// Get framebuffer extent
    /// @return Framebuffer dimensions
    VkExtent2D getExtent() const noexcept { return extent_; }

    /// Get color attachment image
    /// @return Color image handle (VK_NULL_HANDLE if not created)
    VkImage getColorImage() const noexcept { return colorImage_.image; }

    /// Get depth attachment image
    /// @return Depth image handle (VK_NULL_HANDLE if not created)
    VkImage getDepthImage() const noexcept { return depthImage_.image; }

    /// Get current color attachment layout
    /// @return Current color image layout
    VkImageLayout getColorLayout() const noexcept { return colorLayout_; }

    /// Get current depth attachment layout
    /// @return Current depth image layout
    VkImageLayout getDepthLayout() const noexcept { return depthLayout_; }

    /// Resize the framebuffer
    /// Destroys old attachments and creates new ones with the specified extent.
    /// Layout states are reset to UNDEFINED.
    /// @param newExtent New framebuffer dimensions
    /// @return Result indicating success or failure
    core::Result<void> resize(VkExtent2D newExtent);

    /// Transition color attachment to a new layout
    /// Records a pipeline barrier to transition the color image layout.
    /// @param cmd Command buffer to record the barrier
    /// @param newLayout Target image layout
    void transitionColorLayout(VkCommandBuffer cmd, VkImageLayout newLayout) noexcept;

    /// Transition depth attachment to a new layout
    /// Records a pipeline barrier to transition the depth image layout.
    /// @param cmd Command buffer to record the barrier
    /// @param newLayout Target image layout
    void transitionDepthLayout(VkCommandBuffer cmd, VkImageLayout newLayout) noexcept;

private:
    /// Private constructor - use create() instead
    Framebuffer(VkContext* context, VkMemoryManager* memManager, const Config& config);

    /// Initialize framebuffer resources
    /// @return Result indicating success or failure
    core::Result<void> initialize();

    /// Create color attachment (image and image view)
    /// @return Result indicating success or failure
    core::Result<void> createColorAttachment();

    /// Create depth attachment (image and image view)
    /// @return Result indicating success or failure
    core::Result<void> createDepthAttachment();

    /// Cleanup all resources
    void cleanup() noexcept;

    VkContext* context_;           ///< Vulkan context (not owned)
    VkMemoryManager* memManager_;  ///< Memory manager (not owned)
    Config config_;                ///< Framebuffer configuration
    VkExtent2D extent_;            ///< Current framebuffer dimensions

    VkMemoryManager::Image colorImage_;                      ///< Color attachment image
    VkImageView colorView_ = VK_NULL_HANDLE;                 ///< Color attachment image view
    VkImageLayout colorLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;  ///< Current color layout

    VkMemoryManager::Image depthImage_;                      ///< Depth attachment image
    VkImageView depthView_ = VK_NULL_HANDLE;                 ///< Depth attachment image view
    VkImageLayout depthLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;  ///< Current depth layout
};

/// Framebuffer wrapper for swapchain images
/// Manages depth buffer for swapchain rendering. Color attachments are provided
/// by the swapchain itself (via getColorView).
///
/// Example usage:
/// @code
/// SwapchainFramebuffer swapchainFB(context, &memManager, &swapchain);
///
/// // Main rendering loop
/// while (!glfwWindowShouldClose(window)) {
///     auto acquireResult = swapchain.acquireNextImage(imageAvailable.get());
///     if (acquireResult.needsResize) {
///         // Handle resize
///         swapchainFB.resize(newExtent);
///         continue;
///     }
///
///     uint32_t imageIndex = acquireResult.imageIndex;
///
///     cmd.begin();
///     RenderPass::beginSimple(cmd.get(),
///                            swapchainFB.getColorView(imageIndex),
///                            swapchainFB.getDepthView(),
///                            swapchain.getExtent());
///     // Render scene...
///     RenderPass::end(cmd.get());
///     cmd.end();
///
///     // Submit and present...
/// }
/// @endcode
class SwapchainFramebuffer {
public:
    /// Create a swapchain framebuffer
    /// Creates a depth buffer matching the swapchain extent.
    /// @param context Vulkan context (must outlive this framebuffer)
    /// @param memManager Memory manager for allocation (must outlive this framebuffer)
    /// @param swapchain Swapchain providing color attachments (must outlive this framebuffer)
    /// @return Result containing the swapchain framebuffer or error code
    static core::Result<std::unique_ptr<SwapchainFramebuffer>>
    create(VkContext* context, VkMemoryManager* memManager, Swapchain* swapchain);

    /// Destructor - cleans up depth buffer
    ~SwapchainFramebuffer();

    // Non-copyable, non-movable
    SwapchainFramebuffer(const SwapchainFramebuffer&) = delete;
    SwapchainFramebuffer& operator=(const SwapchainFramebuffer&) = delete;
    SwapchainFramebuffer(SwapchainFramebuffer&&) = delete;
    SwapchainFramebuffer& operator=(SwapchainFramebuffer&&) = delete;

    /// Get color attachment image view for a specific swapchain image
    /// @param imageIndex Index of the swapchain image
    /// @return Color image view from swapchain
    VkImageView getColorView(uint32_t imageIndex) const;

    /// Get shared depth attachment image view
    /// @return Depth image view handle
    VkImageView getDepthView() const noexcept { return depthView_; }

    /// Get depth attachment image
    /// @return Depth image handle
    VkImage getDepthImage() const noexcept { return depthImage_.image; }

    /// Get current depth attachment layout
    /// @return Current depth image layout
    VkImageLayout getDepthLayout() const noexcept { return depthLayout_; }

    /// Resize the depth buffer to match new swapchain extent
    /// @param newExtent New framebuffer dimensions
    /// @return Result indicating success or failure
    core::Result<void> resize(VkExtent2D newExtent);

    /// Transition depth attachment to a new layout
    /// @param cmd Command buffer to record the barrier
    /// @param newLayout Target image layout
    void transitionDepthLayout(VkCommandBuffer cmd, VkImageLayout newLayout) noexcept;

private:
    /// Private constructor - use create() instead
    SwapchainFramebuffer(VkContext* context, VkMemoryManager* memManager, Swapchain* swapchain);

    /// Initialize depth buffer
    /// @return Result indicating success or failure
    core::Result<void> initialize();

    /// Create depth buffer (image and image view)
    /// @return Result indicating success or failure
    core::Result<void> createDepthBuffer();

    /// Cleanup depth buffer resources
    void cleanup() noexcept;

    VkContext* context_;           ///< Vulkan context (not owned)
    VkMemoryManager* memManager_;  ///< Memory manager (not owned)
    Swapchain* swapchain_;         ///< Swapchain (not owned)

    VkMemoryManager::Image depthImage_;                      ///< Depth attachment image
    VkImageView depthView_ = VK_NULL_HANDLE;                 ///< Depth attachment image view
    VkImageLayout depthLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;  ///< Current depth layout
};

}  // namespace axiom::gpu
