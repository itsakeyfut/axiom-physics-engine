#include "axiom/debug/debug_draw.hpp"
#include "axiom/debug/physics_debug_draw.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_memory.hpp"
#include "axiom/math/aabb.hpp"
#include "axiom/math/constants.hpp"
#include "axiom/math/quat.hpp"
#include "axiom/math/transform.hpp"
#include "axiom/math/vec3.hpp"
#include "axiom/math/vec4.hpp"

#include <gtest/gtest.h>

#include <filesystem>

using namespace axiom::debug;
using namespace axiom::gpu;
using namespace axiom::math;

// Test fixture for PhysicsDebugDraw tests
class PhysicsDebugDrawTest : public ::testing::Test {
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

        // Create debug draw system
        debugDraw_ = std::make_unique<DebugDraw>(context_.get(), memManager_.get());
    }

    void TearDown() override {
        physicsDebugDraw_.reset();
        debugDraw_.reset();
        memManager_.reset();
        context_.reset();
    }

    void createPhysicsDebugDraw(const PhysicsDebugDrawConfig& config = {}) {
        physicsDebugDraw_ = std::make_unique<PhysicsDebugDraw>(debugDraw_.get(), config);
    }

    std::unique_ptr<VkContext> context_;
    std::unique_ptr<VkMemoryManager> memManager_;
    std::unique_ptr<DebugDraw> debugDraw_;
    std::unique_ptr<PhysicsDebugDraw> physicsDebugDraw_;
};

// Test basic construction
TEST_F(PhysicsDebugDrawTest, Construction) {
    ASSERT_NO_THROW({ createPhysicsDebugDraw(); });
    ASSERT_NE(physicsDebugDraw_, nullptr);
}

// Test construction with custom config
TEST_F(PhysicsDebugDrawTest, ConstructionWithConfig) {
    PhysicsDebugDrawConfig config{};
    config.flags = PhysicsDebugFlags::All;
    config.depthTestEnabled = false;
    config.contactNormalLength = 0.5f;

    ASSERT_NO_THROW({ createPhysicsDebugDraw(config); });
    ASSERT_NE(physicsDebugDraw_, nullptr);

    EXPECT_EQ(physicsDebugDraw_->getFlags(), PhysicsDebugFlags::All);
    EXPECT_EQ(physicsDebugDraw_->getDepthTestEnabled(), false);
}

// Test flag operations
TEST_F(PhysicsDebugDrawTest, FlagOperations) {
    createPhysicsDebugDraw();

    // Test setting flags
    physicsDebugDraw_->setFlags(PhysicsDebugFlags::Shapes | PhysicsDebugFlags::AABBs);
    PhysicsDebugFlags flags = physicsDebugDraw_->getFlags();

    EXPECT_TRUE(hasFlag(flags, PhysicsDebugFlags::Shapes));
    EXPECT_TRUE(hasFlag(flags, PhysicsDebugFlags::AABBs));
    EXPECT_FALSE(hasFlag(flags, PhysicsDebugFlags::Contacts));

    // Test None flag
    physicsDebugDraw_->setFlags(PhysicsDebugFlags::None);
    EXPECT_EQ(physicsDebugDraw_->getFlags(), PhysicsDebugFlags::None);

    // Test All flag
    physicsDebugDraw_->setFlags(PhysicsDebugFlags::All);
    EXPECT_TRUE(hasFlag(physicsDebugDraw_->getFlags(), PhysicsDebugFlags::Shapes));
    EXPECT_TRUE(hasFlag(physicsDebugDraw_->getFlags(), PhysicsDebugFlags::Contacts));
}

// Test drawing a sphere shape
TEST_F(PhysicsDebugDrawTest, DrawSphereShape) {
    createPhysicsDebugDraw();

    DebugShape sphere;
    sphere.type = ShapeType::Sphere;
    sphere.transform = Transform(Vec3(0, 0, 0), Quat::identity());
    sphere.radius = 1.0f;

    size_t vertexCountBefore = debugDraw_->getVertexCount();
    ASSERT_NO_THROW({ physicsDebugDraw_->drawCollisionShape(sphere); });
    size_t vertexCountAfter = debugDraw_->getVertexCount();

    // Should have added vertices for the sphere wireframe
    EXPECT_GT(vertexCountAfter, vertexCountBefore);
}

