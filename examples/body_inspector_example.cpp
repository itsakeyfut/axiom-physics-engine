// Body Inspector Example
// Demonstrates the body inspector panel with ImGui integration
//
// This example creates a window with ImGui and shows how to use the
// BodyInspector to view and edit individual rigid body properties.

#include "axiom/core/logger.hpp"
#include "axiom/frontend/window.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gui/body_inspector.hpp"
#include "axiom/gui/imgui_impl.hpp"
#include "axiom/math/utils.hpp"

#include <chrono>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <memory>
#include <thread>
#include <vector>

using namespace axiom;

// Mock rigid body for demonstration
struct MockRigidBody {
    gui::RigidBodyData data;

    MockRigidBody(uint32_t id, const char* name, gui::BodyType type) {
        data.id = id;
        data.name = name;
        data.type = type;

        // Set some initial values based on type
        if (type == gui::BodyType::Static) {
            data.position = math::Vec3(0.0f, 0.0f, 0.0f);
            data.mass = 0.0f;  // Infinite mass
        } else if (type == gui::BodyType::Dynamic) {
            data.position = math::Vec3(0.0f, 10.0f, 0.0f);
            data.linearVelocity = math::Vec3(0.0f, 0.0f, 0.0f);
            data.mass = 10.0f;
        } else {
            data.position = math::Vec3(5.0f, 0.0f, 0.0f);
            data.linearVelocity = math::Vec3(1.0f, 0.0f, 0.0f);
            data.mass = 0.0f;  // Infinite mass
        }
    }

    // Simulate physics update
    void update(float dt) {
        if (data.type == gui::BodyType::Dynamic) {
            // Simple gravity integration
            math::Vec3 gravity(0.0f, -9.81f, 0.0f);
            data.linearVelocity = data.linearVelocity + gravity * dt;

            // Apply damping
            data.linearVelocity = data.linearVelocity * (1.0f - data.linearDamping);
            data.angularVelocity = data.angularVelocity * (1.0f - data.angularDamping);

            // Integrate position
            data.position = data.position + data.linearVelocity * dt;

            // Simple ground collision
            if (data.position.y < 0.5f) {
                data.position.y = 0.5f;
                data.linearVelocity.y = -data.linearVelocity.y * data.material.restitution;

                // Check if should sleep
                float speed = std::sqrt(data.linearVelocity.x * data.linearVelocity.x +
                                        data.linearVelocity.y * data.linearVelocity.y +
                                        data.linearVelocity.z * data.linearVelocity.z);

                if (speed < data.sleep.linearThreshold && data.sleep.allowSleep) {
                    data.sleep.sleepTime += dt;
                    if (data.sleep.sleepTime > 0.5f && !data.sleep.isSleeping) {
                        data.sleep.isSleeping = true;
                        AXIOM_LOG_INFO("BodyInspector", "Body %u (%s) went to sleep", data.id,
                                       data.name.c_str());
                    }
                } else {
                    data.sleep.sleepTime = 0.0f;
                    if (data.sleep.isSleeping) {
                        data.sleep.isSleeping = false;
                        AXIOM_LOG_INFO("BodyInspector", "Body %u (%s) woke up", data.id,
                                       data.name.c_str());
                    }
                }
            }
        }
    }

    void applyData(const gui::RigidBodyData& newData) {
        data = newData;
        AXIOM_LOG_INFO("BodyInspector", "Body %u (%s) data updated from inspector", data.id,
                       data.name.c_str());
    }
};

