#include "axiom/frontend/window.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_swapchain.hpp"
#include "axiom/gpu/vk_sync.hpp"

#include <gtest/gtest.h>

using namespace axiom::frontend;
using namespace axiom::gpu;
using namespace axiom::core;

// Test fixture for Swapchain tests
// These tests use real GLFW windows and Vulkan surfaces
class SwapchainTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize GLFW first (required for surface extension support)
        auto glfwResult = Window::initializeGLFW();
        if (glfwResult.isFailure()) {
            GTEST_SKIP() << "GLFW initialization failed: " << glfwResult.errorMessage()
                         << " (this is expected in headless CI environments)";
        }

        // Create context for swapchain tests (will include GLFW surface extensions)
        auto contextResult = VkContext::create();
        if (contextResult.isSuccess()) {
            context_ = std::move(contextResult.value());
        } else {
            GTEST_SKIP() << "Vulkan not available: " << contextResult.errorMessage()
                         << " (this is expected in CI environments without GPU)";
        }

        // Create window for surface
        WindowConfig windowConfig{.title = "Swapchain Test Window",
                                  .width = 800,
                                  .height = 600,
                                  .fullscreen = false,
                                  .visible = false};

        auto windowResult = Window::create(context_.get(), windowConfig);
        if (windowResult.isSuccess()) {
            window_ = std::move(windowResult.value());
        } else {
            GTEST_SKIP() << "Window creation failed: " << windowResult.errorMessage()
                         << " (this is expected in headless CI environments)";
        }
    }

    void TearDown() override {
        window_.reset();
        context_.reset();
    }

    std::unique_ptr<VkContext> context_;
    std::unique_ptr<Window> window_;
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

// Test swapchain creation with valid surface
TEST_F(SwapchainTest, CreationWithValidSurface) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(window_, nullptr);

    VkSurfaceKHR surface = window_->getSurface();

    SwapchainConfig config{.surface = surface, .width = 800, .height = 600, .vsync = true};

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

// Test image acquisition
TEST_F(SwapchainTest, AcquireNextImage) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(window_, nullptr);

    VkSurfaceKHR surface = window_->getSurface();
    SwapchainConfig config{.surface = surface, .width = 800, .height = 600, .vsync = true};

    auto swapchainResult = Swapchain::create(context_.get(), config);
    ASSERT_TRUE(swapchainResult.isSuccess());

    Swapchain& swapchain = swapchainResult.value();

    // Create a semaphore for image acquisition
    Semaphore semaphore(context_.get());

    auto acquireResult = swapchain.acquireNextImage(semaphore.get());

    EXPECT_LT(acquireResult.imageIndex, swapchain.getImageCount());
    EXPECT_FALSE(acquireResult.needsResize);
}

// Test presentation
TEST_F(SwapchainTest, Present) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(window_, nullptr);

    VkSurfaceKHR surface = window_->getSurface();
    SwapchainConfig config{.surface = surface, .width = 800, .height = 600, .vsync = true};

    auto swapchainResult = Swapchain::create(context_.get(), config);
    ASSERT_TRUE(swapchainResult.isSuccess());

    Swapchain& swapchain = swapchainResult.value();

    // Create semaphores for synchronization
    Semaphore acquireSemaphore(context_.get());
    Semaphore renderSemaphore(context_.get());

    // Acquire image
    auto acquireResult = swapchain.acquireNextImage(acquireSemaphore.get());
    ASSERT_FALSE(acquireResult.needsResize);

    // Present image
    PresentInfo presentInfo{.imageIndex = acquireResult.imageIndex,
                            .waitSemaphores = {renderSemaphore.get()}};

    bool presentSuccess = swapchain.present(context_->getGraphicsQueue(), presentInfo);

    EXPECT_TRUE(presentSuccess);
}

// Test swapchain resize
TEST_F(SwapchainTest, Resize) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(window_, nullptr);

    VkSurfaceKHR surface = window_->getSurface();
    SwapchainConfig config{.surface = surface, .width = 800, .height = 600, .vsync = true};

    auto swapchainResult = Swapchain::create(context_.get(), config);
    ASSERT_TRUE(swapchainResult.isSuccess());

    Swapchain& swapchain = swapchainResult.value();

    // Resize to new dimensions
    auto resizeResult = swapchain.resize(1024, 768);

    EXPECT_TRUE(resizeResult.isSuccess());
    EXPECT_EQ(swapchain.getExtent().width, 1024u);
    EXPECT_EQ(swapchain.getExtent().height, 768u);
}

