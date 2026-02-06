// Physics Panel Example
// Demonstrates the physics debug panel with ImGui integration
//
// This example creates a window with ImGui and shows how to use the
// PhysicsDebugPanel to display and control physics simulation parameters.

#include "axiom/core/logger.hpp"
#include "axiom/debug/physics_debug_draw.hpp"
#include "axiom/frontend/window.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gui/imgui_impl.hpp"
#include "axiom/gui/physics_panel.hpp"

#include <chrono>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <memory>
#include <thread>

using namespace axiom;

// Simple mock physics world for demonstration
struct MockPhysicsWorld {
    gui::PhysicsWorldStats stats;
    gui::PhysicsWorldConfig config;
    debug::PhysicsDebugFlags debugFlags = debug::PhysicsDebugFlags::Shapes |
                                          debug::PhysicsDebugFlags::Contacts;

    // Simulate some physics activity
    void update(float dt) {
        // Simulate increasing body counts over time
        static float time = 0.0f;
        time += dt;

        stats.totalBodies = 50 + static_cast<uint32_t>(std::sin(time * 0.5f) * 20.0f);
        stats.activeBodies = static_cast<uint32_t>(stats.totalBodies * 0.8f);
        stats.sleepingBodies = stats.totalBodies - stats.activeBodies;
        stats.staticBodies = 10;
        stats.dynamicBodies = stats.totalBodies - stats.staticBodies;
        stats.kinematicBodies = 0;

        // Simulate varying contact counts
        stats.contactPointCount = 100 + static_cast<uint32_t>(std::sin(time * 2.0f) * 50.0f);
        stats.constraintCount = 25;
        stats.islandCount = 5 + static_cast<uint32_t>(std::sin(time) * 2.0f);

        // Simulate performance metrics (in milliseconds)
        float variation = 1.0f + std::sin(time * 3.0f) * 0.3f;
        stats.broadphaseTime = 2.0f * variation;
        stats.narrowphaseTime = 5.0f * variation;
        stats.solverTime = 8.0f * variation;
        stats.integrationTime = 1.5f * variation;
        stats.totalStepTime = stats.broadphaseTime + stats.narrowphaseTime + stats.solverTime +
                              stats.integrationTime;
    }

    void applyConfig(const gui::PhysicsWorldConfig& newConfig) {
        config = newConfig;
        AXIOM_LOG_INFO("PhysicsPanel", "Configuration updated: gravity=(%.2f, %.2f, %.2f)",
                       config.gravity.x, config.gravity.y, config.gravity.z);
    }

    void applyDebugFlags(debug::PhysicsDebugFlags newFlags) {
        debugFlags = newFlags;
        AXIOM_LOG_DEBUG("PhysicsPanel", "Debug flags updated: 0x%08X",
                        static_cast<uint32_t>(debugFlags));
    }
};

int main() {
    // Initialize logging
    AXIOM_LOG_INFO("PhysicsPanel", "Starting physics panel example");

    // Initialize GLFW
    auto glfwResult = frontend::Window::initializeGLFW();
    if (glfwResult.isFailure()) {
        AXIOM_LOG_ERROR("PhysicsPanel", "Failed to initialize GLFW: %s", glfwResult.errorMessage());
        return 1;
    }

    // Create Vulkan context
    auto contextResult = gpu::VkContext::create();
    if (contextResult.isFailure()) {
        AXIOM_LOG_ERROR("PhysicsPanel", "Failed to create Vulkan context: %s",
                        contextResult.errorMessage());
        return 1;
    }
    auto context = std::move(contextResult.value());

    // Create window
    frontend::WindowConfig windowConfig{};
    windowConfig.title = "Physics Panel Example";
    windowConfig.width = 1280;
    windowConfig.height = 720;
    windowConfig.visible = true;

    auto windowResult = frontend::Window::create(context.get(), windowConfig);
    if (windowResult.isFailure()) {
        AXIOM_LOG_ERROR("PhysicsPanel", "Failed to create window: %s", windowResult.errorMessage());
        return 1;
    }
    auto window = std::move(windowResult.value());

    // Create ImGui renderer
    auto imguiResult = gui::ImGuiRenderer::create(context.get(), window.get());
    if (imguiResult.isFailure()) {
        AXIOM_LOG_ERROR("PhysicsPanel", "Failed to create ImGui renderer: %s",
                        imguiResult.errorMessage());
        return 1;
    }
    auto imgui = std::move(imguiResult.value());

    // Create physics debug panel
    gui::PhysicsDebugPanel panel;

    // Create mock physics world
    MockPhysicsWorld world;

    // Main loop
    AXIOM_LOG_INFO("PhysicsPanel", "Entering main loop");
    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    while (!window->shouldClose()) {
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        // Poll window events
        window->pollEvents();

        // Update mock physics world
        world.update(deltaTime);

        // Start ImGui frame
        imgui->newFrame();

        // Render the physics debug panel
        if (panel.render(world.stats, world.config, world.debugFlags)) {
            // Configuration was modified by user, apply it to the world
            world.applyConfig(world.config);
            world.applyDebugFlags(world.debugFlags);
        }

        // Render additional UI for demonstration
        ImGui::Begin("Example Info");
        ImGui::Text("Physics Panel Example");
        ImGui::Separator();
        ImGui::Text("This example demonstrates the PhysicsDebugPanel");
        ImGui::Text("with a mock physics simulation.");
        ImGui::Spacing();
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Frame Time: %.2f ms", deltaTime * 1000.0f);
        ImGui::Spacing();
        ImGui::Text("Instructions:");
        ImGui::BulletText("Adjust simulation settings in the panel");
        ImGui::BulletText("Toggle visualization options");
        ImGui::BulletText("Observe performance metrics");
        ImGui::Spacing();
        if (ImGui::Button("Reset Panel Position")) {
            ImGui::SetWindowPos("Physics Debug Panel", ImVec2(50, 50));
        }
        ImGui::End();

        // Show ImGui demo window for reference
        static bool showDemoWindow = false;
        ImGui::Begin("Example Info");
        ImGui::Checkbox("Show ImGui Demo Window", &showDemoWindow);
        ImGui::End();

        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        // Finalize ImGui rendering
        ImGui::Render();

        // Clear the screen (simple background color)
        // Note: In a real application, you would have a proper render pass here
        // For this example, we just demonstrate the ImGui integration

        // Present (normally you'd render to a command buffer and present)
        // For this example, we just sleep to limit frame rate
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
    }

    AXIOM_LOG_INFO("PhysicsPanel", "Exiting physics panel example");
    return 0;
}
