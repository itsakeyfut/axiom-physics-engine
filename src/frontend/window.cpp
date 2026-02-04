#include "axiom/frontend/window.hpp"

#include "axiom/core/assert.hpp"
#include "axiom/core/logger.hpp"
#include "axiom/gpu/vk_instance.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace axiom::frontend {

// Initialize static members
bool Window::glfwInitialized_ = false;
int Window::windowCount_ = 0;

namespace {

// GLFW error callback
void glfwErrorCallback(int error, const char* description) {
    AXIOM_LOG_ERROR("GLFW", "Error %d: %s", error, description);
}

}  // namespace

Window::Window(VkContext* context, const WindowConfig& config)
    : context_(context), config_(config) {
    AXIOM_ASSERT(context != nullptr, "VkContext must not be null");
}

Window::~Window() {
    // Destroy Vulkan surface first (requires instance to be alive)
    if (surface_ != VK_NULL_HANDLE && context_ != nullptr) {
        vkDestroySurfaceKHR(context_->getInstance(), surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }

    // Destroy GLFW window
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
        windowCount_--;
    }

    // Terminate GLFW if this was the last window
    if (windowCount_ == 0 && glfwInitialized_) {
        glfwTerminate();
        glfwInitialized_ = false;
        AXIOM_LOG_INFO("Window", "GLFW terminated");
    }
}

core::Result<std::unique_ptr<Window>> Window::create(VkContext* context,
                                                     const WindowConfig& config) {
    // Validate parameters
    if (context == nullptr) {
        return core::Result<std::unique_ptr<Window>>::failure(core::ErrorCode::InvalidParameter,
                                                              "VkContext must not be null");
    }

    if (config.width == 0 || config.height == 0) {
        return core::Result<std::unique_ptr<Window>>::failure(core::ErrorCode::InvalidParameter,
                                                              "Window dimensions must be non-zero");
    }

    // Initialize GLFW if not already initialized
    auto initResult = initializeGLFW();
    if (initResult.isFailure()) {
        return core::Result<std::unique_ptr<Window>>::failure(initResult.errorCode(),
                                                              initResult.errorMessage());
    }

    // Create window object
    auto window = std::unique_ptr<Window>(new Window(context, config));

    // Create GLFW window
    auto createResult = window->createWindow();
    if (createResult.isFailure()) {
        return core::Result<std::unique_ptr<Window>>::failure(createResult.errorCode(),
                                                              createResult.errorMessage());
    }

    // Create Vulkan surface
    auto surfaceResult = window->createSurface();
    if (surfaceResult.isFailure()) {
        return core::Result<std::unique_ptr<Window>>::failure(surfaceResult.errorCode(),
                                                              surfaceResult.errorMessage());
    }

    AXIOM_LOG_INFO("Window", "Window created: %dx%d (%s)", config.width, config.height,
                   config.fullscreen ? "fullscreen" : "windowed");

    return core::Result<std::unique_ptr<Window>>::success(std::move(window));
}

core::Result<void> Window::initializeGLFW() {
    if (glfwInitialized_) {
        return core::Result<void>::success();
    }

    // Set error callback before initialization
    glfwSetErrorCallback(glfwErrorCallback);

    // Initialize GLFW
    if (glfwInit() != GLFW_TRUE) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to initialize GLFW");
    }

    glfwInitialized_ = true;
    AXIOM_LOG_INFO("Window", "GLFW initialized successfully");

    return core::Result<void>::success();
}

