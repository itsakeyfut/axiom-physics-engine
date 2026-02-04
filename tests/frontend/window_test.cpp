#include "axiom/frontend/window.hpp"
#include "axiom/gpu/vk_instance.hpp"

#include <gtest/gtest.h>

using namespace axiom::frontend;
using namespace axiom::gpu;
using namespace axiom::core;

// Test fixture for Window tests
class WindowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize GLFW first (required for surface extension support)
        auto glfwResult = Window::initializeGLFW();
        if (glfwResult.isFailure()) {
            GTEST_SKIP() << "GLFW initialization failed: " << glfwResult.errorMessage()
                         << " (this is expected in headless CI environments)";
        }

        // Create Vulkan context (will include GLFW surface extensions)
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

// Test WindowConfig default values
TEST(WindowConfigTest, DefaultConstruction) {
    WindowConfig config;

    EXPECT_STREQ(config.title, "Axiom Physics Engine");
    EXPECT_EQ(config.width, 1920u);
    EXPECT_EQ(config.height, 1080u);
    EXPECT_FALSE(config.fullscreen);
    EXPECT_TRUE(config.resizable);
    EXPECT_TRUE(config.vsync);
}

// Test WindowConfig custom values
TEST(WindowConfigTest, CustomConstruction) {
    WindowConfig config{.title = "Test Window",
                        .width = 800,
                        .height = 600,
                        .fullscreen = true,
                        .resizable = false,
                        .vsync = false};

    EXPECT_STREQ(config.title, "Test Window");
    EXPECT_EQ(config.width, 800u);
    EXPECT_EQ(config.height, 600u);
    EXPECT_TRUE(config.fullscreen);
    EXPECT_FALSE(config.resizable);
    EXPECT_FALSE(config.vsync);
}

