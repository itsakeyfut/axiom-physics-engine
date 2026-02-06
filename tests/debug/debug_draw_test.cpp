#include "axiom/debug/debug_draw.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_memory.hpp"
#include "axiom/math/mat4.hpp"
#include "axiom/math/transform.hpp"
#include "axiom/math/vec3.hpp"
#include "axiom/math/vec4.hpp"

#include <gtest/gtest.h>

#include <filesystem>

using namespace axiom::debug;
using namespace axiom::gpu;
using namespace axiom::math;

// Test fixture for DebugDraw tests
class DebugDrawTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create Vulkan context
        auto contextResult = VkContext::create();
        if (contextResult.isSuccess()) {
            context_ = std::move(contextResult.value());
        } else {
            GTEST_SKIP() << "Vulkan not available: " << contextResult.errorMessage()
                         << " (this is expected in CI environments without GPU)";
        }

        // Create memory manager
        auto memManagerResult = VkMemoryManager::create(context_.get());
        if (memManagerResult.isSuccess()) {
            memManager_ = std::move(memManagerResult.value());
        } else {
            GTEST_SKIP() << "Failed to create memory manager: " << memManagerResult.errorMessage();
        }

        // Check if shaders are available
        if (!std::filesystem::exists("shaders/debug/line.vert.spv") ||
            !std::filesystem::exists("shaders/debug/line.frag.spv")) {
            GTEST_SKIP() << "Debug shaders not found (compile shaders/debug/*.vert/frag with "
                            "glslangValidator or slangc)";
        }
    }

    void TearDown() override {
        debugDraw_.reset();
        memManager_.reset();
        context_.reset();
    }

    void createDebugDraw() {
        debugDraw_ = std::make_unique<DebugDraw>(context_.get(), memManager_.get());
    }

    std::unique_ptr<VkContext> context_;
    std::unique_ptr<VkMemoryManager> memManager_;
    std::unique_ptr<DebugDraw> debugDraw_;
};

// Test basic construction
TEST_F(DebugDrawTest, Construction) {
    ASSERT_NO_THROW({ createDebugDraw(); });
    ASSERT_NE(debugDraw_, nullptr);
}

// Test construction with custom config
TEST_F(DebugDrawTest, ConstructionWithConfig) {
    DebugDrawConfig config{};
    config.initialVertexCapacity = 5000;
    config.depthTestEnabled = false;

    ASSERT_NO_THROW(
        { debugDraw_ = std::make_unique<DebugDraw>(context_.get(), memManager_.get(), config); });

    ASSERT_NE(debugDraw_, nullptr);
    EXPECT_EQ(debugDraw_->getDepthTestEnabled(), false);
}

// Test drawing a single line
TEST_F(DebugDrawTest, DrawLine) {
    createDebugDraw();

    Vec3 start(0, 0, 0);
    Vec3 end(1, 1, 1);
    Vec4 color(1, 0, 0, 1);

    ASSERT_NO_THROW({ debugDraw_->drawLine(start, end, color); });

    // Should have 2 vertices (1 line = 2 vertices)
    EXPECT_EQ(debugDraw_->getVertexCount(), 2);
}

// Test drawing multiple lines
TEST_F(DebugDrawTest, DrawMultipleLines) {
    createDebugDraw();

    for (int i = 0; i < 10; ++i) {
        Vec3 start(static_cast<float>(i), 0, 0);
        Vec3 end(static_cast<float>(i), 1, 0);
        Vec4 color(1, 1, 1, 1);
        debugDraw_->drawLine(start, end, color);
    }

    // Should have 20 vertices (10 lines * 2 vertices)
    EXPECT_EQ(debugDraw_->getVertexCount(), 20);
}

// Test drawing a box (AABB)
TEST_F(DebugDrawTest, DrawBoxAABB) {
    createDebugDraw();

    Vec3 min(-1, -1, -1);
    Vec3 max(1, 1, 1);
    Vec4 color(0, 1, 0, 1);

    ASSERT_NO_THROW({ debugDraw_->drawBox(min, max, color); });

    // Box has 12 edges, each edge = 2 vertices = 24 vertices total
    EXPECT_EQ(debugDraw_->getVertexCount(), 24);
}

// Test drawing an oriented box
TEST_F(DebugDrawTest, DrawBoxOriented) {
    createDebugDraw();

    Transform transform;
    transform.position = Vec3(5, 0, 0);
    Vec3 halfExtents(1, 2, 3);
    Vec4 color(0, 0, 1, 1);

    ASSERT_NO_THROW({ debugDraw_->drawBox(transform, halfExtents, color); });

    // Box has 12 edges = 24 vertices
    EXPECT_EQ(debugDraw_->getVertexCount(), 24);
}

// Test drawing a sphere
TEST_F(DebugDrawTest, DrawSphere) {
    createDebugDraw();

    Vec3 center(0, 0, 0);
    float radius = 1.0f;
    Vec4 color(1, 1, 0, 1);
    int segments = 8;

    ASSERT_NO_THROW({ debugDraw_->drawSphere(center, radius, color, segments); });

    // Sphere draws meridians and parallels
    // Each segment creates multiple lines
    EXPECT_GT(debugDraw_->getVertexCount(), 0);
}

// Test drawing a capsule
TEST_F(DebugDrawTest, DrawCapsule) {
    createDebugDraw();

    Vec3 start(0, 0, 0);
    Vec3 end(0, 5, 0);
    float radius = 1.0f;
    Vec4 color(1, 0, 1, 1);

    ASSERT_NO_THROW({ debugDraw_->drawCapsule(start, end, radius, color); });

    EXPECT_GT(debugDraw_->getVertexCount(), 0);
}

