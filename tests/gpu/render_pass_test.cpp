#include "axiom/gpu/render_pass.hpp"
#include "axiom/gpu/vk_command.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_memory.hpp"

#include <gtest/gtest.h>

using namespace axiom::gpu;
using namespace axiom::core;
using namespace axiom::math;

// Test fixture for RenderPass tests
class RenderPassTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create Vulkan context
        auto contextResult = VkContext::create();
        if (contextResult.isSuccess()) {
            context_ = std::move(contextResult.value());
        } else {
            GTEST_SKIP() << "Vulkan not available: " << contextResult.errorMessage()
                         << " (this is expected in CI environments without GPU)";
        }

        // Create memory manager
        auto memoryResult = VkMemoryManager::create(context_.get());
        if (memoryResult.isSuccess()) {
            memory_ = std::move(memoryResult.value());
        } else {
            GTEST_SKIP() << "Failed to create memory manager: " << memoryResult.errorMessage();
        }

        // Create command pool
        commandPool_ = std::make_unique<CommandPool>(context_.get(),
                                                     context_->getGraphicsQueueFamily());

        // Create test images for attachments
        createTestImages();
    }

    void TearDown() override {
        // Destroy test images
        if (colorImageView_ != VK_NULL_HANDLE) {
            vkDestroyImageView(context_->getDevice(), colorImageView_, nullptr);
        }
        if (depthImageView_ != VK_NULL_HANDLE) {
            vkDestroyImageView(context_->getDevice(), depthImageView_, nullptr);
        }
        if (colorImageView2_ != VK_NULL_HANDLE) {
            vkDestroyImageView(context_->getDevice(), colorImageView2_, nullptr);
        }
        if (colorImageView3_ != VK_NULL_HANDLE) {
            vkDestroyImageView(context_->getDevice(), colorImageView3_, nullptr);
        }

        if (colorImage_ != VK_NULL_HANDLE) {
            VkMemoryManager::Image img{colorImage_, colorAllocation_};
            memory_->destroyImage(img);
        }
        if (depthImage_ != VK_NULL_HANDLE) {
            VkMemoryManager::Image img{depthImage_, depthAllocation_};
            memory_->destroyImage(img);
        }
        if (colorImage2_ != VK_NULL_HANDLE) {
            VkMemoryManager::Image img{colorImage2_, colorAllocation2_};
            memory_->destroyImage(img);
        }
        if (colorImage3_ != VK_NULL_HANDLE) {
            VkMemoryManager::Image img{colorImage3_, colorAllocation3_};
            memory_->destroyImage(img);
        }

        commandPool_.reset();
        memory_.reset();
        context_.reset();
    }

    void createTestImages() {
        const VkExtent3D extent{testWidth_, testHeight_, 1};

        // Create color image
        VkMemoryManager::ImageCreateInfo colorInfo{};
        colorInfo.extent = extent;
        colorInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        colorInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        auto colorResult = memory_->createImage(colorInfo);
        if (colorResult.isSuccess()) {
            auto image = colorResult.value();
            colorImage_ = image.image;
            colorAllocation_ = image.allocation;
            colorImageView_ = createImageView(colorImage_, VK_FORMAT_R8G8B8A8_UNORM,
                                              VK_IMAGE_ASPECT_COLOR_BIT);
        }

        // Create depth image
        VkMemoryManager::ImageCreateInfo depthInfo{};
        depthInfo.extent = extent;
        depthInfo.format = VK_FORMAT_D32_SFLOAT;
        depthInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        auto depthResult = memory_->createImage(depthInfo);
        if (depthResult.isSuccess()) {
            auto image = depthResult.value();
            depthImage_ = image.image;
            depthAllocation_ = image.allocation;
            depthImageView_ = createImageView(depthImage_, VK_FORMAT_D32_SFLOAT,
                                              VK_IMAGE_ASPECT_DEPTH_BIT);
        }

        // Create additional color images for MRT tests
        VkMemoryManager::ImageCreateInfo colorInfo2{};
        colorInfo2.extent = extent;
        colorInfo2.format = VK_FORMAT_R8G8B8A8_UNORM;
        colorInfo2.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        auto colorResult2 = memory_->createImage(colorInfo2);
        if (colorResult2.isSuccess()) {
            auto image = colorResult2.value();
            colorImage2_ = image.image;
            colorAllocation2_ = image.allocation;
            colorImageView2_ = createImageView(colorImage2_, VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_IMAGE_ASPECT_COLOR_BIT);
        }

        VkMemoryManager::ImageCreateInfo colorInfo3{};
        colorInfo3.extent = extent;
        colorInfo3.format = VK_FORMAT_R8G8B8A8_UNORM;
        colorInfo3.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        auto colorResult3 = memory_->createImage(colorInfo3);
        if (colorResult3.isSuccess()) {
            auto image = colorResult3.value();
            colorImage3_ = image.image;
            colorAllocation3_ = image.allocation;
            colorImageView3_ = createImageView(colorImage3_, VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectMask;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView = VK_NULL_HANDLE;
        vkCreateImageView(context_->getDevice(), &viewInfo, nullptr, &imageView);
        return imageView;
    }

    std::unique_ptr<VkContext> context_;
    std::unique_ptr<VkMemoryManager> memory_;
    std::unique_ptr<CommandPool> commandPool_;

    // Test images
    VkImage colorImage_ = VK_NULL_HANDLE;
    void* colorAllocation_ = nullptr;
    VkImageView colorImageView_ = VK_NULL_HANDLE;

    VkImage depthImage_ = VK_NULL_HANDLE;
    void* depthAllocation_ = nullptr;
    VkImageView depthImageView_ = VK_NULL_HANDLE;

    VkImage colorImage2_ = VK_NULL_HANDLE;
    void* colorAllocation2_ = nullptr;
    VkImageView colorImageView2_ = VK_NULL_HANDLE;

    VkImage colorImage3_ = VK_NULL_HANDLE;
    void* colorAllocation3_ = nullptr;
    VkImageView colorImageView3_ = VK_NULL_HANDLE;

    static constexpr uint32_t testWidth_ = 1280;
    static constexpr uint32_t testHeight_ = 720;
};

// Test basic render pass begin and end
TEST_F(RenderPassTest, BasicBeginEnd) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(commandPool_, nullptr);
    ASSERT_NE(colorImageView_, VK_NULL_HANDLE);

    VkCommandBuffer cmd = commandPool_->allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Begin render pass
    RenderPassInfo info{};
    AttachmentInfo colorAttachment{};
    colorAttachment.imageView = colorImageView_;
    colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    info.colorAttachments.push_back(colorAttachment);
    info.renderArea = {{0, 0}, {testWidth_, testHeight_}};

    RenderPass::begin(cmd, info);
    // Render pass is active here
    RenderPass::end(cmd);

    // End command buffer
    vkEndCommandBuffer(cmd);

    // Test passed if no Vulkan validation errors occurred
}