// Test drawing a box shape
TEST_F(PhysicsDebugDrawTest, DrawBoxShape) {
    createPhysicsDebugDraw();

    DebugShape box;
    box.type = ShapeType::Box;
    box.transform = Transform(Vec3(0, 0, 0), Quat::identity());
    box.halfExtents = Vec3(1.0f, 1.0f, 1.0f);

    size_t vertexCountBefore = debugDraw_->getVertexCount();
    ASSERT_NO_THROW({ physicsDebugDraw_->drawCollisionShape(box); });
    size_t vertexCountAfter = debugDraw_->getVertexCount();

    // Should have added vertices for the box wireframe (12 edges = 24 vertices)
    EXPECT_GE(vertexCountAfter - vertexCountBefore, 24);
}

// Test drawing a capsule shape
TEST_F(PhysicsDebugDrawTest, DrawCapsuleShape) {
    createPhysicsDebugDraw();

    DebugShape capsule;
    capsule.type = ShapeType::Capsule;
    capsule.transform = Transform(Vec3(0, 0, 0), Quat::identity());
    capsule.radius = 0.5f;
    capsule.height = 2.0f;

    size_t vertexCountBefore = debugDraw_->getVertexCount();
    ASSERT_NO_THROW({ physicsDebugDraw_->drawCollisionShape(capsule); });
    size_t vertexCountAfter = debugDraw_->getVertexCount();

    // Should have added vertices for the capsule wireframe
    EXPECT_GT(vertexCountAfter, vertexCountBefore);
}

// Test drawing a plane shape
TEST_F(PhysicsDebugDrawTest, DrawPlaneShape) {
    createPhysicsDebugDraw();

    DebugShape plane;
    plane.type = ShapeType::Plane;
    plane.transform = Transform(Vec3(0, 0, 0), Quat::identity());
    plane.normal = Vec3(0, 1, 0);  // XZ plane

    size_t vertexCountBefore = debugDraw_->getVertexCount();
    ASSERT_NO_THROW({ physicsDebugDraw_->drawCollisionShape(plane); });
    size_t vertexCountAfter = debugDraw_->getVertexCount();

    // Should have added vertices for the plane visualization
    EXPECT_GT(vertexCountAfter, vertexCountBefore);
}

// Test drawing a contact point
TEST_F(PhysicsDebugDrawTest, DrawContactPoint) {
    createPhysicsDebugDraw();

    DebugContactPoint contact;
    contact.position = Vec3(0, 0, 0);
    contact.normal = Vec3(0, 1, 0);
    contact.penetrationDepth = 0.1f;

    size_t vertexCountBefore = debugDraw_->getVertexCount();
    ASSERT_NO_THROW({ physicsDebugDraw_->drawContactPoint(contact); });
    size_t vertexCountAfter = debugDraw_->getVertexCount();

    // Should have added vertices for contact visualization
    EXPECT_GT(vertexCountAfter, vertexCountBefore);
}

// Test drawing a contact point with flags disabled
TEST_F(PhysicsDebugDrawTest, DrawContactPointDisabled) {
    PhysicsDebugDrawConfig config;
    config.flags = PhysicsDebugFlags::Shapes;  // Contacts disabled
    createPhysicsDebugDraw(config);

    DebugContactPoint contact;
    contact.position = Vec3(0, 0, 0);
    contact.normal = Vec3(0, 1, 0);

    size_t vertexCountBefore = debugDraw_->getVertexCount();
    physicsDebugDraw_->drawContactPoint(contact);
    size_t vertexCountAfter = debugDraw_->getVertexCount();

    // Should not have added any vertices
    EXPECT_EQ(vertexCountAfter, vertexCountBefore);
}

// Test drawing a constraint
TEST_F(PhysicsDebugDrawTest, DrawConstraint) {
    createPhysicsDebugDraw();

    DebugConstraint constraint;
    constraint.anchorA = Vec3(0, 0, 0);
    constraint.anchorB = Vec3(1, 1, 1);

    size_t vertexCountBefore = debugDraw_->getVertexCount();
    ASSERT_NO_THROW({ physicsDebugDraw_->drawConstraint(constraint); });
    size_t vertexCountAfter = debugDraw_->getVertexCount();

    // Should have added vertices for constraint visualization
    EXPECT_GT(vertexCountAfter, vertexCountBefore);
}

// Test drawing an AABB
TEST_F(PhysicsDebugDrawTest, DrawAABB) {
    createPhysicsDebugDraw();

    AABB aabb(Vec3(-1, -1, -1), Vec3(1, 1, 1));

    size_t vertexCountBefore = debugDraw_->getVertexCount();
    ASSERT_NO_THROW({ physicsDebugDraw_->drawAABB(aabb); });
    size_t vertexCountAfter = debugDraw_->getVertexCount();

    // Should have added vertices for AABB wireframe
    EXPECT_GT(vertexCountAfter, vertexCountBefore);
}