// Test drawing a cone
TEST_F(DebugDrawTest, DrawCone) {
    createDebugDraw();

    Vec3 base(0, 0, 0);
    Vec3 tip(0, 2, 0);
    float radius = 0.5f;
    Vec4 color(0, 1, 1, 1);

    ASSERT_NO_THROW({ debugDraw_->drawCone(base, tip, radius, color); });

    EXPECT_GT(debugDraw_->getVertexCount(), 0);
}

// Test drawing an arrow
TEST_F(DebugDrawTest, DrawArrow) {
    createDebugDraw();

    Vec3 start(0, 0, 0);
    Vec3 end(3, 0, 0);
    Vec4 color(1, 0, 0, 1);

    ASSERT_NO_THROW({ debugDraw_->drawArrow(start, end, color); });

    EXPECT_GT(debugDraw_->getVertexCount(), 0);
}

// Test drawing a plane
TEST_F(DebugDrawTest, DrawPlane) {
    createDebugDraw();

    Vec3 center(0, 0, 0);
    Vec3 normal(0, 1, 0);
    float size = 5.0f;
    Vec4 color(0.5f, 0.5f, 0.5f, 1);

    ASSERT_NO_THROW({ debugDraw_->drawPlane(center, normal, size, color); });

    EXPECT_GT(debugDraw_->getVertexCount(), 0);
}

// Test drawing coordinate axes
TEST_F(DebugDrawTest, DrawAxis) {
    createDebugDraw();

    Transform transform = Transform::identity();

    ASSERT_NO_THROW({ debugDraw_->drawAxis(transform); });

    // Axis draws 3 arrows (X, Y, Z)
    EXPECT_GT(debugDraw_->getVertexCount(), 0);
}

// Test drawing a grid
TEST_F(DebugDrawTest, DrawGrid) {
    createDebugDraw();

    Vec3 center(0, 0, 0);
    float size = 10.0f;
    int divisions = 10;
    Vec4 color(0.3f, 0.3f, 0.3f, 1);

    ASSERT_NO_THROW({ debugDraw_->drawGrid(center, size, divisions, color); });

    // Grid draws (divisions+1)*2 lines
    // Each line = 2 vertices
    int expectedLines = (divisions + 1) * 2;
    int expectedVertices = expectedLines * 2;
    EXPECT_EQ(debugDraw_->getVertexCount(), expectedVertices);
}

// Test clear functionality
TEST_F(DebugDrawTest, Clear) {
    createDebugDraw();

    // Draw some lines
    for (int i = 0; i < 5; ++i) {
        debugDraw_->drawLine(Vec3(0, 0, 0), Vec3(1, 1, 1), Vec4(1, 1, 1, 1));
    }

    EXPECT_EQ(debugDraw_->getVertexCount(), 10);

    // Clear
    debugDraw_->clear();

    EXPECT_EQ(debugDraw_->getVertexCount(), 0);
}

// Test depth test enable/disable
TEST_F(DebugDrawTest, DepthTestToggle) {
    createDebugDraw();

    // Default state
    EXPECT_TRUE(debugDraw_->getDepthTestEnabled());

    // Disable
    debugDraw_->setDepthTestEnabled(false);
    EXPECT_FALSE(debugDraw_->getDepthTestEnabled());

    // Re-enable
    debugDraw_->setDepthTestEnabled(true);
    EXPECT_TRUE(debugDraw_->getDepthTestEnabled());
}

// Test drawing many primitives (stress test)
TEST_F(DebugDrawTest, DrawManyPrimitives) {
    createDebugDraw();

    // Draw many lines
    for (int i = 0; i < 1000; ++i) {
        float x = static_cast<float>(i % 10);
        float y = static_cast<float>(i / 10);
        debugDraw_->drawLine(Vec3(x, y, 0), Vec3(x + 0.5f, y + 0.5f, 0), Vec4(1, 1, 1, 1));
    }

    // Should handle 1000 lines (2000 vertices)
    EXPECT_EQ(debugDraw_->getVertexCount(), 2000);
}

// Test complex scene
TEST_F(DebugDrawTest, DrawComplexScene) {
    createDebugDraw();

    // Draw grid
    debugDraw_->drawGrid(Vec3(0, 0, 0), 20.0f, 20, Vec4(0.3f, 0.3f, 0.3f, 1));

    // Draw coordinate axes
    debugDraw_->drawAxis(Transform::identity(), 2.0f);

    // Draw some boxes
    for (int i = 0; i < 5; ++i) {
        Vec3 pos(static_cast<float>(i) * 3.0f, 0, 0);
        Transform t(pos);
        debugDraw_->drawBox(t, Vec3(1, 1, 1), Vec4(0, 1, 0, 1));
    }

    // Draw some spheres
    for (int i = 0; i < 3; ++i) {
        Vec3 pos(0, static_cast<float>(i) * 3.0f, 0);
        debugDraw_->drawSphere(pos, 1.0f, Vec4(1, 0, 0, 1), 12);
    }

    // Should have accumulated many vertices
    EXPECT_GT(debugDraw_->getVertexCount(), 100);
}

// Note: Actual rendering tests (flush with command buffer) would require
// a full rendering setup with swapchain, render pass, etc. Those tests
// are better suited for integration tests or visual debugging tools.
