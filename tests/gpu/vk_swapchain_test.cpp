#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_swapchain.hpp"

#include <gtest/gtest.h>

using namespace axiom::gpu;
using namespace axiom::core;

// Test fixture for Swapchain tests
// Note: These tests require a valid VkSurfaceKHR which requires a window system.
// Since the project doesn't have windowing infrastructure yet, most tests are
// disabled and serve as documentation of the API contract.
class SwapchainTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create context for swapchain tests
        auto result = VkContext::create();
        if (result.isSuccess()) {
            context_ = std::move(result.value());
        } else {
            GTEST_SKIP() << "Vulkan not available: " << result.errorMessage()
                         << " (this is expected in CI environments without GPU)";
        }
    }

    void TearDown() override { context_.reset(); }

    std::unique_ptr<VkContext> context_;
};

// Test swapchain configuration structure
TEST(SwapchainConfigTest, DefaultConstruction) {
    SwapchainConfig config;

    EXPECT_EQ(config.surface, VK_NULL_HANDLE);
    EXPECT_EQ(config.width, 0u);
    EXPECT_EQ(config.height, 0u);
    EXPECT_EQ(config.preferredPresentMode, VK_PRESENT_MODE_MAILBOX_KHR);
    EXPECT_TRUE(config.vsync);
}

// Test swapchain configuration with custom values
TEST(SwapchainConfigTest, CustomConstruction) {
    SwapchainConfig config{.surface = reinterpret_cast<VkSurfaceKHR>(0x1234),  // Mock value
                           .width = 1920,
                           .height = 1080,
                           .preferredPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR,
                           .vsync = false};

    EXPECT_EQ(config.surface, reinterpret_cast<VkSurfaceKHR>(0x1234));
    EXPECT_EQ(config.width, 1920u);
    EXPECT_EQ(config.height, 1080u);
    EXPECT_EQ(config.preferredPresentMode, VK_PRESENT_MODE_IMMEDIATE_KHR);
    EXPECT_FALSE(config.vsync);
}

// Test AcquireResult structure
TEST(SwapchainStructTest, AcquireResultDefault) {
    AcquireResult result;

    EXPECT_EQ(result.imageIndex, 0u);
    EXPECT_FALSE(result.needsResize);
}

// Test PresentInfo structure
TEST(SwapchainStructTest, PresentInfoDefault) {
    PresentInfo info;

    EXPECT_EQ(info.imageIndex, 0u);
    EXPECT_TRUE(info.waitSemaphores.empty());
}

