#pragma once

#include "axiom/core/result.hpp"

#include <memory>
#include <vulkan/vulkan.h>

// Forward declarations
struct GLFWwindow;
struct ImGuiContext;

namespace axiom::gpu {
class VkContext;
}

namespace axiom::frontend {
class Window;
}

namespace axiom::gui {

// Import types from other modules
using axiom::frontend::Window;
using axiom::gpu::VkContext;

/// ImGui renderer for Vulkan/GLFW
///
/// This class provides a high-level interface to ImGui rendering with Vulkan
/// and GLFW backends. It handles all ImGui initialization, frame lifecycle,
/// and rendering operations.
///
/// Key features:
/// - Vulkan backend with descriptor pool management
/// - GLFW input handling (mouse, keyboard, scroll)
/// - Font texture upload to GPU
/// - Automatic frame lifecycle management
/// - Dark theme by default
///
/// Thread safety:
/// - All methods must be called from the main thread (ImGui requirement)
/// - ImGui is not thread-safe
///
/// Example usage:
/// @code
/// // Initialize ImGui renderer
/// auto renderer = ImGuiRenderer::create(vkContext, window);
/// if (renderer.isSuccess()) {
///     ImGuiRenderer& imgui = *renderer.value();
///
///     // Main loop
///     while (!window.shouldClose()) {
///         window.pollEvents();
///
///         // Start new ImGui frame
///         imgui.newFrame();
///
///         // Draw ImGui widgets
///         ImGui::Begin("Debug Panel");
///         ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
///         ImGui::End();
///
///         // Render to command buffer
///         VkCommandBuffer cmd = ...;
///         imgui.render(cmd);
///     }
/// }
/// @endcode
class ImGuiRenderer {
public:
    /// Create a new ImGui renderer
    ///
    /// This initializes both the Vulkan and GLFW backends for ImGui.
    /// It creates the necessary descriptor pool, uploads font textures,
    /// and configures ImGui for the specified window.
    ///
    /// @param context The Vulkan context (must be valid)
    /// @param window The window to render ImGui to (must be valid)
    /// @return Result containing the renderer on success, or error code on failure
    ///
    /// Error conditions:
    /// - InvalidParameter: context or window is null
    /// - VulkanInitializationFailed: Failed to initialize Vulkan backend
    /// - VulkanInitializationFailed: Failed to create descriptor pool
    static core::Result<std::unique_ptr<ImGuiRenderer>> create(VkContext* context, Window* window);

    /// Destructor - cleans up ImGui resources
    ~ImGuiRenderer();

    // Disable copy and move operations (ImGui context is not copyable)
    ImGuiRenderer(const ImGuiRenderer&) = delete;
    ImGuiRenderer& operator=(const ImGuiRenderer&) = delete;
    ImGuiRenderer(ImGuiRenderer&&) = delete;
    ImGuiRenderer& operator=(ImGuiRenderer&&) = delete;

    /// Start a new ImGui frame
    ///
    /// This should be called once per frame, before any ImGui drawing commands.
    /// It processes input events and prepares ImGui for drawing.
    ///
    /// Call order: window.pollEvents() -> newFrame() -> ImGui::* -> render()
    void newFrame();

    /// Render ImGui draw data to a command buffer
    ///
    /// This should be called after all ImGui drawing commands and before
    /// submitting the command buffer. It records rendering commands into
    /// the provided command buffer.
    ///
    /// The command buffer must be in the recording state (between begin/end).
    /// The caller is responsible for submitting the command buffer to the queue.
    ///
    /// @param commandBuffer The command buffer to record rendering commands into
    void render(VkCommandBuffer commandBuffer);

    /// Get the ImGui context
    ///
    /// This can be used to call ImGui functions directly or share the context
    /// across multiple systems. The context is owned by this renderer.
    ///
    /// @return The ImGui context pointer (never null for a valid renderer)
    ImGuiContext* getContext() const noexcept { return imguiContext_; }

private:
    /// Private constructor - use create() instead
    /// @param context The Vulkan context
    /// @param window The window to render to
    ImGuiRenderer(VkContext* context, Window* window);

    /// Initialize ImGui and backends
    /// @return Result indicating success or failure
    core::Result<void> initialize();

    /// Initialize the Vulkan backend
    /// @return Result indicating success or failure
    core::Result<void> initializeVulkanBackend();

    /// Initialize the GLFW backend
    /// @return Result indicating success or failure
    core::Result<void> initializeGLFWBackend();

    /// Create descriptor pool for ImGui
    /// @return Result indicating success or failure
    core::Result<void> createDescriptorPool();

    /// Upload font textures to GPU
    /// @return Result indicating success or failure
    core::Result<void> uploadFonts();

    /// Configure ImGui style (dark theme)
    void configureStyle();

    // Vulkan context (not owned)
    VkContext* context_ = nullptr;

    // Window (not owned)
    Window* window_ = nullptr;

    // ImGui context (owned)
    ImGuiContext* imguiContext_ = nullptr;

    // Descriptor pool for ImGui (owned, destroyed in destructor)
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;

    // Render pass for ImGui (owned, destroyed in destructor)
    VkRenderPass renderPass_ = VK_NULL_HANDLE;

    // Initialization state
    bool vulkanBackendInitialized_ = false;
    bool glfwBackendInitialized_ = false;
};

}  // namespace axiom::gui