core::Result<void> Window::createWindow() {
    // Set window hints for Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // No OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, config_.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE,
                   config_.visible ? GLFW_TRUE : GLFW_FALSE);  // Visibility for testing

    // Create window
    GLFWmonitor* monitor = config_.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    window_ = glfwCreateWindow(static_cast<int>(config_.width), static_cast<int>(config_.height),
                               config_.title, monitor, nullptr);

    if (window_ == nullptr) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to create GLFW window");
    }

    windowCount_++;
    isFullscreen_ = config_.fullscreen;

    // Store windowed dimensions for fullscreen toggle
    if (!config_.fullscreen) {
        storeWindowedDimensions();
    }

    // Store Window pointer in GLFW user data for callbacks
    glfwSetWindowUserPointer(window_, this);

    // Register GLFW callbacks
    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
    glfwSetKeyCallback(window_, keyCallback);
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    glfwSetCursorPosCallback(window_, cursorPosCallback);
    glfwSetScrollCallback(window_, scrollCallback);

    return core::Result<void>::success();
}

core::Result<void> Window::createSurface() {
    VkResult result = glfwCreateWindowSurface(context_->getInstance(), window_, nullptr, &surface_);

    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to create Vulkan surface");
    }

    return core::Result<void>::success();
}

void Window::storeWindowedDimensions() {
    glfwGetWindowPos(window_, &windowedX_, &windowedY_);
    glfwGetWindowSize(window_, &windowedWidth_, &windowedHeight_);
}

uint32_t Window::getWidth() const noexcept {
    int width = 0;
    glfwGetWindowSize(window_, &width, nullptr);
    return static_cast<uint32_t>(width);
}

uint32_t Window::getHeight() const noexcept {
    int height = 0;
    glfwGetWindowSize(window_, nullptr, &height);
    return static_cast<uint32_t>(height);
}

void Window::getFramebufferSize(uint32_t& width, uint32_t& height) const {
    int w = 0, h = 0;
    glfwGetFramebufferSize(window_, &w, &h);
    width = static_cast<uint32_t>(w);
    height = static_cast<uint32_t>(h);
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(window_) != 0;
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::setTitle(const char* title) {
    glfwSetWindowTitle(window_, title);
}

void Window::toggleFullscreen() {
    if (isFullscreen_) {
        // Switch to windowed mode
        glfwSetWindowMonitor(window_, nullptr, windowedX_, windowedY_, windowedWidth_,
                             windowedHeight_, GLFW_DONT_CARE);
        isFullscreen_ = false;
        AXIOM_LOG_INFO("Window", "Switched to windowed mode: %dx%d", windowedWidth_,
                       windowedHeight_);
    } else {
        // Store current windowed dimensions
        storeWindowedDimensions();

        // Switch to fullscreen mode
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwSetWindowMonitor(window_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        isFullscreen_ = true;
        AXIOM_LOG_INFO("Window", "Switched to fullscreen mode: %dx%d@%dHz", mode->width,
                       mode->height, mode->refreshRate);
    }
}

bool Window::isFullscreen() const noexcept {
    return isFullscreen_;
}

void Window::setResizeCallback(ResizeCallback callback) {
    resizeCallback_ = std::move(callback);
}

void Window::setKeyCallback(KeyCallback callback) {
    keyCallback_ = std::move(callback);
}

void Window::setMouseButtonCallback(MouseButtonCallback callback) {
    mouseButtonCallback_ = std::move(callback);
}

void Window::setCursorPosCallback(CursorPosCallback callback) {
    cursorPosCallback_ = std::move(callback);
}

void Window::setScrollCallback(ScrollCallback callback) {
    scrollCallback_ = std::move(callback);
}

// Static GLFW callbacks

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self != nullptr && self->resizeCallback_) {
        self->resizeCallback_(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    }
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self != nullptr && self->keyCallback_) {
        self->keyCallback_(key, scancode, action, mods);
    }
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self != nullptr && self->mouseButtonCallback_) {
        self->mouseButtonCallback_(button, action, mods);
    }
}

void Window::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self != nullptr && self->cursorPosCallback_) {
        self->cursorPosCallback_(xpos, ypos);
    }
}

void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self != nullptr && self->scrollCallback_) {
        self->scrollCallback_(xoffset, yoffset);
    }
}

}  // namespace axiom::frontend
