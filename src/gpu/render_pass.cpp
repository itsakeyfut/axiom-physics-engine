#include "axiom/gpu/render_pass.hpp"

#include <algorithm>

namespace axiom::gpu {

void RenderPass::begin(VkCommandBuffer cmd, const RenderPassInfo& info) noexcept {
    // Prepare color attachment descriptions
    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    colorAttachments.reserve(info.colorAttachments.size());

    for (const auto& attachment : info.colorAttachments) {
        VkRenderingAttachmentInfo attachmentInfo{};
        attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachmentInfo.imageView = attachment.imageView;
        attachmentInfo.imageLayout = attachment.layout;
        attachmentInfo.loadOp = attachment.loadOp;
        attachmentInfo.storeOp = attachment.storeOp;
        attachmentInfo.clearValue = attachment.clearValue;

        // MSAA resolve
        if (attachment.resolveImageView != VK_NULL_HANDLE) {
            attachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
            attachmentInfo.resolveImageView = attachment.resolveImageView;
            attachmentInfo.resolveImageLayout = attachment.resolveLayout;
        } else {
            attachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
        }

        colorAttachments.push_back(attachmentInfo);
    }

    // Prepare depth attachment description (if present)
    VkRenderingAttachmentInfo depthAttachmentInfo{};
    VkRenderingAttachmentInfo* pDepthAttachment = nullptr;

    if (info.depthAttachment.has_value()) {
        const auto& attachment = info.depthAttachment.value();
        depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachmentInfo.imageView = attachment.imageView;
        depthAttachmentInfo.imageLayout = attachment.layout;
        depthAttachmentInfo.loadOp = attachment.loadOp;
        depthAttachmentInfo.storeOp = attachment.storeOp;
        depthAttachmentInfo.clearValue = attachment.clearValue;

        // MSAA resolve
        if (attachment.resolveImageView != VK_NULL_HANDLE) {
            depthAttachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
            depthAttachmentInfo.resolveImageView = attachment.resolveImageView;
            depthAttachmentInfo.resolveImageLayout = attachment.resolveLayout;
        } else {
            depthAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
        }

        pDepthAttachment = &depthAttachmentInfo;
    }

    // Prepare stencil attachment description (if present)
    VkRenderingAttachmentInfo stencilAttachmentInfo{};
    VkRenderingAttachmentInfo* pStencilAttachment = nullptr;

    if (info.stencilAttachment.has_value()) {
        const auto& attachment = info.stencilAttachment.value();
        stencilAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        stencilAttachmentInfo.imageView = attachment.imageView;
        stencilAttachmentInfo.imageLayout = attachment.layout;
        stencilAttachmentInfo.loadOp = attachment.loadOp;
        stencilAttachmentInfo.storeOp = attachment.storeOp;
        stencilAttachmentInfo.clearValue = attachment.clearValue;

        // MSAA resolve
        if (attachment.resolveImageView != VK_NULL_HANDLE) {
            stencilAttachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
            stencilAttachmentInfo.resolveImageView = attachment.resolveImageView;
            stencilAttachmentInfo.resolveImageLayout = attachment.resolveLayout;
        } else {
            stencilAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
        }

        pStencilAttachment = &stencilAttachmentInfo;
    }

    // Begin dynamic rendering
    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = info.renderArea;
    renderingInfo.layerCount = info.layerCount;
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderingInfo.pColorAttachments = colorAttachments.empty() ? nullptr : colorAttachments.data();
    renderingInfo.pDepthAttachment = pDepthAttachment;
    renderingInfo.pStencilAttachment = pStencilAttachment;

    vkCmdBeginRendering(cmd, &renderingInfo);
}

void RenderPass::end(VkCommandBuffer cmd) noexcept {
    vkCmdEndRendering(cmd);
}

void RenderPass::beginSimple(VkCommandBuffer cmd, VkImageView colorView, VkImageView depthView,
                             VkExtent2D extent, const math::Vec4& clearColor) noexcept {
    RenderPassInfo info{};

    // Color attachment
    if (colorView != VK_NULL_HANDLE) {
        AttachmentInfo colorAttachment{};
        colorAttachment.imageView = colorView;
        colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = {
            {clearColor.x, clearColor.y, clearColor.z, clearColor.w}};
        info.colorAttachments.push_back(colorAttachment);
    }

    // Depth attachment
    if (depthView != VK_NULL_HANDLE) {
        AttachmentInfo depthAttachment{};
        depthAttachment.imageView = depthView;
        depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue.depthStencil = {1.0f, 0};
        info.depthAttachment = depthAttachment;
    }

    // Render area
    info.renderArea.offset = {0, 0};
    info.renderArea.extent = extent;

    begin(cmd, info);
}

void RenderPass::beginSimpleNoClear(VkCommandBuffer cmd, VkImageView colorView,
                                    VkImageView depthView, VkExtent2D extent) noexcept {
    RenderPassInfo info{};

    // Color attachment (load existing contents)
    if (colorView != VK_NULL_HANDLE) {
        AttachmentInfo colorAttachment{};
        colorAttachment.imageView = colorView;
        colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        info.colorAttachments.push_back(colorAttachment);
    }

    // Depth attachment (load existing contents)
    if (depthView != VK_NULL_HANDLE) {
        AttachmentInfo depthAttachment{};
        depthAttachment.imageView = depthView;
        depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        info.depthAttachment = depthAttachment;
    }

    // Render area
    info.renderArea.offset = {0, 0};
    info.renderArea.extent = extent;

    begin(cmd, info);
}

}  // namespace axiom::gpu