// Test drawing velocity vector
TEST_F(PhysicsDebugDrawTest, DrawVelocity) {
    createPhysicsDebugDraw();

    Vec3 position(0, 0, 0);
    Vec3 velocity(1, 2, 3);

    size_t vertexCountBefore = debugDraw_->getVertexCount();
    ASSERT_NO_THROW({ physicsDebugDraw_->drawVelocity(position, velocity); });
    size_t vertexCountAfter = debugDraw_->getVertexCount();

    // Should have added vertices for velocity arrow
    EXPECT_GT(vertexCountAfter, vertexCountBefore);
}

// Test drawing negligible velocity (should skip)
TEST_F(PhysicsDebugDrawTest, DrawNegligibleVelocity) {
    createPhysicsDebugDraw();

    Vec3 position(0, 0, 0);
    Vec3 velocity(0.0001f, 0.0001f, 0.0001f);  // Very small

    size_t vertexCountBefore = debugDraw_->getVertexCount();
    physicsDebugDraw_->drawVelocity(position, velocity);
    size_t vertexCountAfter = debugDraw_->getVertexCount();

    // Should not have added any vertices (velocity too small)
    EXPECT_EQ(vertexCountAfter, vertexCountBefore);
}

// Test drawing force vector
TEST_F(PhysicsDebugDrawTest, DrawForce) {
    createPhysicsDebugDraw();

    Vec3 position(0, 0, 0);
    Vec3 force(100, 200, 300);

    size_t vertexCountBefore = debugDraw_->getVertexCount();
    ASSERT_NO_THROW({ physicsDebugDraw_->drawForce(position, force); });
    size_t vertexCountAfter = debugDraw_->getVertexCount();

    // Should have added vertices for force arrow
    EXPECT_GT(vertexCountAfter, vertexCountBefore);
}

// Test drawing angular velocity
TEST_F(PhysicsDebugDrawTest, DrawAngularVelocity) {
    createPhysicsDebugDraw();

    Vec3 position(0, 0, 0);
    Vec3 angularVelocity(0, PI_F, 0);  // Rotation around Y-axis

    size_t vertexCountBefore = debugDraw_->getVertexCount();
    ASSERT_NO_THROW({ physicsDebugDraw_->drawAngularVelocity(position, angularVelocity); });
    size_t vertexCountAfter = debugDraw_->getVertexCount();

    // Should have added vertices for angular velocity visualization
    EXPECT_GT(vertexCountAfter, vertexCountBefore);
}

// Test drawing center of mass
TEST_F(PhysicsDebugDrawTest, DrawCenterOfMass) {
    createPhysicsDebugDraw();

    Vec3 position(0, 0, 0);

    size_t vertexCountBefore = debugDraw_->getVertexCount();
    ASSERT_NO_THROW({ physicsDebugDraw_->drawCenterOfMass(position); });
    size_t vertexCountAfter = debugDraw_->getVertexCount();

    // Should have added vertices for center of mass marker
    EXPECT_GT(vertexCountAfter, vertexCountBefore);
}

// Test drawing a complete rigid body
TEST_F(PhysicsDebugDrawTest, DrawRigidBody) {
    PhysicsDebugDrawConfig config;
    config.flags = PhysicsDebugFlags::All;
    createPhysicsDebugDraw(config);

    DebugRigidBody body;
    body.shape.type = ShapeType::Box;
    body.shape.halfExtents = Vec3(1.0f, 1.0f, 1.0f);
    body.transform = Transform(Vec3(0, 5, 0), Quat::identity());
    body.linearVelocity = Vec3(0, -1, 0);
    body.angularVelocity = Vec3(0, PI_F / 2, 0);
    body.force = Vec3(0, -98.1f, 0);  // Gravity
    body.centerOfMass = Vec3(0, 0, 0);
    body.aabb = AABB(Vec3(-1, 4, -1), Vec3(1, 6, 1));
    body.isAwake = true;
    body.islandIndex = 0;

    size_t vertexCountBefore = debugDraw_->getVertexCount();
    ASSERT_NO_THROW({ physicsDebugDraw_->drawRigidBody(body); });
    size_t vertexCountAfter = debugDraw_->getVertexCount();

    // Should have added many vertices (shape + AABB + axes + com + velocity + force)
    EXPECT_GT(vertexCountAfter, vertexCountBefore);
}

