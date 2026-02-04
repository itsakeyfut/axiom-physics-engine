// Axiom Physics Engine - Main Application
// Simple GLFW window demonstration

#include "axiom/core/logger.hpp"
#include "axiom/frontend/window.hpp"
#include "axiom/gpu/vk_instance.hpp"

#include <memory>

int main() {
    AXIOM_LOG_INFO("App", "Axiom Physics Engine - Starting");

    // Initialize GLFW first (required for Vulkan surface extensions)
    auto glfwResult = axiom::frontend::Window::initializeGLFW();
    if (glfwResult.isFailure()) {
        AXIOM_LOG_ERROR("App", "Failed to initialize GLFW: %s", glfwResult.errorMessage());
        return 1;
    }

    AXIOM_LOG_INFO("App", "GLFW initialized successfully");

    // Create Vulkan context (will include GLFW surface extensions)
    auto contextResult = axiom::gpu::VkContext::create();
    if (contextResult.isFailure()) {
        AXIOM_LOG_ERROR("App", "Failed to create Vulkan context: %s", contextResult.errorMessage());
        return 1;
    }

    std::unique_ptr<axiom::gpu::VkContext> context = std::move(contextResult.value());
    AXIOM_LOG_INFO("App", "Vulkan context created successfully");

    // Create window
    axiom::frontend::WindowConfig windowConfig{
        .title = "Axiom Physics Engine", .width = 1280, .height = 720, .vsync = true};

    auto windowResult = axiom::frontend::Window::create(context.get(), windowConfig);
    if (windowResult.isFailure()) {
        AXIOM_LOG_ERROR("App", "Failed to create window: %s", windowResult.errorMessage());
        return 1;
    }

    std::unique_ptr<axiom::frontend::Window> window = std::move(windowResult.value());
    AXIOM_LOG_INFO("App", "Window created successfully");

    // Setup resize callback
    window->setResizeCallback([](uint32_t width, uint32_t height) {
        AXIOM_LOG_INFO("App", "Window resized to %ux%u", width, height);
    });

    // Setup key callback for ESC to close
    window->setKeyCallback([](int key, int /*scancode*/, int action, int /*mods*/) {
        if (key == 256 && action == 1) {  // ESC key pressed (GLFW_KEY_ESCAPE = 256, GLFW_PRESS =
                                          // 1)
            AXIOM_LOG_INFO("App", "ESC pressed - closing window");
            // Note: Window closing is handled by the window's native close button
            // A future enhancement could add Window::setShouldClose() method
        }
    });

    AXIOM_LOG_INFO("App", "Entering main loop (press ESC or close window to exit)");

    // Main loop
    while (!window->shouldClose()) {
        window->pollEvents();

        // TODO: Render frame here
        // For now, just a simple loop that polls events
    }

    AXIOM_LOG_INFO("App", "Main loop exited - shutting down");

    // Cleanup (automatic via RAII)
    window.reset();
    context.reset();

    AXIOM_LOG_INFO("App", "Axiom Physics Engine - Shutdown complete");

    return 0;
}