// Test window creation fails with null context
TEST(WindowErrorTest, CreationFailsWithNullContext) {
    WindowConfig config{.width = 800, .height = 600, .visible = false};

    auto result = Window::create(nullptr, config);

    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test window creation fails with zero width
TEST_F(WindowTest, CreationFailsWithZeroWidth) {
    ASSERT_NE(context_, nullptr);

    WindowConfig config{.width = 0, .height = 600, .visible = false};

    auto result = Window::create(context_.get(), config);

    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test window creation fails with zero height
TEST_F(WindowTest, CreationFailsWithZeroHeight) {
    ASSERT_NE(context_, nullptr);

    WindowConfig config{.width = 800, .height = 0, .visible = false};

    auto result = Window::create(context_.get(), config);

    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test basic window creation
TEST_F(WindowTest, BasicCreation) {
    ASSERT_NE(context_, nullptr);

    WindowConfig config{.width = 800, .height = 600, .fullscreen = false, .visible = false};

    auto result = Window::create(context_.get(), config);

    ASSERT_TRUE(result.isSuccess());

    Window& window = *result.value();

    EXPECT_NE(window.getNativeHandle(), nullptr);
    EXPECT_NE(window.getSurface(), VK_NULL_HANDLE);
    EXPECT_FALSE(window.shouldClose());
    EXPECT_FALSE(window.isFullscreen());
}

// Test window dimensions
TEST_F(WindowTest, WindowDimensions) {
    ASSERT_NE(context_, nullptr);

    WindowConfig config{.width = 1280, .height = 720, .visible = false};

    auto result = Window::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    Window& window = *result.value();

    // Window size should match configuration
    EXPECT_EQ(window.getWidth(), 1280u);
    EXPECT_EQ(window.getHeight(), 720u);

    // Framebuffer size should be valid (may differ on high-DPI displays)
    uint32_t fbWidth = 0, fbHeight = 0;
    window.getFramebufferSize(fbWidth, fbHeight);
    EXPECT_GT(fbWidth, 0u);
    EXPECT_GT(fbHeight, 0u);
}

// Test window title
TEST_F(WindowTest, WindowTitle) {
    ASSERT_NE(context_, nullptr);

    WindowConfig config{.title = "Initial Title", .width = 800, .height = 600, .visible = false};

    auto result = Window::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    Window& window = *result.value();

    // Change title
    window.setTitle("New Title");

    // Note: GLFW doesn't provide a way to query the title, so we just verify it doesn't crash
}

// Test resize callback
TEST_F(WindowTest, ResizeCallback) {
    ASSERT_NE(context_, nullptr);

    WindowConfig config{.width = 800, .height = 600, .visible = false};

    auto result = Window::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    Window& window = *result.value();

    // Set resize callback
    bool callbackInvoked = false;
    uint32_t callbackWidth = 0;
    uint32_t callbackHeight = 0;

    window.setResizeCallback([&](uint32_t width, uint32_t height) {
        callbackInvoked = true;
        callbackWidth = width;
        callbackHeight = height;
    });

    // Note: We can't easily trigger a resize event in tests without user interaction
    // This test just verifies that setting the callback doesn't crash
    EXPECT_FALSE(callbackInvoked);  // No resize event yet
}

// Test key callback
TEST_F(WindowTest, KeyCallback) {
    ASSERT_NE(context_, nullptr);

    WindowConfig config{.width = 800, .height = 600, .visible = false};

    auto result = Window::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    Window& window = *result.value();

    // Set key callback
    bool callbackInvoked = false;

    window.setKeyCallback([&](int key, int scancode, int action, int mods) {
        (void)key;
        (void)scancode;
        (void)action;
        (void)mods;
        callbackInvoked = true;
    });

    // Note: We can't easily trigger key events in tests without user interaction
    EXPECT_FALSE(callbackInvoked);  // No key event yet
}

// Test mouse button callback
TEST_F(WindowTest, MouseButtonCallback) {
    ASSERT_NE(context_, nullptr);

    WindowConfig config{.width = 800, .height = 600, .visible = false};

    auto result = Window::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    Window& window = *result.value();

    // Set mouse button callback
    bool callbackInvoked = false;

    window.setMouseButtonCallback([&](int button, int action, int mods) {
        (void)button;
        (void)action;
        (void)mods;
        callbackInvoked = true;
    });

    EXPECT_FALSE(callbackInvoked);  // No mouse event yet
}

// Test cursor position callback
TEST_F(WindowTest, CursorPosCallback) {
    ASSERT_NE(context_, nullptr);

    WindowConfig config{.width = 800, .height = 600, .visible = false};

    auto result = Window::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    Window& window = *result.value();

    // Set cursor position callback
    bool callbackInvoked = false;

    window.setCursorPosCallback([&](double xpos, double ypos) {
        (void)xpos;
        (void)ypos;
        callbackInvoked = true;
    });

    EXPECT_FALSE(callbackInvoked);  // No cursor movement yet
}

// Test scroll callback
TEST_F(WindowTest, ScrollCallback) {
    ASSERT_NE(context_, nullptr);

    WindowConfig config{.width = 800, .height = 600, .visible = false};

    auto result = Window::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    Window& window = *result.value();

    // Set scroll callback
    bool callbackInvoked = false;

    window.setScrollCallback([&](double xoffset, double yoffset) {
        (void)xoffset;
        (void)yoffset;
        callbackInvoked = true;
    });

    EXPECT_FALSE(callbackInvoked);  // No scroll event yet
}

// Test poll events
TEST_F(WindowTest, PollEvents) {
    ASSERT_NE(context_, nullptr);

    WindowConfig config{.width = 800, .height = 600, .visible = false};

    auto result = Window::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    Window& window = *result.value();

    // Poll events should not crash
    window.pollEvents();
    window.pollEvents();
    window.pollEvents();
}

// Test multiple windows
TEST_F(WindowTest, MultipleWindows) {
    ASSERT_NE(context_, nullptr);

    WindowConfig config1{.title = "Window 1", .width = 800, .height = 600, .visible = false};
    WindowConfig config2{.title = "Window 2", .width = 640, .height = 480, .visible = false};

    auto result1 = Window::create(context_.get(), config1);
    ASSERT_TRUE(result1.isSuccess());

    auto result2 = Window::create(context_.get(), config2);
    ASSERT_TRUE(result2.isSuccess());

    Window& window1 = *result1.value();
    Window& window2 = *result2.value();

    // Both windows should be valid
    EXPECT_NE(window1.getNativeHandle(), nullptr);
    EXPECT_NE(window2.getNativeHandle(), nullptr);
    EXPECT_NE(window1.getSurface(), VK_NULL_HANDLE);
    EXPECT_NE(window2.getSurface(), VK_NULL_HANDLE);

    // Surfaces should be different
    EXPECT_NE(window1.getSurface(), window2.getSurface());
}

// Test window is not copyable
TEST(WindowTraitsTest, NotCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<Window>);
    EXPECT_FALSE(std::is_copy_assignable_v<Window>);
}

// Test window is not movable
TEST(WindowTraitsTest, NotMovable) {
    EXPECT_FALSE(std::is_move_constructible_v<Window>);
    EXPECT_FALSE(std::is_move_assignable_v<Window>);
}

// NOTE: Fullscreen tests have been removed because fullscreen mode changes
// affect the entire display even with invisible windows, disrupting the user's workspace.
// Fullscreen functionality can be tested manually if needed.

// Test window destruction
TEST_F(WindowTest, WindowDestruction) {
    ASSERT_NE(context_, nullptr);

    WindowConfig config{.width = 800, .height = 600, .visible = false};

    {
        auto result = Window::create(context_.get(), config);
        ASSERT_TRUE(result.isSuccess());

        Window& window = *result.value();
        EXPECT_NE(window.getNativeHandle(), nullptr);

        // Window should be destroyed when result goes out of scope
    }

    // No crash after destruction
}

// Test surface is valid for swapchain creation
TEST_F(WindowTest, SurfaceValidForSwapchain) {
    ASSERT_NE(context_, nullptr);

    WindowConfig config{.width = 800, .height = 600, .visible = false};

    auto result = Window::create(context_.get(), config);
    ASSERT_TRUE(result.isSuccess());

    Window& window = *result.value();

    VkSurfaceKHR surface = window.getSurface();
    ASSERT_NE(surface, VK_NULL_HANDLE);

    // Check if surface is supported by the physical device
    VkBool32 supported = VK_FALSE;
    uint32_t queueFamily = context_->getGraphicsQueueFamily();

    VkResult vkResult = vkGetPhysicalDeviceSurfaceSupportKHR(context_->getPhysicalDevice(),
                                                             queueFamily, surface, &supported);

    EXPECT_EQ(vkResult, VK_SUCCESS);
    EXPECT_TRUE(supported);
}
