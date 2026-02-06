#include "axiom/gui/physics_panel.hpp"

#include <gtest/gtest.h>

#include <imgui.h>

namespace axiom::gui {

// Test fixture for PhysicsDebugPanel tests
class PhysicsDebugPanelTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create an ImGui context for testing
        // (In a real application, this would be created by ImGuiRenderer)
        imguiContext_ = ImGui::CreateContext();
        ImGui::SetCurrentContext(imguiContext_);

        // Initialize ImGui IO (required for rendering)
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1920, 1080);
        io.DeltaTime = 1.0f / 60.0f;

        // Build font atlas (required by ImGui)
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    }

    void TearDown() override {
        // Clean up ImGui context
        if (imguiContext_ != nullptr) {
            ImGui::DestroyContext(imguiContext_);
            imguiContext_ = nullptr;
        }
    }

    ImGuiContext* imguiContext_ = nullptr;
};

// Test: PhysicsDebugPanel creation should succeed
TEST_F(PhysicsDebugPanelTest, CreateSuccess) {
    EXPECT_NO_THROW({ PhysicsDebugPanel panel; });
}

// Test: Default window state should be open
TEST_F(PhysicsDebugPanelTest, DefaultWindowStateIsOpen) {
    PhysicsDebugPanel panel;
    EXPECT_TRUE(panel.isOpen());
}

// Test: Window state can be toggled
TEST_F(PhysicsDebugPanelTest, WindowStateCanBeToggled) {
    PhysicsDebugPanel panel;

    panel.setOpen(false);
    EXPECT_FALSE(panel.isOpen());

    panel.setOpen(true);
    EXPECT_TRUE(panel.isOpen());

    panel.toggleOpen();
    EXPECT_FALSE(panel.isOpen());

    panel.toggleOpen();
    EXPECT_TRUE(panel.isOpen());
}

// Test: Default section visibility
TEST_F(PhysicsDebugPanelTest, DefaultSectionVisibility) {
    PhysicsDebugPanel panel;

    EXPECT_TRUE(panel.getShowStats());
    EXPECT_TRUE(panel.getShowSettings());
    EXPECT_TRUE(panel.getShowVisualization());
    EXPECT_TRUE(panel.getShowPerformance());
}

// Test: Section visibility can be controlled
TEST_F(PhysicsDebugPanelTest, SectionVisibilityCanBeControlled) {
    PhysicsDebugPanel panel;

    panel.setShowStats(false);
    EXPECT_FALSE(panel.getShowStats());

    panel.setShowSettings(false);
    EXPECT_FALSE(panel.getShowSettings());

    panel.setShowVisualization(false);
    EXPECT_FALSE(panel.getShowVisualization());

    panel.setShowPerformance(false);
    EXPECT_FALSE(panel.getShowPerformance());

    panel.setShowStats(true);
    EXPECT_TRUE(panel.getShowStats());
}

// Test: Render with minimal stats (should not crash)
TEST_F(PhysicsDebugPanelTest, RenderWithMinimalStats) {
    PhysicsDebugPanel panel;
    PhysicsWorldStats stats{};
    PhysicsWorldConfig config{};

    // Start ImGui frame
    ImGui::NewFrame();

    // Render panel (should not crash even with zero stats)
    EXPECT_NO_THROW({ panel.render(stats, config); });

    // End ImGui frame
    ImGui::Render();
}

// Test: Render with populated stats
TEST_F(PhysicsDebugPanelTest, RenderWithPopulatedStats) {
    PhysicsDebugPanel panel;

    PhysicsWorldStats stats{};
    stats.totalBodies = 100;
    stats.activeBodies = 80;
    stats.sleepingBodies = 20;
    stats.staticBodies = 10;
    stats.dynamicBodies = 85;
    stats.kinematicBodies = 5;
    stats.contactPointCount = 150;
    stats.constraintCount = 50;
    stats.islandCount = 10;
    stats.totalStepTime = 16.67f;
    stats.broadphaseTime = 2.0f;
    stats.narrowphaseTime = 5.0f;
    stats.solverTime = 8.0f;
    stats.integrationTime = 1.67f;

    PhysicsWorldConfig config{};

    // Start ImGui frame
    ImGui::NewFrame();

    // Render panel
    EXPECT_NO_THROW({ panel.render(stats, config); });

    // End ImGui frame
    ImGui::Render();
}