// Test drawing a sleeping rigid body
TEST_F(PhysicsDebugDrawTest, DrawSleepingRigidBody) {
    createPhysicsDebugDraw();

    DebugRigidBody body;
    body.shape.type = ShapeType::Sphere;
    body.shape.radius = 1.0f;
    body.transform = Transform(Vec3(0, 0, 0), Quat::identity());
    body.isAwake = false;  // Sleeping
    body.aabb = AABB(Vec3(-1, -1, -1), Vec3(1, 1, 1));

    // Should draw without errors (color will be darker for sleeping bodies)
    ASSERT_NO_THROW({ physicsDebugDraw_->drawRigidBody(body); });
}

// Test drawing rigid bodies with island coloring
TEST_F(PhysicsDebugDrawTest, DrawRigidBodyWithIslandColoring) {
    PhysicsDebugDrawConfig config;
    config.flags = PhysicsDebugFlags::Shapes | PhysicsDebugFlags::Islands;
    createPhysicsDebugDraw(config);

    DebugRigidBody body1;
    body1.shape.type = ShapeType::Sphere;
    body1.shape.radius = 1.0f;
    body1.transform = Transform(Vec3(0, 0, 0), Quat::identity());
    body1.islandIndex = 0;
    body1.aabb = AABB(Vec3(-1, -1, -1), Vec3(1, 1, 1));

    DebugRigidBody body2;
    body2.shape.type = ShapeType::Sphere;
    body2.shape.radius = 1.0f;
    body2.transform = Transform(Vec3(5, 0, 0), Quat::identity());
    body2.islandIndex = 1;  // Different island
    body2.aabb = AABB(Vec3(4, -1, -1), Vec3(6, 1, 1));

    ASSERT_NO_THROW({
        physicsDebugDraw_->drawRigidBody(body1);
        physicsDebugDraw_->drawRigidBody(body2);
    });
}

// Test depth test configuration
TEST_F(PhysicsDebugDrawTest, DepthTestConfiguration) {
    createPhysicsDebugDraw();

    EXPECT_TRUE(physicsDebugDraw_->getDepthTestEnabled());

    physicsDebugDraw_->setDepthTestEnabled(false);
    EXPECT_FALSE(physicsDebugDraw_->getDepthTestEnabled());

    physicsDebugDraw_->setDepthTestEnabled(true);
    EXPECT_TRUE(physicsDebugDraw_->getDepthTestEnabled());
}

// Test config get/set
TEST_F(PhysicsDebugDrawTest, ConfigGetSet) {
    createPhysicsDebugDraw();

    PhysicsDebugDrawConfig config;
    config.flags = PhysicsDebugFlags::All;
    config.contactNormalLength = 0.5f;
    config.velocityScale = 0.2f;
    config.forceScale = 0.002f;

    physicsDebugDraw_->setConfig(config);
    const PhysicsDebugDrawConfig& retrievedConfig = physicsDebugDraw_->getConfig();

    EXPECT_EQ(retrievedConfig.flags, config.flags);
    EXPECT_FLOAT_EQ(retrievedConfig.contactNormalLength, config.contactNormalLength);
    EXPECT_FLOAT_EQ(retrievedConfig.velocityScale, config.velocityScale);
    EXPECT_FLOAT_EQ(retrievedConfig.forceScale, config.forceScale);
}

// Test drawing with various shape types
TEST_F(PhysicsDebugDrawTest, DrawVariousShapeTypes) {
    createPhysicsDebugDraw();

    // Sphere
    DebugShape sphere;
    sphere.type = ShapeType::Sphere;
    sphere.transform = Transform(Vec3(0, 0, 0), Quat::identity());
    sphere.radius = 1.0f;
    ASSERT_NO_THROW({ physicsDebugDraw_->drawCollisionShape(sphere); });

    // Box
    DebugShape box;
    box.type = ShapeType::Box;
    box.transform = Transform(Vec3(3, 0, 0), Quat::identity());
    box.halfExtents = Vec3(0.5f, 1.0f, 0.5f);
    ASSERT_NO_THROW({ physicsDebugDraw_->drawCollisionShape(box); });

    // Capsule
    DebugShape capsule;
    capsule.type = ShapeType::Capsule;
    capsule.transform = Transform(Vec3(6, 0, 0), Quat::identity());
    capsule.radius = 0.5f;
    capsule.height = 2.0f;
    ASSERT_NO_THROW({ physicsDebugDraw_->drawCollisionShape(capsule); });

    // Plane
    DebugShape plane;
    plane.type = ShapeType::Plane;
    plane.transform = Transform(Vec3(0, -1, 0), Quat::identity());
    plane.normal = Vec3(0, 1, 0);
    ASSERT_NO_THROW({ physicsDebugDraw_->drawCollisionShape(plane); });

    // Should have drawn all shapes
    EXPECT_GT(debugDraw_->getVertexCount(), 0);
}
