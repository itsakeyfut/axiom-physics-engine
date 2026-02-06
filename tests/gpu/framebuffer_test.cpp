#include "axiom/gpu/framebuffer.hpp"
#include "axiom/gpu/render_pass.hpp"
#include "axiom/gpu/vk_command.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_memory.hpp"
#include "axiom/gpu/vk_swapchain.hpp"

#include <gtest/gtest.h>

using namespace axiom::gpu;
using namespace axiom::core;

// Test fixture for Framebuffer tests
class FramebufferTest : public ::testing::Test {
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
    }

    void TearDown() override {
        commandPool_.reset();
        memory_.reset();
        context_.reset();
    }

    std::unique_ptr<VkContext> context_;
    std::unique_ptr<VkMemoryManager> memory_;
    std::unique_ptr<CommandPool> commandPool_;

    static constexpr uint32_t testWidth_ = 1280;
    static constexpr uint32_t testHeight_ = 720;
};

// Test creating a framebuffer with color and depth attachments
TEST_F(FramebufferTest, CreateColorAndDepth) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memory_, nullptr);

    Framebuffer::Config config{};
    config.extent = {testWidth_, testHeight_};
    config.colorFormat = VK_FORMAT_R8G8B8A8_SRGB;
    config.depthFormat = VK_FORMAT_D32_SFLOAT;
    config.createColorAttachment = true;
    config.createDepthAttachment = true;

    auto fbResult = Framebuffer::create(context_.get(), memory_.get(), config);
    ASSERT_TRUE(fbResult.isSuccess()) << fbResult.errorMessage();

    auto& fb = fbResult.value();
    EXPECT_NE(fb->getColorView(), VK_NULL_HANDLE);
    EXPECT_NE(fb->getDepthView(), VK_NULL_HANDLE);
    EXPECT_NE(fb->getColorImage(), VK_NULL_HANDLE);
    EXPECT_NE(fb->getDepthImage(), VK_NULL_HANDLE);
    EXPECT_EQ(fb->getExtent().width, testWidth_);
    EXPECT_EQ(fb->getExtent().height, testHeight_);
    VkImageLayout initialColorLayout = fb->getColorLayout();
    VkImageLayout initialDepthLayout = fb->getDepthLayout();
    EXPECT_EQ(initialColorLayout, VK_IMAGE_LAYOUT_UNDEFINED);
    EXPECT_EQ(initialDepthLayout, VK_IMAGE_LAYOUT_UNDEFINED);
}

// Test creating a framebuffer with color only (no depth)
TEST_F(FramebufferTest, CreateColorOnly) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memory_, nullptr);

    Framebuffer::Config config{};
    config.extent = {testWidth_, testHeight_};
    config.colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    config.createColorAttachment = true;
    config.createDepthAttachment = false;

    auto fbResult = Framebuffer::create(context_.get(), memory_.get(), config);
    ASSERT_TRUE(fbResult.isSuccess());

    auto& fb = fbResult.value();
    EXPECT_NE(fb->getColorView(), VK_NULL_HANDLE);
    EXPECT_EQ(fb->getDepthView(), VK_NULL_HANDLE);
    EXPECT_NE(fb->getColorImage(), VK_NULL_HANDLE);
    EXPECT_EQ(fb->getDepthImage(), VK_NULL_HANDLE);
}

// Test creating a framebuffer with depth only (shadow map use case)
TEST_F(FramebufferTest, CreateDepthOnly) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memory_, nullptr);

    Framebuffer::Config config{};
    config.extent = {2048, 2048};  // Shadow map resolution
    config.depthFormat = VK_FORMAT_D32_SFLOAT;
    config.createColorAttachment = false;
    config.createDepthAttachment = true;

    auto fbResult = Framebuffer::create(context_.get(), memory_.get(), config);
    ASSERT_TRUE(fbResult.isSuccess());

    auto& fb = fbResult.value();
    EXPECT_EQ(fb->getColorView(), VK_NULL_HANDLE);
    EXPECT_NE(fb->getDepthView(), VK_NULL_HANDLE);
    EXPECT_EQ(fb->getColorImage(), VK_NULL_HANDLE);
    EXPECT_NE(fb->getDepthImage(), VK_NULL_HANDLE);
    EXPECT_EQ(fb->getExtent().width, 2048);
    EXPECT_EQ(fb->getExtent().height, 2048);
}

// Test different color formats
TEST_F(FramebufferTest, DifferentColorFormats) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memory_, nullptr);

    std::vector<VkFormat> formats = {
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R16G16B16A16_SFLOAT,
    };

    for (VkFormat format : formats) {
        Framebuffer::Config config{};
        config.extent = {testWidth_, testHeight_};
        config.colorFormat = format;
        config.createColorAttachment = true;
        config.createDepthAttachment = false;

        auto fbResult = Framebuffer::create(context_.get(), memory_.get(), config);
        ASSERT_TRUE(fbResult.isSuccess()) << "Failed with format: " << format;

        auto& fb = fbResult.value();
        EXPECT_NE(fb->getColorView(), VK_NULL_HANDLE);
    }
}