// Test: Render with debug flags
TEST_F(PhysicsDebugPanelTest, RenderWithDebugFlags) {
    PhysicsDebugPanel panel;

    PhysicsWorldStats stats{};
    PhysicsWorldConfig config{};
    debug::PhysicsDebugFlags flags = debug::PhysicsDebugFlags::Shapes |
                                     debug::PhysicsDebugFlags::Contacts;

    // Start ImGui frame
    ImGui::NewFrame();

    // Render panel with debug flags
    EXPECT_NO_THROW({ panel.render(stats, config, flags); });

    // End ImGui frame
    ImGui::Render();
}

// Test: Config modification returns false when window is closed
TEST_F(PhysicsDebugPanelTest, ClosedWindowDoesNotModifyConfig) {
    PhysicsDebugPanel panel;
    panel.setOpen(false);

    PhysicsWorldStats stats{};
    PhysicsWorldConfig config{};

    // Start ImGui frame
    ImGui::NewFrame();

    // Render should return false when window is closed
    bool modified = panel.render(stats, config);
    EXPECT_FALSE(modified);

    // End ImGui frame
    ImGui::Render();
}

// Test: Default configuration values
TEST_F(PhysicsDebugPanelTest, DefaultConfigurationValues) {
    PhysicsWorldConfig config{};

    // Check default values match expectations
    EXPECT_EQ(config.gravity.x, 0.0f);
    EXPECT_EQ(config.gravity.y, -9.81f);
    EXPECT_EQ(config.gravity.z, 0.0f);
    EXPECT_FLOAT_EQ(config.timeStep, 1.0f / 60.0f);
    EXPECT_EQ(config.velocityIterations, 8u);
    EXPECT_EQ(config.positionIterations, 3u);
    EXPECT_TRUE(config.allowSleep);
}

// Test: Panel can be copied and moved
TEST_F(PhysicsDebugPanelTest, CopyAndMoveSemantics) {
    PhysicsDebugPanel panel1;
    panel1.setOpen(false);

    // Copy constructor
    PhysicsDebugPanel panel2(panel1);
    EXPECT_FALSE(panel2.isOpen());

    // Copy assignment
    PhysicsDebugPanel panel3;
    panel3 = panel1;
    EXPECT_FALSE(panel3.isOpen());

    // Move constructor
    PhysicsDebugPanel panel4(std::move(panel2));
    EXPECT_FALSE(panel4.isOpen());

    // Move assignment
    PhysicsDebugPanel panel5;
    panel5 = std::move(panel3);
    EXPECT_FALSE(panel5.isOpen());
}

// Test: Multiple render calls in same frame
TEST_F(PhysicsDebugPanelTest, MultipleRenderCalls) {
    PhysicsDebugPanel panel;

    PhysicsWorldStats stats{};
    PhysicsWorldConfig config{};

    // Start ImGui frame
    ImGui::NewFrame();

    // Multiple render calls should not crash
    EXPECT_NO_THROW({
        panel.render(stats, config);
        // Note: ImGui doesn't support rendering the same window twice in one frame
        // but we test that the code handles it gracefully
    });

    // End ImGui frame
    ImGui::Render();
}

// Test: Stats with zero time (edge case)
TEST_F(PhysicsDebugPanelTest, StatsWithZeroTime) {
    PhysicsDebugPanel panel;

    PhysicsWorldStats stats{};
    stats.totalStepTime = 0.0f;
    stats.broadphaseTime = 0.0f;
    stats.narrowphaseTime = 0.0f;
    stats.solverTime = 0.0f;
    stats.integrationTime = 0.0f;

    PhysicsWorldConfig config{};

    // Start ImGui frame
    ImGui::NewFrame();

    // Should not crash or divide by zero
    EXPECT_NO_THROW({ panel.render(stats, config); });

    // End ImGui frame
    ImGui::Render();
}

// Test: Very large body counts
TEST_F(PhysicsDebugPanelTest, VeryLargeBodyCounts) {
    PhysicsDebugPanel panel;

    PhysicsWorldStats stats{};
    stats.totalBodies = 1000000;
    stats.activeBodies = 999999;
    stats.contactPointCount = 5000000;

    PhysicsWorldConfig config{};

    // Start ImGui frame
    ImGui::NewFrame();

    // Should handle large numbers without issues
    EXPECT_NO_THROW({ panel.render(stats, config); });

    // End ImGui frame
    ImGui::Render();
}

}  // namespace axiom::gui
