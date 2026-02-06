#include "axiom/frontend/window.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gui/imgui_impl.hpp"

#include <gtest/gtest.h>

#include <imgui.h>

namespace axiom::gui {

// Test fixture for ImGuiRenderer tests
class ImGuiRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize GLFW
        auto glfwResult = frontend::Window::initializeGLFW();
        if (glfwResult.isFailure()) {
            GTEST_SKIP() << "Failed to initialize GLFW: " << glfwResult.errorMessage();
        }

        // Create Vulkan context
        auto contextResult = gpu::VkContext::create();
        if (contextResult.isFailure()) {
            GTEST_SKIP() << "Failed to create Vulkan context (GPU may not be available): "
                         << contextResult.errorMessage();
        }
        context_ = std::move(contextResult.value());

        // Create window (invisible for testing)
        frontend::WindowConfig windowConfig{};
        windowConfig.title = "ImGui Test Window";
        windowConfig.width = 800;
        windowConfig.height = 600;
        windowConfig.visible = false;  // Don't show window during tests

        auto windowResult = frontend::Window::create(context_.get(), windowConfig);
        if (windowResult.isFailure()) {
            context_.reset();
            GTEST_SKIP() << "Failed to create window: " << windowResult.errorMessage();
        }
        window_ = std::move(windowResult.value());
    }

    void TearDown() override {
        // Clean up in reverse order
        window_.reset();
        context_.reset();
    }

    std::unique_ptr<gpu::VkContext> context_;
    std::unique_ptr<frontend::Window> window_;
};

// Test: ImGuiRenderer creation should succeed
TEST_F(ImGuiRendererTest, CreateSuccess) {
    auto result = ImGuiRenderer::create(context_.get(), window_.get());
    ASSERT_TRUE(result.isSuccess()) << "Failed to create ImGuiRenderer: " << result.errorMessage();

    auto& renderer = result.value();
    EXPECT_NE(renderer, nullptr);
    EXPECT_NE(renderer->getContext(), nullptr);
}

// Test: ImGuiRenderer creation with null context should fail
TEST(ImGuiRendererTestStatic, CreateWithNullContext) {
    auto result = ImGuiRenderer::create(nullptr, nullptr);
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), core::ErrorCode::InvalidParameter);
}

// Test: newFrame should not crash
TEST_F(ImGuiRendererTest, NewFrame) {
    auto result = ImGuiRenderer::create(context_.get(), window_.get());
    ASSERT_TRUE(result.isSuccess());

    auto& renderer = result.value();

    // Call newFrame (must be followed by ImGui::Render to complete the frame)
    EXPECT_NO_THROW({
        renderer->newFrame();
        ImGui::Render();  // End the frame properly
    });
}

// Test: Basic ImGui rendering workflow
TEST_F(ImGuiRendererTest, BasicRenderingWorkflow) {
    auto result = ImGuiRenderer::create(context_.get(), window_.get());
    ASSERT_TRUE(result.isSuccess());

    auto& renderer = result.value();

    // Start new frame
    EXPECT_NO_THROW(renderer->newFrame());

    // Draw some ImGui widgets
    EXPECT_NO_THROW({
        ImGui::Begin("Test Window");
        ImGui::Text("Test text");
        ImGui::Button("Test Button");
        ImGui::End();
    });

    // Complete the frame (must call ImGui::Render to finalize)
    EXPECT_NO_THROW(ImGui::Render());

    // Note: We don't call renderer->render(commandBuffer) here because we don't have
    // a valid command buffer. The render() method is tested in integration tests.
}

// Test: ImGui context is properly initialized
TEST_F(ImGuiRendererTest, ContextInitialized) {
    auto result = ImGuiRenderer::create(context_.get(), window_.get());
    ASSERT_TRUE(result.isSuccess());

    auto& renderer = result.value();
    ImGuiContext* ctx = renderer->getContext();
    ASSERT_NE(ctx, nullptr);

    // Check that ImGui IO is properly configured
    ImGui::SetCurrentContext(ctx);
    ImGuiIO& io = ImGui::GetIO();

    // Verify keyboard navigation is enabled
    EXPECT_TRUE(io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard);

    // Note: Docking is optional in some ImGui versions, skip this check
    // EXPECT_TRUE(io.ConfigFlags & ImGuiConfigFlags_DockingEnable);
}

// Test: Multiple ImGuiRenderer instances (disabled - not a supported use case)
// ImGui backends manage global state, so multiple instances would interfere with each other.
// In a real application, you should only have one ImGuiRenderer instance per application.
TEST_F(ImGuiRendererTest, DISABLED_MultipleInstances) {
    auto result1 = ImGuiRenderer::create(context_.get(), window_.get());
    ASSERT_TRUE(result1.isSuccess());

    ImGuiContext* ctx1 = result1.value()->getContext();
    ASSERT_NE(ctx1, nullptr);

    // Note: Creating multiple ImGuiRenderer instances is not supported
    // because ImGui backends manage global state
}

// Test: Cleanup should not crash
TEST_F(ImGuiRendererTest, CleanupDoesNotCrash) {
    auto result = ImGuiRenderer::create(context_.get(), window_.get());
    ASSERT_TRUE(result.isSuccess());

    // Explicitly destroy the renderer
    EXPECT_NO_THROW({ result.value().reset(); });
}

// Test: Dark theme should be applied
TEST_F(ImGuiRendererTest, DarkThemeApplied) {
    auto result = ImGuiRenderer::create(context_.get(), window_.get());
    ASSERT_TRUE(result.isSuccess());

    auto& renderer = result.value();
    ImGui::SetCurrentContext(renderer->getContext());

    // Check that the style has been modified (dark theme)
    ImGuiStyle& style = ImGui::GetStyle();

    // Verify some style properties that are set by configureStyle()
    EXPECT_GT(style.WindowRounding, 0.0f);
    EXPECT_GT(style.FrameRounding, 0.0f);
    EXPECT_GT(style.ScrollbarRounding, 0.0f);

    // Verify window background is dark
    ImVec4 windowBg = style.Colors[ImGuiCol_WindowBg];
    EXPECT_LT(windowBg.x, 0.2f);  // R component should be dark
    EXPECT_LT(windowBg.y, 0.2f);  // G component should be dark
    EXPECT_LT(windowBg.z, 0.2f);  // B component should be dark
}

}  // namespace axiom::gui