// Test different depth formats
TEST_F(FramebufferTest, DifferentDepthFormats) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memory_, nullptr);

    std::vector<VkFormat> formats = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
    };

    for (VkFormat format : formats) {
        Framebuffer::Config config{};
        config.extent = {testWidth_, testHeight_};
        config.depthFormat = format;
        config.createColorAttachment = false;
        config.createDepthAttachment = true;

        auto fbResult = Framebuffer::create(context_.get(), memory_.get(), config);
        ASSERT_TRUE(fbResult.isSuccess()) << "Failed with format: " << format;

        auto& fb = fbResult.value();
        EXPECT_NE(fb->getDepthView(), VK_NULL_HANDLE);
    }
}

// Test framebuffer resize
TEST_F(FramebufferTest, Resize) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memory_, nullptr);

    Framebuffer::Config config{};
    config.extent = {testWidth_, testHeight_};
    config.createColorAttachment = true;
    config.createDepthAttachment = true;

    auto fbResult = Framebuffer::create(context_.get(), memory_.get(), config);
    ASSERT_TRUE(fbResult.isSuccess());

    auto& fb = fbResult.value();
    VkImageView oldColorView = fb->getColorView();
    VkImageView oldDepthView = fb->getDepthView();

    // Resize to new dimensions
    VkExtent2D newExtent = {1920, 1080};
    auto resizeResult = fb->resize(newExtent);
    ASSERT_TRUE(resizeResult.isSuccess());

    // Verify new extent
    EXPECT_EQ(fb->getExtent().width, 1920);
    EXPECT_EQ(fb->getExtent().height, 1080);

    // Verify new image views were created (different handles)
    VkImageView newColorView = fb->getColorView();
    VkImageView newDepthView = fb->getDepthView();
    EXPECT_NE(newColorView, VK_NULL_HANDLE);
    EXPECT_NE(newDepthView, VK_NULL_HANDLE);
    EXPECT_NE(newColorView, oldColorView);
    EXPECT_NE(newDepthView, oldDepthView);

    // Layouts should be reset to UNDEFINED after resize
    VkImageLayout colorLayout = fb->getColorLayout();
    VkImageLayout depthLayout = fb->getDepthLayout();
    EXPECT_EQ(colorLayout, VK_IMAGE_LAYOUT_UNDEFINED);
    EXPECT_EQ(depthLayout, VK_IMAGE_LAYOUT_UNDEFINED);
}

// Test color layout transitions
TEST_F(FramebufferTest, ColorLayoutTransitions) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memory_, nullptr);
    ASSERT_NE(commandPool_, nullptr);

    Framebuffer::Config config{};
    config.extent = {testWidth_, testHeight_};
    config.createColorAttachment = true;
    config.createDepthAttachment = false;

    auto fbResult = Framebuffer::create(context_.get(), memory_.get(), config);
    ASSERT_TRUE(fbResult.isSuccess());

    auto& fb = fbResult.value();
    VkCommandBuffer cmd = commandPool_->allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Transition UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
    VkImageLayout layout1 = fb->getColorLayout();
    EXPECT_EQ(layout1, VK_IMAGE_LAYOUT_UNDEFINED);
    fb->transitionColorLayout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkImageLayout layout2 = fb->getColorLayout();
    EXPECT_EQ(layout2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Transition COLOR_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    fb->transitionColorLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkImageLayout layout3 = fb->getColorLayout();
    EXPECT_EQ(layout3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Transition SHADER_READ_ONLY_OPTIMAL -> TRANSFER_SRC_OPTIMAL
    fb->transitionColorLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VkImageLayout layout4 = fb->getColorLayout();
    EXPECT_EQ(layout4, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    vkEndCommandBuffer(cmd);
}

// Test depth layout transitions
TEST_F(FramebufferTest, DepthLayoutTransitions) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memory_, nullptr);
    ASSERT_NE(commandPool_, nullptr);

    Framebuffer::Config config{};
    config.extent = {testWidth_, testHeight_};
    config.createColorAttachment = false;
    config.createDepthAttachment = true;

    auto fbResult = Framebuffer::create(context_.get(), memory_.get(), config);
    ASSERT_TRUE(fbResult.isSuccess());

    auto& fb = fbResult.value();
    VkCommandBuffer cmd = commandPool_->allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Transition UNDEFINED -> DEPTH_ATTACHMENT_OPTIMAL
    VkImageLayout depthLayout1 = fb->getDepthLayout();
    EXPECT_EQ(depthLayout1, VK_IMAGE_LAYOUT_UNDEFINED);
    fb->transitionDepthLayout(cmd, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    VkImageLayout depthLayout2 = fb->getDepthLayout();
    EXPECT_EQ(depthLayout2, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    // Transition DEPTH_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    fb->transitionDepthLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkImageLayout depthLayout3 = fb->getDepthLayout();
    EXPECT_EQ(depthLayout3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkEndCommandBuffer(cmd);
}

// Test using framebuffer with render pass
TEST_F(FramebufferTest, UseWithRenderPass) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memory_, nullptr);
    ASSERT_NE(commandPool_, nullptr);

    Framebuffer::Config config{};
    config.extent = {testWidth_, testHeight_};
    config.createColorAttachment = true;
    config.createDepthAttachment = true;

    auto fbResult = Framebuffer::create(context_.get(), memory_.get(), config);
    ASSERT_TRUE(fbResult.isSuccess());

    auto& fb = fbResult.value();
    VkCommandBuffer cmd = commandPool_->allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Use framebuffer with render pass
    RenderPassInfo rpInfo{};

    AttachmentInfo colorAttachment{};
    colorAttachment.imageView = fb->getColorView();
    colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.1f, 0.2f, 0.3f, 1.0f}};
    rpInfo.colorAttachments.push_back(colorAttachment);

    AttachmentInfo depthAttachment{};
    depthAttachment.imageView = fb->getDepthView();
    depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};
    rpInfo.depthAttachment = depthAttachment;

    rpInfo.renderArea = {{0, 0}, config.extent};

    RenderPass::begin(cmd, rpInfo);
    // Draw commands would go here...
    RenderPass::end(cmd);

    vkEndCommandBuffer(cmd);
}