// Test that swapchain creation fails without valid context
TEST_F(SwapchainTest, CreationFailsWithNullContext) {
    SwapchainConfig config{.surface = reinterpret_cast<VkSurfaceKHR>(0x1234),  // Mock value
                           .width = 1920,
                           .height = 1080,
                           .vsync = true};

    auto result = Swapchain::create(nullptr, config);

    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test that swapchain creation fails without valid surface
TEST_F(SwapchainTest, CreationFailsWithNullSurface) {
    ASSERT_NE(context_, nullptr);

    SwapchainConfig config{.surface = VK_NULL_HANDLE,  // Invalid surface
                           .width = 1920,
                           .height = 1080,
                           .vsync = true};

    auto result = Swapchain::create(context_.get(), config);

    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test that swapchain creation fails with invalid dimensions
TEST_F(SwapchainTest, CreationFailsWithZeroWidth) {
    ASSERT_NE(context_, nullptr);

    SwapchainConfig config{.surface = reinterpret_cast<VkSurfaceKHR>(0x1234),  // Mock value
                           .width = 0,                                         // Invalid
                           .height = 1080,
                           .vsync = true};

    auto result = Swapchain::create(context_.get(), config);

    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test that swapchain creation fails with invalid dimensions
TEST_F(SwapchainTest, CreationFailsWithZeroHeight) {
    ASSERT_NE(context_, nullptr);

    SwapchainConfig config{.surface = reinterpret_cast<VkSurfaceKHR>(0x1234),  // Mock value
                           .width = 1920,
                           .height = 0,  // Invalid
                           .vsync = true};

    auto result = Swapchain::create(context_.get(), config);

    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test that swapchain is move-constructible
TEST(SwapchainMoveTest, MoveConstructible) {
    EXPECT_TRUE(std::is_move_constructible_v<Swapchain>);
}

// Test that swapchain is move-assignable
TEST(SwapchainMoveTest, MoveAssignable) {
    EXPECT_TRUE(std::is_move_assignable_v<Swapchain>);
}

// Test that swapchain is not copy-constructible
TEST(SwapchainMoveTest, NotCopyConstructible) {
    EXPECT_FALSE(std::is_copy_constructible_v<Swapchain>);
}

// Test that swapchain is not copy-assignable
TEST(SwapchainMoveTest, NotCopyAssignable) {
    EXPECT_FALSE(std::is_copy_assignable_v<Swapchain>);
}

// The following tests are disabled because they require a real window surface
// They serve as documentation of expected behavior and can be enabled when
// windowing infrastructure is added to the test suite.

// DISABLED: Test swapchain creation with valid surface
TEST_F(SwapchainTest, DISABLED_CreationWithValidSurface) {
    ASSERT_NE(context_, nullptr);

    // TODO: Create a real VkSurfaceKHR using GLFW or similar
    // For now, this test is disabled as it requires windowing infrastructure
    VkSurfaceKHR mockSurface = VK_NULL_HANDLE;  // Would be real surface

    SwapchainConfig config{.surface = mockSurface, .width = 1920, .height = 1080, .vsync = true};

    auto result = Swapchain::create(context_.get(), config);

    EXPECT_TRUE(result.isSuccess());
    if (result.isSuccess()) {
        Swapchain& swapchain = result.value();

        EXPECT_NE(swapchain.get(), VK_NULL_HANDLE);
        EXPECT_NE(swapchain.getFormat(), VK_FORMAT_UNDEFINED);
        EXPECT_GT(swapchain.getExtent().width, 0u);
        EXPECT_GT(swapchain.getExtent().height, 0u);
        EXPECT_GT(swapchain.getImageCount(), 0u);
    }
}

// DISABLED: Test image acquisition
TEST_F(SwapchainTest, DISABLED_AcquireNextImage) {
    ASSERT_NE(context_, nullptr);

    // TODO: Create real surface and swapchain
    VkSurfaceKHR mockSurface = VK_NULL_HANDLE;
    SwapchainConfig config{.surface = mockSurface, .width = 1920, .height = 1080, .vsync = true};

    auto result = Swapchain::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    Swapchain& swapchain = result.value();

    // TODO: Create real semaphore
    VkSemaphore mockSemaphore = VK_NULL_HANDLE;

    auto acquireResult = swapchain.acquireNextImage(mockSemaphore);

    EXPECT_LT(acquireResult.imageIndex, swapchain.getImageCount());
}

// DISABLED: Test presentation
TEST_F(SwapchainTest, DISABLED_Present) {
    ASSERT_NE(context_, nullptr);

    // TODO: Create real surface and swapchain
    VkSurfaceKHR mockSurface = VK_NULL_HANDLE;
    SwapchainConfig config{.surface = mockSurface, .width = 1920, .height = 1080, .vsync = true};

    auto result = Swapchain::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    Swapchain& swapchain = result.value();

    PresentInfo presentInfo{.imageIndex = 0, .waitSemaphores = {}};

    bool presentSuccess = swapchain.present(context_->getGraphicsQueue(), presentInfo);

    EXPECT_TRUE(presentSuccess);
}

// DISABLED: Test swapchain resize
TEST_F(SwapchainTest, DISABLED_Resize) {
    ASSERT_NE(context_, nullptr);

    // TODO: Create real surface and swapchain
    VkSurfaceKHR mockSurface = VK_NULL_HANDLE;
    SwapchainConfig config{.surface = mockSurface, .width = 1920, .height = 1080, .vsync = true};

    auto result = Swapchain::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    Swapchain& swapchain = result.value();

    // Resize to new dimensions
    auto resizeResult = swapchain.resize(2560, 1440);

    EXPECT_TRUE(resizeResult.isSuccess());
    EXPECT_EQ(swapchain.getExtent().width, 2560u);
    EXPECT_EQ(swapchain.getExtent().height, 1440u);
}

// DISABLED: Test resize with zero dimensions fails
TEST_F(SwapchainTest, DISABLED_ResizeFailsWithZeroDimensions) {
    ASSERT_NE(context_, nullptr);

    // TODO: Create real surface and swapchain
    VkSurfaceKHR mockSurface = VK_NULL_HANDLE;
    SwapchainConfig config{.surface = mockSurface, .width = 1920, .height = 1080, .vsync = true};

    auto result = Swapchain::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    Swapchain& swapchain = result.value();

    // Try to resize with zero width
    auto resizeResult = swapchain.resize(0, 1440);

    EXPECT_FALSE(resizeResult.isSuccess());
    EXPECT_EQ(resizeResult.errorCode(), ErrorCode::InvalidParameter);
}

// DISABLED: Test VSync enforcement
TEST_F(SwapchainTest, DISABLED_VSyncEnforcement) {
    ASSERT_NE(context_, nullptr);

    // TODO: Create real surface
    VkSurfaceKHR mockSurface = VK_NULL_HANDLE;

    // Create swapchain with vsync enabled
    SwapchainConfig configVSync{
        .surface = mockSurface, .width = 1920, .height = 1080, .vsync = true};

    auto resultVSync = Swapchain::create(context_.get(), configVSync);
    ASSERT_TRUE(resultVSync.isSuccess());

    // Present mode should be FIFO when vsync is enabled
    // (This would require exposing present mode in the API for testing)
}

// DISABLED: Test out-of-date handling
TEST_F(SwapchainTest, DISABLED_OutOfDateHandling) {
    ASSERT_NE(context_, nullptr);

    // TODO: Create real surface and swapchain
    VkSurfaceKHR mockSurface = VK_NULL_HANDLE;
    SwapchainConfig config{.surface = mockSurface, .width = 1920, .height = 1080, .vsync = true};

    auto result = Swapchain::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    // Swapchain& swapchain = result.value(); // Unused until real surface testing is available

    // TODO: Trigger out-of-date condition (e.g., by resizing window)
    // and verify that acquireNextImage returns needsResize = true
    // Will use 'swapchain' once real surface is available
}

// DISABLED: Test multiple swapchains
TEST_F(SwapchainTest, DISABLED_MultipleSwapchains) {
    ASSERT_NE(context_, nullptr);

    // TODO: Create multiple real surfaces and swapchains
    // Verify that multiple swapchains can coexist
}

// DISABLED: Test swapchain image access
TEST_F(SwapchainTest, DISABLED_ImageAccess) {
    ASSERT_NE(context_, nullptr);

    // TODO: Create real surface and swapchain
    VkSurfaceKHR mockSurface = VK_NULL_HANDLE;
    SwapchainConfig config{.surface = mockSurface, .width = 1920, .height = 1080, .vsync = true};

    auto result = Swapchain::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    Swapchain& swapchain = result.value();

    // Verify all images and image views are valid
    for (uint32_t i = 0; i < swapchain.getImageCount(); ++i) {
        EXPECT_NE(swapchain.getImage(i), VK_NULL_HANDLE);
        EXPECT_NE(swapchain.getImageView(i), VK_NULL_HANDLE);
    }
}