// Test render pass with depth attachment
TEST_F(RenderPassTest, WithDepthAttachment) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(commandPool_, nullptr);
    ASSERT_NE(colorImageView_, VK_NULL_HANDLE);
    ASSERT_NE(depthImageView_, VK_NULL_HANDLE);

    VkCommandBuffer cmd = commandPool_->allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Begin render pass with color and depth
    RenderPassInfo info{};

    AttachmentInfo colorAttachment{};
    colorAttachment.imageView = colorImageView_;
    colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.1f, 0.2f, 0.3f, 1.0f}};
    info.colorAttachments.push_back(colorAttachment);

    AttachmentInfo depthAttachment{};
    depthAttachment.imageView = depthImageView_;
    depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};
    info.depthAttachment = depthAttachment;

    info.renderArea = {{0, 0}, {testWidth_, testHeight_}};

    RenderPass::begin(cmd, info);
    RenderPass::end(cmd);

    vkEndCommandBuffer(cmd);
}

// Test multiple color attachments (MRT)
TEST_F(RenderPassTest, MultipleColorAttachments) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(commandPool_, nullptr);
    ASSERT_NE(colorImageView_, VK_NULL_HANDLE);
    ASSERT_NE(colorImageView2_, VK_NULL_HANDLE);
    ASSERT_NE(colorImageView3_, VK_NULL_HANDLE);

    VkCommandBuffer cmd = commandPool_->allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Begin render pass with 3 color attachments (G-Buffer style)
    RenderPassInfo info{};

    AttachmentInfo attachment1{};
    attachment1.imageView = colorImageView_;
    attachment1.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment1.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment1.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment1.clearValue.color = {{1.0f, 0.0f, 0.0f, 1.0f}};
    info.colorAttachments.push_back(attachment1);

    AttachmentInfo attachment2{};
    attachment2.imageView = colorImageView2_;
    attachment2.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment2.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment2.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment2.clearValue.color = {{0.0f, 1.0f, 0.0f, 1.0f}};
    info.colorAttachments.push_back(attachment2);

    AttachmentInfo attachment3{};
    attachment3.imageView = colorImageView3_;
    attachment3.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment3.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment3.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment3.clearValue.color = {{0.0f, 0.0f, 1.0f, 1.0f}};
    info.colorAttachments.push_back(attachment3);

    info.renderArea = {{0, 0}, {testWidth_, testHeight_}};

    RenderPass::begin(cmd, info);
    RenderPass::end(cmd);

    vkEndCommandBuffer(cmd);
}

