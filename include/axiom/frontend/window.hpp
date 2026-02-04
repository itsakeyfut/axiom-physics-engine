#pragma once

#include "axiom/core/result.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <vulkan/vulkan.h>

// Forward declare GLFW types to avoid including GLFW header in public API
struct GLFWwindow;

// Forward declarations from GPU module
namespace axiom::gpu {
class VkContext;
}

namespace axiom::frontend {

// Import VkContext from gpu namespace
using axiom::gpu::VkContext;

/// Configuration for window creation
/// This structure contains all parameters needed to create and configure
/// a GLFW window for Vulkan rendering.
struct WindowConfig {
    /// Window title displayed in the title bar
    const char* title = "Axiom Physics Engine";

    /// Initial window width in screen coordinates
    uint32_t width = 1920;

    /// Initial window height in screen coordinates
    uint32_t height = 1080;

    /// Start in fullscreen mode
    bool fullscreen = false;

    /// Allow window resizing
    bool resizable = true;

    /// Enable vertical synchronization
    bool vsync = true;

    /// Make window visible (set to false for headless/testing)
    /// When false, window is created but not displayed, useful for unit tests
    bool visible = true;
};

/// GLFW window wrapper with Vulkan surface integration
///
/// The Window class provides a high-level interface to GLFW window management
/// with integrated Vulkan surface creation. It handles window lifecycle,
/// event callbacks, and provides the VkSurfaceKHR required for swapchain creation.
///
/// Key features:
/// - GLFW window creation and management
/// - Vulkan surface (VkSurfaceKHR) creation and ownership
/// - Event handling (resize, keyboard, mouse)
/// - Fullscreen toggle support
/// - DPI-aware framebuffer sizing
///
/// Thread safety:
/// - Window creation and destruction must be called from the main thread
/// - Event polling (pollEvents) must be called from the main thread
/// - Callbacks are invoked from the main thread during pollEvents
/// - Getters are thread-safe after window creation
///
/// IMPORTANT: Initialization order
/// - GLFW must be initialized BEFORE creating VkContext if you plan to use windows
/// - Call Window::initializeGLFW() before VkContext::create() to ensure proper extension support
/// - This allows VkContext to query the required Vulkan instance extensions for window surfaces
///
/// Example usage:
/// @code
/// // Initialize GLFW first (required for surface extension support)
/// auto glfwResult = Window::initializeGLFW();
/// if (glfwResult.isFailure()) {
///     // Handle GLFW initialization failure
/// }
///
/// // Create Vulkan context (will include GLFW surface extensions)
/// auto contextResult = VkContext::create();
/// VkContext* context = contextResult.value().get();
///
/// // Create window
/// WindowConfig config{
///     .title = "My Physics Simulation",
///     .width = 1280,
///     .height = 720,
///     .vsync = true
/// };
///
/// auto windowResult = Window::create(context, config);
/// if (windowResult.isSuccess()) {
///     Window& window = *windowResult.value();
///
///     // Set resize callback for swapchain recreation
///     window.setResizeCallback([&](uint32_t width, uint32_t height) {
///         // Recreate swapchain with new dimensions
///     });
///
///     // Main loop
///     while (!window.shouldClose()) {
///         window.pollEvents();
///         // Render frame...
///     }
/// }
/// @endcode
class Window {
public:
    /// Initialize GLFW library (call this before creating VkContext)
    ///
    /// This function must be called before creating a VkContext if you plan
    /// to use windows. It allows VkContext to query the required Vulkan
    /// instance extensions for window surface support.
    ///
    /// This function is idempotent - calling it multiple times is safe.
    ///
    /// @return Result indicating success or failure
    ///
    /// Error conditions:
    /// - VulkanInitializationFailed: GLFW initialization failed
    static core::Result<void> initializeGLFW();

    /// Create a new window with Vulkan surface
    ///
    /// This function creates a window with the specified configuration and
    /// creates a Vulkan surface for the window. GLFW will be initialized
    /// if not already done (but it's recommended to call initializeGLFW()
    /// before creating VkContext for proper extension support).
    ///
    /// @param context The Vulkan context (must be created with surface extension support)
    /// @param config Configuration parameters for the window
    /// @return Result containing the window on success, or error code on failure
    ///
    /// Error conditions:
    /// - InvalidParameter: context is null or config has invalid dimensions
    /// - VulkanInitializationFailed: GLFW initialization or window creation failed
    /// - VulkanInitializationFailed: Surface creation failed (VkContext may lack required
    /// extensions)
    static core::Result<std::unique_ptr<Window>> create(VkContext* context,
                                                        const WindowConfig& config);

    /// Destructor - destroys the Vulkan surface and closes the window
    /// Note: GLFW termination is handled by the last Window instance
    ~Window();

    // Disable copy and move operations (GLFW window is not copyable)
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    /// Get the native GLFW window handle
    /// @return The GLFWwindow pointer (never null for a valid Window)
    GLFWwindow* getNativeHandle() const noexcept { return window_; }

    /// Get the Vulkan surface for this window
    /// @return The VkSurfaceKHR handle (never VK_NULL_HANDLE for a valid Window)
    VkSurfaceKHR getSurface() const noexcept { return surface_; }

    /// Get the current window width in screen coordinates
    /// @return Window width in screen coordinates (may differ from framebuffer size on high-DPI)
    uint32_t getWidth() const noexcept;

    /// Get the current window height in screen coordinates
    /// @return Window height in screen coordinates (may differ from framebuffer size on high-DPI)
    uint32_t getHeight() const noexcept;

