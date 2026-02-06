#pragma once

#include "axiom/math/vec4.hpp"

#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

namespace axiom::gpu {

/// Attachment information for dynamic rendering
/// Describes a single color, depth, or stencil attachment including its image view,
/// layout, load/store operations, clear value, and optional MSAA resolve target.
///
/// @note When using MSAA resolve with stencil attachments, resolveMode must be set to
/// VK_RESOLVE_MODE_SAMPLE_ZERO_BIT or another stencil-compatible mode. The default
/// VK_RESOLVE_MODE_AVERAGE_BIT is only valid for color and depth attachments (if supported
/// by the device for depth).
struct AttachmentInfo {
    VkImageView imageView = VK_NULL_HANDLE;  ///< Image view to render to
    VkImageLayout layout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  ///< Image layout during rendering
    VkAttachmentLoadOp loadOp =
        VK_ATTACHMENT_LOAD_OP_CLEAR;  ///< Load operation (clear/load/don't care)
    VkAttachmentStoreOp storeOp =
        VK_ATTACHMENT_STORE_OP_STORE;  ///< Store operation (store/don't care)
    VkClearValue clearValue = {};      ///< Clear value (color or depth/stencil)

    // For MSAA resolve
    VkResolveModeFlagBits resolveMode =
        VK_RESOLVE_MODE_AVERAGE_BIT;  ///< Resolve mode (AVERAGE for color/depth, SAMPLE_ZERO
                                      ///< for stencil)
    VkImageView resolveImageView = VK_NULL_HANDLE;  ///< Resolve target for MSAA (optional)
    VkImageLayout resolveLayout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  ///< Resolve target layout
};

/// Render pass configuration for dynamic rendering
/// Contains all attachments (color, depth, stencil) and rendering area configuration.
/// Used with VK_KHR_dynamic_rendering to define rendering without pre-created render pass objects.
struct RenderPassInfo {
    std::vector<AttachmentInfo> colorAttachments;     ///< Color attachments (0 or more)
    std::optional<AttachmentInfo> depthAttachment;    ///< Depth attachment (optional)
    std::optional<AttachmentInfo> stencilAttachment;  ///< Stencil attachment (optional)
    VkRect2D renderArea = {};                         ///< Rendering area (offset and extent)
    uint32_t layerCount = 1;                          ///< Number of layers for layered rendering
};

/// Render pass abstraction using dynamic rendering
/// This class provides a simplified interface for beginning and ending rendering operations
/// using VK_KHR_dynamic_rendering. Unlike traditional Vulkan render passes, dynamic rendering
/// allows defining attachments on-the-fly without pre-created VkRenderPass objects.
///
/// Advantages of dynamic rendering:
/// - No need to pre-create render pass objects
/// - Specify attachment configuration on the fly
/// - Reduced code complexity
/// - More intuitive API
///
/// Features:
/// - Multiple color attachments (MRT - Multiple Render Targets)
/// - Depth and stencil attachments
/// - MSAA resolve support
/// - Load/store operations per attachment
/// - Clear values per attachment
/// - Layered rendering support
///
/// Example usage:
/// @code
/// // Full control
/// RenderPassInfo rpInfo{};
///
/// // Color attachment
/// AttachmentInfo colorAttachment{};
/// colorAttachment.imageView = swapchain.getImageView(imageIndex);
/// colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
/// colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
/// colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
/// colorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
/// rpInfo.colorAttachments.push_back(colorAttachment);
///
/// // Depth attachment
/// AttachmentInfo depthAttachment{};
/// depthAttachment.imageView = depthImageView;
/// depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
/// depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
/// depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
/// depthAttachment.clearValue.depthStencil = {1.0f, 0};
/// rpInfo.depthAttachment = depthAttachment;
///
/// // Render area
/// rpInfo.renderArea = {{0, 0}, extent};
///
/// // Begin rendering
/// cmd.begin();
/// RenderPass::begin(cmd.get(), rpInfo);
///
/// // Record drawing commands...
/// pipeline->bind(cmd.get());
/// vkCmdDraw(cmd, vertexCount, 1, 0, 0);
///
/// RenderPass::end(cmd.get());
/// cmd.end();
///
/// // Simple helper for common case
/// cmd.begin();
/// RenderPass::beginSimple(cmd.get(),
///                        colorView,
///                        depthView,
///                        extent,
///                        math::Vec4(0.1f, 0.1f, 0.1f, 1.0f));  // Clear to dark gray
///
/// // Draw...
///
/// RenderPass::end(cmd.get());
/// cmd.end();
/// @endcode
///
/// Multiple color attachments (G-Buffer rendering):
/// @code
/// RenderPassInfo rpInfo{};
/// rpInfo.colorAttachments = {
///     {gBufferAlbedo,   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, /* ... */},
///     {gBufferNormal,   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, /* ... */},
///     {gBufferPosition, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, /* ... */}
/// };
/// rpInfo.depthAttachment = depthAttachment;
/// rpInfo.renderArea = {{0, 0}, extent};
///
/// RenderPass::begin(cmd, rpInfo);
/// // Fragment shader outputs to multiple locations
/// RenderPass::end(cmd);
/// @endcode
class RenderPass {
public:
    /// Begin a render pass with full control
    /// Starts dynamic rendering with the specified attachment configuration.
    /// Must be followed by RenderPass::end() before submitting the command buffer.
    /// @param cmd Command buffer to record rendering commands
    /// @param info Render pass configuration with attachments and render area
    static void begin(VkCommandBuffer cmd, const RenderPassInfo& info) noexcept;

    /// End the current render pass
    /// Completes dynamic rendering started by begin() or beginSimple().
    /// @param cmd Command buffer (must be the same one used in begin)
    static void end(VkCommandBuffer cmd) noexcept;

    /// Begin a simple render pass with color and depth attachments
    /// Convenience method for the common case of single color + depth rendering.
    /// Both attachments are cleared to the specified values.
    /// @param cmd Command buffer to record rendering commands
    /// @param colorView Color attachment image view
    /// @param depthView Depth attachment image view (can be VK_NULL_HANDLE for no depth)
    /// @param extent Rendering extent (width and height)
    /// @param clearColor Clear color value (default black)
    static void beginSimple(VkCommandBuffer cmd, VkImageView colorView, VkImageView depthView,
                            VkExtent2D extent,
                            const math::Vec4& clearColor = math::Vec4(0.0f, 0.0f, 0.0f,
                                                                      1.0f)) noexcept;

    /// Begin a simple render pass without clearing attachments
    /// Convenience method that loads existing attachment contents instead of clearing.
    /// @param cmd Command buffer to record rendering commands
    /// @param colorView Color attachment image view
    /// @param depthView Depth attachment image view (can be VK_NULL_HANDLE for no depth)
    /// @param extent Rendering extent (width and height)
    static void beginSimpleNoClear(VkCommandBuffer cmd, VkImageView colorView,
                                   VkImageView depthView, VkExtent2D extent) noexcept;
};

}  // namespace axiom::gpu