// Test invalid configurations
TEST_F(FramebufferTest, InvalidConfigurations) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memory_, nullptr);

    // Invalid extent (zero width)
    {
        Framebuffer::Config config{};
        config.extent = {0, testHeight_};
        config.createColorAttachment = true;

        auto fbResult = Framebuffer::create(context_.get(), memory_.get(), config);
        EXPECT_TRUE(fbResult.isFailure());
    }

    // Invalid extent (zero height)
    {
        Framebuffer::Config config{};
        config.extent = {testWidth_, 0};
        config.createColorAttachment = true;

        auto fbResult = Framebuffer::create(context_.get(), memory_.get(), config);
        EXPECT_TRUE(fbResult.isFailure());
    }

    // No attachments
    {
        Framebuffer::Config config{};
        config.extent = {testWidth_, testHeight_};
        config.createColorAttachment = false;
        config.createDepthAttachment = false;

        auto fbResult = Framebuffer::create(context_.get(), memory_.get(), config);
        EXPECT_TRUE(fbResult.isFailure());
    }

    // Null context
    {
        Framebuffer::Config config{};
        config.extent = {testWidth_, testHeight_};
        config.createColorAttachment = true;

        auto fbResult = Framebuffer::create(nullptr, memory_.get(), config);
        EXPECT_TRUE(fbResult.isFailure());
    }

    // Null memory manager
    {
        Framebuffer::Config config{};
        config.extent = {testWidth_, testHeight_};
        config.createColorAttachment = true;

        auto fbResult = Framebuffer::create(context_.get(), nullptr, config);
        EXPECT_TRUE(fbResult.isFailure());
    }
}

// Test various sizes
TEST_F(FramebufferTest, VariousSizes) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memory_, nullptr);

    std::vector<VkExtent2D> sizes = {
        {256, 256},    // Small square
        {512, 512},    // Medium square
        {1024, 1024},  // Large square
        {2048, 2048},  // Very large square (shadow maps)
        {1920, 1080},  // Full HD
        {2560, 1440},  // 2K
        {3840, 2160},  // 4K
    };

    for (const auto& size : sizes) {
        Framebuffer::Config config{};
        config.extent = size;
        config.createColorAttachment = true;
        config.createDepthAttachment = true;

        auto fbResult = Framebuffer::create(context_.get(), memory_.get(), config);
        ASSERT_TRUE(fbResult.isSuccess())
            << "Failed with size: " << size.width << "x" << size.height;

        auto& fb = fbResult.value();
        EXPECT_EQ(fb->getExtent().width, size.width);
        EXPECT_EQ(fb->getExtent().height, size.height);
    }
}

// Note: MSAA tests are commented out as they require additional setup
// and may not be supported on all devices. Uncomment if needed.
/*
TEST_F(FramebufferTest, MSAASupport) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memory_, nullptr);

    Framebuffer::Config config{};
    config.extent = {testWidth_, testHeight_};
    config.createColorAttachment = true;
    config.createDepthAttachment = true;
    config.samples = VK_SAMPLE_COUNT_4_BIT;  // 4x MSAA

    auto fbResult = Framebuffer::create(context_.get(), memory_.get(), config);
    ASSERT_TRUE(fbResult.isSuccess());

    auto& fb = fbResult.value();
    EXPECT_NE(fb->getColorView(), VK_NULL_HANDLE);
    EXPECT_NE(fb->getDepthView(), VK_NULL_HANDLE);
}
*/