int main() {
    // Initialize logging
    AXIOM_LOG_INFO("BodyInspector", "Starting body inspector example");

    // Initialize GLFW
    auto glfwResult = frontend::Window::initializeGLFW();
    if (glfwResult.isFailure()) {
        AXIOM_LOG_ERROR("BodyInspector", "Failed to initialize GLFW: %s",
                        glfwResult.errorMessage());
        return 1;
    }

    // Create Vulkan context
    auto contextResult = gpu::VkContext::create();
    if (contextResult.isFailure()) {
        AXIOM_LOG_ERROR("BodyInspector", "Failed to create Vulkan context: %s",
                        contextResult.errorMessage());
        return 1;
    }
    auto context = std::move(contextResult.value());

    // Create window
    frontend::WindowConfig windowConfig{};
    windowConfig.title = "Body Inspector Example";
    windowConfig.width = 1280;
    windowConfig.height = 720;
    windowConfig.visible = true;

    auto windowResult = frontend::Window::create(context.get(), windowConfig);
    if (windowResult.isFailure()) {
        AXIOM_LOG_ERROR("BodyInspector", "Failed to create window: %s",
                        windowResult.errorMessage());
        return 1;
    }
    auto window = std::move(windowResult.value());

    // Initialize ImGui renderer
    auto imguiResult = gui::ImGuiRenderer::create(context.get(), window.get());
    if (imguiResult.isFailure()) {
        AXIOM_LOG_ERROR("BodyInspector", "Failed to create ImGui renderer: %s",
                        imguiResult.errorMessage());
        return 1;
    }
    auto imgui = std::move(imguiResult.value());

    // Create body inspector
    gui::BodyInspector inspector;

    // Create some mock rigid bodies
    std::vector<MockRigidBody> bodies;
    bodies.emplace_back(1, "Ground", gui::BodyType::Static);
    bodies.back().data.shapeType = gui::ShapeType::Box;
    bodies.back().data.shapeExtents = math::Vec3(50.0f, 1.0f, 50.0f);

    bodies.emplace_back(2, "Falling Box", gui::BodyType::Dynamic);
    bodies.back().data.shapeType = gui::ShapeType::Box;
    bodies.back().data.shapeExtents = math::Vec3(1.0f, 1.0f, 1.0f);
    bodies.back().data.position = math::Vec3(0.0f, 15.0f, 0.0f);
    bodies.back().data.material.restitution = 0.7f;

    bodies.emplace_back(3, "Heavy Sphere", gui::BodyType::Dynamic);
    bodies.back().data.shapeType = gui::ShapeType::Sphere;
    bodies.back().data.shapeExtents = math::Vec3(0.5f, 0.0f, 0.0f);
    bodies.back().data.position = math::Vec3(2.0f, 20.0f, 0.0f);
    bodies.back().data.mass = 50.0f;
    bodies.back().data.material.density = 7850.0f;  // Steel

    bodies.emplace_back(4, "Light Ball", gui::BodyType::Dynamic);
    bodies.back().data.shapeType = gui::ShapeType::Sphere;
    bodies.back().data.shapeExtents = math::Vec3(0.3f, 0.0f, 0.0f);
    bodies.back().data.position = math::Vec3(-2.0f, 18.0f, 0.0f);
    bodies.back().data.mass = 1.0f;
    bodies.back().data.material.density = 100.0f;  // Light material

    bodies.emplace_back(5, "Moving Platform", gui::BodyType::Kinematic);
    bodies.back().data.shapeType = gui::ShapeType::Box;
    bodies.back().data.shapeExtents = math::Vec3(3.0f, 0.5f, 3.0f);
    bodies.back().data.position = math::Vec3(5.0f, 5.0f, 0.0f);

    // Track selected body
    int selectedBodyIndex = 1;  // Start with "Falling Box" selected

    // Timing
    auto lastTime = std::chrono::high_resolution_clock::now();

    AXIOM_LOG_INFO("BodyInspector", "Entering main loop");

    // Main loop
    while (!window->shouldClose()) {
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Cap delta time to prevent huge steps
        if (dt > 0.1f)
            dt = 0.1f;

        // Poll window events
        window->pollEvents();

        // Update physics simulation
        for (auto& body : bodies) {
            if (!body.data.sleep.isSleeping) {
                body.update(dt);
            }
        }

        // Start ImGui frame
        imgui->newFrame();

        // Create body selection window
        ImGui::Begin("Body List");
        ImGui::Text("Select a body to inspect:");
        ImGui::Separator();

        for (size_t i = 0; i < bodies.size(); ++i) {
            const auto& body = bodies[i].data;

            // Color code by type
            ImVec4 color;
            if (body.type == gui::BodyType::Static) {
                color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
            } else if (body.type == gui::BodyType::Dynamic) {
                color = body.sleep.isSleeping ? ImVec4(0.5f, 0.5f, 1.0f, 1.0f)
                                              : ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
            } else {
                color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);

            char label[128];
            snprintf(label, sizeof(label), "%u: %s", body.id, body.name.c_str());

            if (ImGui::Selectable(label, selectedBodyIndex == static_cast<int>(i))) {
                selectedBodyIndex = static_cast<int>(i);
                AXIOM_LOG_INFO("BodyInspector", "Selected body %u: %s", body.id, body.name.c_str());
            }

            ImGui::PopStyleColor();

            // Show status
            if (body.sleep.isSleeping) {
                ImGui::SameLine();
                ImGui::TextDisabled("(sleeping)");
            }
        }

        ImGui::Separator();
        ImGui::Text("Legend:");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  Static");
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "  Dynamic (awake)");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "  Dynamic (sleeping)");
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "  Kinematic");

        ImGui::End();

        // Render body inspector for selected body
        if (selectedBodyIndex >= 0 && selectedBodyIndex < static_cast<int>(bodies.size())) {
            auto& selectedBody = bodies[selectedBodyIndex];

            // Create window title with body name
            char windowTitle[256];
            snprintf(windowTitle, sizeof(windowTitle), "Body Inspector - %s",
                     selectedBody.data.name.c_str());

            // Render inspector and check if data was modified
            if (inspector.render(selectedBody.data, windowTitle)) {
                selectedBody.applyData(selectedBody.data);
            }
        }

        // Show help window
        ImGui::Begin("Help");
        ImGui::Text("Body Inspector Example");
        ImGui::Separator();
        ImGui::TextWrapped("This example demonstrates the BodyInspector GUI component.");
        ImGui::Spacing();
        ImGui::Text("Features:");
        ImGui::BulletText("View and edit body properties in real-time");
        ImGui::BulletText("Switch between Euler angles and quaternions");
        ImGui::BulletText("Adjust mass, material, and shape properties");
        ImGui::BulletText("Control sleep behavior");
        ImGui::BulletText("Simple physics simulation with gravity");
        ImGui::Spacing();
        ImGui::Text("Tips:");
        ImGui::BulletText("Select bodies from the 'Body List' window");
        ImGui::BulletText("Bodies turn blue when sleeping");
        ImGui::BulletText("Modify position or velocity to wake sleeping bodies");
        ImGui::BulletText("Try different restitution values (bounciness)");
        ImGui::Spacing();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    static_cast<double>(dt * 1000.0f), static_cast<double>(1.0f / dt));
        ImGui::End();

        // Finalize ImGui rendering
        ImGui::Render();

        // Present (normally you'd render to a command buffer and present)
        // For this example, we just sleep to limit frame rate
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
    }

    AXIOM_LOG_INFO("BodyInspector", "Shutting down");

    AXIOM_LOG_INFO("BodyInspector", "Exiting body inspector example");
    return 0;
}