// Test beginSimple helper
TEST_F(RenderPassTest, BeginSimpleHelper) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(commandPool_, nullptr);
    ASSERT_NE(colorImageView_, VK_NULL_HANDLE);
    ASSERT_NE(depthImageView_, VK_NULL_HANDLE);

    VkCommandBuffer cmd = commandPool_->allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Use simple helper with custom clear color
    RenderPass::beginSimple(cmd, colorImageView_, depthImageView_, {testWidth_, testHeight_},
                            Vec4(0.1f, 0.2f, 0.3f, 1.0f));
    RenderPass::end(cmd);

    vkEndCommandBuffer(cmd);
}

// Test beginSimpleNoClear helper
TEST_F(RenderPassTest, BeginSimpleNoClearHelper) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(commandPool_, nullptr);
    ASSERT_NE(colorImageView_, VK_NULL_HANDLE);
    ASSERT_NE(depthImageView_, VK_NULL_HANDLE);

    VkCommandBuffer cmd = commandPool_->allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Use simple helper without clearing (loads existing contents)
    RenderPass::beginSimpleNoClear(cmd, colorImageView_, depthImageView_,
                                   {testWidth_, testHeight_});
    RenderPass::end(cmd);

    vkEndCommandBuffer(cmd);
}

// Test color only (no depth)
TEST_F(RenderPassTest, ColorOnlyNoDepth) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(commandPool_, nullptr);
    ASSERT_NE(colorImageView_, VK_NULL_HANDLE);

    VkCommandBuffer cmd = commandPool_->allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Simple helper with no depth attachment
    RenderPass::beginSimple(cmd, colorImageView_, VK_NULL_HANDLE, {testWidth_, testHeight_});
    RenderPass::end(cmd);

    vkEndCommandBuffer(cmd);
}

// Test different load operations
TEST_F(RenderPassTest, DifferentLoadOperations) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(commandPool_, nullptr);
    ASSERT_NE(colorImageView_, VK_NULL_HANDLE);

    VkCommandBuffer cmd = commandPool_->allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Test LOAD operation
    {
        RenderPassInfo info{};
        AttachmentInfo colorAttachment{};
        colorAttachment.imageView = colorImageView_;
        colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        info.colorAttachments.push_back(colorAttachment);
        info.renderArea = {{0, 0}, {testWidth_, testHeight_}};

        RenderPass::begin(cmd, info);
        RenderPass::end(cmd);
    }

    // Test DONT_CARE operation
    {
        RenderPassInfo info{};
        AttachmentInfo colorAttachment{};
        colorAttachment.imageView = colorImageView_;
        colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        info.colorAttachments.push_back(colorAttachment);
        info.renderArea = {{0, 0}, {testWidth_, testHeight_}};

        RenderPass::begin(cmd, info);
        RenderPass::end(cmd);
    }

    vkEndCommandBuffer(cmd);
}