    /// Get the framebuffer size in pixels
    ///
    /// On high-DPI displays (Retina, 4K), the framebuffer size may be larger
    /// than the window size in screen coordinates. Use this for rendering operations.
    ///
    /// @param width Output parameter for framebuffer width in pixels
    /// @param height Output parameter for framebuffer height in pixels
    void getFramebufferSize(uint32_t& width, uint32_t& height) const;

    /// Check if the window should close
    ///
    /// This returns true when the user has attempted to close the window
    /// (e.g., by clicking the close button or pressing Alt+F4).
    ///
    /// @return true if the window should close, false otherwise
    bool shouldClose() const;

    /// Poll for window events
    ///
    /// This processes all pending events and invokes registered callbacks.
    /// Must be called regularly from the main thread (typically once per frame).
    void pollEvents();

    /// Set the window title
    /// @param title New window title (string is copied, can be temporary)
    void setTitle(const char* title);

    /// Toggle between windowed and fullscreen mode
    ///
    /// In fullscreen mode, the window covers the entire primary monitor.
    /// The window dimensions are preserved when switching back to windowed mode.
    void toggleFullscreen();

    /// Check if the window is currently in fullscreen mode
    /// @return true if fullscreen, false if windowed
    bool isFullscreen() const noexcept;

    // Event callback types

    /// Callback invoked when the framebuffer is resized
    /// Parameters: width (pixels), height (pixels)
    using ResizeCallback = std::function<void(uint32_t, uint32_t)>;

    /// Callback invoked when a key is pressed, released, or repeated
    /// Parameters: key code, scancode, action (GLFW_PRESS/GLFW_RELEASE/GLFW_REPEAT), mods
    using KeyCallback = std::function<void(int, int, int, int)>;

    /// Callback invoked when a mouse button is pressed or released
    /// Parameters: button code, action (GLFW_PRESS/GLFW_RELEASE), mods
    using MouseButtonCallback = std::function<void(int, int, int)>;

    /// Callback invoked when the mouse cursor moves
    /// Parameters: x position, y position (in screen coordinates)
    using CursorPosCallback = std::function<void(double, double)>;

    /// Callback invoked when the mouse scroll wheel is used
    /// Parameters: x offset, y offset
    using ScrollCallback = std::function<void(double, double)>;

    /// Set the framebuffer resize callback
    ///
    /// The callback is invoked when the framebuffer size changes, typically
    /// when the user resizes the window or toggles fullscreen. This is the
    /// appropriate place to recreate the swapchain.
    ///
    /// @param callback Function to call on framebuffer resize (can be null to unregister)
    void setResizeCallback(ResizeCallback callback);

    /// Set the keyboard input callback
    /// @param callback Function to call on key events (can be null to unregister)
    void setKeyCallback(KeyCallback callback);

    /// Set the mouse button callback
    /// @param callback Function to call on mouse button events (can be null to unregister)
    void setMouseButtonCallback(MouseButtonCallback callback);

    /// Set the cursor position callback
    /// @param callback Function to call on cursor movement (can be null to unregister)
    void setCursorPosCallback(CursorPosCallback callback);

    /// Set the scroll callback
    /// @param callback Function to call on scroll events (can be null to unregister)
    void setScrollCallback(ScrollCallback callback);

private:
    /// Private constructor - use create() instead
    /// @param context The Vulkan context
    /// @param config Window configuration
    Window(VkContext* context, const WindowConfig& config);

    /// Create the GLFW window
    /// @return Result indicating success or failure
    core::Result<void> createWindow();

    /// Create the Vulkan surface for the window
    /// @return Result indicating success or failure
    core::Result<void> createSurface();

    /// Store window dimensions for fullscreen toggle
    void storeWindowedDimensions();

    /// GLFW callback for framebuffer resize events
    /// @param window The GLFW window that was resized
    /// @param width New framebuffer width in pixels
    /// @param height New framebuffer height in pixels
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    /// GLFW callback for key events
    /// @param window The GLFW window that received the event
    /// @param key The keyboard key code
    /// @param scancode The platform-specific scancode
    /// @param action GLFW_PRESS, GLFW_RELEASE, or GLFW_REPEAT
    /// @param mods Bit field describing modifier keys (Shift, Ctrl, Alt, etc.)
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    /// GLFW callback for mouse button events
    /// @param window The GLFW window that received the event
    /// @param button The mouse button code
    /// @param action GLFW_PRESS or GLFW_RELEASE
    /// @param mods Bit field describing modifier keys
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    /// GLFW callback for cursor position events
    /// @param window The GLFW window that received the event
    /// @param xpos Cursor x position in screen coordinates
    /// @param ypos Cursor y position in screen coordinates
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

    /// GLFW callback for scroll events
    /// @param window The GLFW window that received the event
    /// @param xoffset Horizontal scroll offset
    /// @param yoffset Vertical scroll offset
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    // Vulkan context (not owned)
    VkContext* context_ = nullptr;

    // Window configuration
    WindowConfig config_;

    // GLFW window handle
    GLFWwindow* window_ = nullptr;

    // Vulkan surface (owned, destroyed in destructor)
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;

    // Fullscreen state
    bool isFullscreen_ = false;

    // Windowed mode dimensions (for fullscreen toggle)
    int windowedX_ = 0;
    int windowedY_ = 0;
    int windowedWidth_ = 0;
    int windowedHeight_ = 0;

    // User callbacks
    ResizeCallback resizeCallback_;
    KeyCallback keyCallback_;
    MouseButtonCallback mouseButtonCallback_;
    CursorPosCallback cursorPosCallback_;
    ScrollCallback scrollCallback_;

    // Global GLFW initialization state
    static bool glfwInitialized_;
    static int windowCount_;
};

}  // namespace axiom::frontend