// Test resize with zero dimensions fails
TEST_F(SwapchainTest, ResizeFailsWithZeroDimensions) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(window_, nullptr);

    VkSurfaceKHR surface = window_->getSurface();
    SwapchainConfig config{.surface = surface, .width = 800, .height = 600, .vsync = true};

    auto swapchainResult = Swapchain::create(context_.get(), config);
    ASSERT_TRUE(swapchainResult.isSuccess());

    Swapchain& swapchain = swapchainResult.value();

    // Try to resize with zero width
    auto resizeResult = swapchain.resize(0, 768);

    EXPECT_FALSE(resizeResult.isSuccess());
    EXPECT_EQ(resizeResult.errorCode(), ErrorCode::InvalidParameter);
}

// Test VSync enforcement
TEST_F(SwapchainTest, VSyncEnforcement) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(window_, nullptr);

    VkSurfaceKHR surface = window_->getSurface();

    // Create swapchain with vsync enabled
    SwapchainConfig configVSync{.surface = surface, .width = 800, .height = 600, .vsync = true};

    auto resultVSync = Swapchain::create(context_.get(), configVSync);
    ASSERT_TRUE(resultVSync.isSuccess());

    // Swapchain should be created successfully with vsync
    Swapchain& swapchain = resultVSync.value();
    EXPECT_NE(swapchain.get(), VK_NULL_HANDLE);

    // Note: Present mode is not exposed in the API, but we can verify the swapchain was created
    // The actual FIFO present mode enforcement happens internally
}

// Test out-of-date handling
// Note: This test cannot reliably trigger an out-of-date condition programmatically
// In real usage, out-of-date occurs when the window is resized externally
TEST_F(SwapchainTest, OutOfDateHandling) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(window_, nullptr);

    VkSurfaceKHR surface = window_->getSurface();
    SwapchainConfig config{.surface = surface, .width = 800, .height = 600, .vsync = true};

    auto swapchainResult = Swapchain::create(context_.get(), config);
    ASSERT_TRUE(swapchainResult.isSuccess());

    Swapchain& swapchain = swapchainResult.value();

    // Create semaphore for acquisition
    Semaphore semaphore(context_.get());

    // Acquire image - should succeed normally without resize needed
    auto acquireResult = swapchain.acquireNextImage(semaphore.get());
    EXPECT_FALSE(acquireResult.needsResize);

    // Note: To trigger out-of-date, we would need external window resize events
    // which are difficult to simulate in unit tests
}

// Test multiple swapchains
TEST_F(SwapchainTest, MultipleSwapchains) {
    ASSERT_NE(context_, nullptr);

    // Create two windows
    WindowConfig config1{.title = "Window 1", .width = 640, .height = 480, .visible = false};
    WindowConfig config2{.title = "Window 2", .width = 800, .height = 600, .visible = false};

    auto window1Result = Window::create(context_.get(), config1);
    ASSERT_TRUE(window1Result.isSuccess());
    Window& window1 = *window1Result.value();

    auto window2Result = Window::create(context_.get(), config2);
    ASSERT_TRUE(window2Result.isSuccess());
    Window& window2 = *window2Result.value();

    // Create swapchains for both windows
    SwapchainConfig swapchainConfig1{
        .surface = window1.getSurface(), .width = 640, .height = 480, .vsync = true};
    SwapchainConfig swapchainConfig2{
        .surface = window2.getSurface(), .width = 800, .height = 600, .vsync = true};

    auto swapchain1Result = Swapchain::create(context_.get(), swapchainConfig1);
    ASSERT_TRUE(swapchain1Result.isSuccess());

    auto swapchain2Result = Swapchain::create(context_.get(), swapchainConfig2);
    ASSERT_TRUE(swapchain2Result.isSuccess());

    // Verify both swapchains are valid and distinct
    Swapchain& swapchain1 = swapchain1Result.value();
    Swapchain& swapchain2 = swapchain2Result.value();

    EXPECT_NE(swapchain1.get(), VK_NULL_HANDLE);
    EXPECT_NE(swapchain2.get(), VK_NULL_HANDLE);
    EXPECT_NE(swapchain1.get(), swapchain2.get());
}

// Test swapchain image access
TEST_F(SwapchainTest, ImageAccess) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(window_, nullptr);

    VkSurfaceKHR surface = window_->getSurface();
    SwapchainConfig config{.surface = surface, .width = 800, .height = 600, .vsync = true};

    auto swapchainResult = Swapchain::create(context_.get(), config);
    ASSERT_TRUE(swapchainResult.isSuccess());

    Swapchain& swapchain = swapchainResult.value();

    // Verify all images and image views are valid
    uint32_t imageCount = swapchain.getImageCount();
    EXPECT_GT(imageCount, 0u);

    for (uint32_t i = 0; i < imageCount; ++i) {
        EXPECT_NE(swapchain.getImage(i), VK_NULL_HANDLE);
        EXPECT_NE(swapchain.getImageView(i), VK_NULL_HANDLE);
    }
}
