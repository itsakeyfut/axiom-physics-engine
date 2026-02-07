#include "axiom/gui/body_inspector.hpp"

#include <gtest/gtest.h>

using namespace axiom::gui;
using namespace axiom::math;

// Test fixture for BodyInspector tests
class BodyInspectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create default body data
        bodyData_ = RigidBodyData{};
        bodyData_.id = 42;
        bodyData_.type = BodyType::Dynamic;
        bodyData_.name = "TestBody";
        bodyData_.position = Vec3(1.0f, 2.0f, 3.0f);
        bodyData_.rotation = Quat(0.0f, 0.0f, 0.0f, 1.0f);
        bodyData_.linearVelocity = Vec3(0.5f, 0.0f, 0.0f);
        bodyData_.angularVelocity = Vec3(0.0f, 0.1f, 0.0f);
        bodyData_.mass = 10.0f;
        bodyData_.inertiaTensor = Vec3(1.0f, 1.0f, 1.0f);
    }

    RigidBodyData bodyData_;
    BodyInspector inspector_;
};

// === Constructor Tests ===

TEST_F(BodyInspectorTest, DefaultConstructor) {
    BodyInspector inspector;

    // Verify default state
    EXPECT_TRUE(inspector.isOpen());
    EXPECT_TRUE(inspector.getShowIdentity());
    EXPECT_TRUE(inspector.getShowTransform());
    EXPECT_TRUE(inspector.getShowDynamics());
    EXPECT_TRUE(inspector.getShowMassProperties());
    EXPECT_TRUE(inspector.getShowShape());
    EXPECT_TRUE(inspector.getShowMaterial());
    EXPECT_TRUE(inspector.getShowFiltering());
    EXPECT_TRUE(inspector.getShowSleep());
    EXPECT_TRUE(inspector.getUseEulerAngles());
}

// === Window State Tests ===

TEST_F(BodyInspectorTest, WindowOpenClose) {
    EXPECT_TRUE(inspector_.isOpen());

    inspector_.setOpen(false);
    EXPECT_FALSE(inspector_.isOpen());

    inspector_.setOpen(true);
    EXPECT_TRUE(inspector_.isOpen());
}

TEST_F(BodyInspectorTest, WindowToggle) {
    bool initialState = inspector_.isOpen();

    inspector_.toggleOpen();
    EXPECT_NE(inspector_.isOpen(), initialState);

    inspector_.toggleOpen();
    EXPECT_EQ(inspector_.isOpen(), initialState);
}

// === Section Visibility Tests ===

TEST_F(BodyInspectorTest, IdentitySectionVisibility) {
    inspector_.setShowIdentity(false);
    EXPECT_FALSE(inspector_.getShowIdentity());

    inspector_.setShowIdentity(true);
    EXPECT_TRUE(inspector_.getShowIdentity());
}

TEST_F(BodyInspectorTest, TransformSectionVisibility) {
    inspector_.setShowTransform(false);
    EXPECT_FALSE(inspector_.getShowTransform());

    inspector_.setShowTransform(true);
    EXPECT_TRUE(inspector_.getShowTransform());
}

TEST_F(BodyInspectorTest, DynamicsSectionVisibility) {
    inspector_.setShowDynamics(false);
    EXPECT_FALSE(inspector_.getShowDynamics());

    inspector_.setShowDynamics(true);
    EXPECT_TRUE(inspector_.getShowDynamics());
}

TEST_F(BodyInspectorTest, MassPropertiesSectionVisibility) {
    inspector_.setShowMassProperties(false);
    EXPECT_FALSE(inspector_.getShowMassProperties());

    inspector_.setShowMassProperties(true);
    EXPECT_TRUE(inspector_.getShowMassProperties());
}

TEST_F(BodyInspectorTest, ShapeSectionVisibility) {
    inspector_.setShowShape(false);
    EXPECT_FALSE(inspector_.getShowShape());

    inspector_.setShowShape(true);
    EXPECT_TRUE(inspector_.getShowShape());
}

TEST_F(BodyInspectorTest, MaterialSectionVisibility) {
    inspector_.setShowMaterial(false);
    EXPECT_FALSE(inspector_.getShowMaterial());

    inspector_.setShowMaterial(true);
    EXPECT_TRUE(inspector_.getShowMaterial());
}

TEST_F(BodyInspectorTest, FilteringSectionVisibility) {
    inspector_.setShowFiltering(false);
    EXPECT_FALSE(inspector_.getShowFiltering());

    inspector_.setShowFiltering(true);
    EXPECT_TRUE(inspector_.getShowFiltering());
}

TEST_F(BodyInspectorTest, SleepSectionVisibility) {
    inspector_.setShowSleep(false);
    EXPECT_FALSE(inspector_.getShowSleep());

    inspector_.setShowSleep(true);
    EXPECT_TRUE(inspector_.getShowSleep());
}

// === Display Options Tests ===

TEST_F(BodyInspectorTest, EulerAngleToggle) {
    EXPECT_TRUE(inspector_.getUseEulerAngles());

    inspector_.setUseEulerAngles(false);
    EXPECT_FALSE(inspector_.getUseEulerAngles());

    inspector_.setUseEulerAngles(true);
    EXPECT_TRUE(inspector_.getUseEulerAngles());
}

// === RigidBodyData Tests ===

TEST_F(BodyInspectorTest, BodyTypeEnum) {
    RigidBodyData data;

    data.type = BodyType::Static;
    EXPECT_EQ(data.type, BodyType::Static);

    data.type = BodyType::Dynamic;
    EXPECT_EQ(data.type, BodyType::Dynamic);

    data.type = BodyType::Kinematic;
    EXPECT_EQ(data.type, BodyType::Kinematic);
}

TEST_F(BodyInspectorTest, ShapeTypeEnum) {
    RigidBodyData data;

    data.shapeType = ShapeType::Sphere;
    EXPECT_EQ(data.shapeType, ShapeType::Sphere);

    data.shapeType = ShapeType::Box;
    EXPECT_EQ(data.shapeType, ShapeType::Box);

    data.shapeType = ShapeType::Capsule;
    EXPECT_EQ(data.shapeType, ShapeType::Capsule);

    data.shapeType = ShapeType::Cylinder;
    EXPECT_EQ(data.shapeType, ShapeType::Cylinder);

    data.shapeType = ShapeType::Mesh;
    EXPECT_EQ(data.shapeType, ShapeType::Mesh);

    data.shapeType = ShapeType::Convex;
    EXPECT_EQ(data.shapeType, ShapeType::Convex);
}

TEST_F(BodyInspectorTest, MaterialInfoDefaults) {
    MaterialInfo material;

    EXPECT_FLOAT_EQ(material.restitution, 0.5f);
    EXPECT_FLOAT_EQ(material.friction, 0.5f);
    EXPECT_FLOAT_EQ(material.rollingFriction, 0.0f);
    EXPECT_FLOAT_EQ(material.density, 1000.0f);
}

TEST_F(BodyInspectorTest, FilterInfoDefaults) {
    FilterInfo filter;

    EXPECT_EQ(filter.categoryBits, 0x0001u);
    EXPECT_EQ(filter.maskBits, 0xFFFFu);
    EXPECT_EQ(filter.groupIndex, 0);
}

TEST_F(BodyInspectorTest, SleepInfoDefaults) {
    SleepInfo sleep;

    EXPECT_FALSE(sleep.isSleeping);
    EXPECT_FLOAT_EQ(sleep.sleepTime, 0.0f);
    EXPECT_TRUE(sleep.allowSleep);
    EXPECT_FLOAT_EQ(sleep.linearThreshold, 0.01f);
    EXPECT_FLOAT_EQ(sleep.angularThreshold, 0.01f);
}

TEST_F(BodyInspectorTest, RigidBodyDataDefaults) {
    RigidBodyData data;

    // Identity
    EXPECT_EQ(data.id, 0u);
    EXPECT_EQ(data.type, BodyType::Dynamic);
    EXPECT_EQ(data.name, "");

    // Transform
    EXPECT_EQ(data.position, Vec3(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(data.rotation.x, 0.0f);
    EXPECT_EQ(data.rotation.y, 0.0f);
    EXPECT_EQ(data.rotation.z, 0.0f);
    EXPECT_EQ(data.rotation.w, 1.0f);

    // Dynamics
    EXPECT_EQ(data.linearVelocity, Vec3(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(data.angularVelocity, Vec3(0.0f, 0.0f, 0.0f));

    // Mass
    EXPECT_FLOAT_EQ(data.mass, 1.0f);
    EXPECT_EQ(data.inertiaTensor, Vec3(1.0f, 1.0f, 1.0f));
    EXPECT_FLOAT_EQ(data.linearDamping, 0.01f);
    EXPECT_FLOAT_EQ(data.angularDamping, 0.05f);

    // Shape
    EXPECT_EQ(data.shapeType, ShapeType::Box);
    EXPECT_EQ(data.shapeExtents, Vec3(1.0f, 1.0f, 1.0f));
}

// === Data Modification Tests ===

TEST_F(BodyInspectorTest, ModifyPosition) {
    bodyData_.position = Vec3(10.0f, 20.0f, 30.0f);

    EXPECT_FLOAT_EQ(bodyData_.position.x, 10.0f);
    EXPECT_FLOAT_EQ(bodyData_.position.y, 20.0f);
    EXPECT_FLOAT_EQ(bodyData_.position.z, 30.0f);
}

TEST_F(BodyInspectorTest, ModifyRotation) {
    bodyData_.rotation = Quat(0.1f, 0.2f, 0.3f, 0.9f);

    EXPECT_FLOAT_EQ(bodyData_.rotation.x, 0.1f);
    EXPECT_FLOAT_EQ(bodyData_.rotation.y, 0.2f);
    EXPECT_FLOAT_EQ(bodyData_.rotation.z, 0.3f);
    EXPECT_FLOAT_EQ(bodyData_.rotation.w, 0.9f);
}

TEST_F(BodyInspectorTest, ModifyVelocity) {
    bodyData_.linearVelocity = Vec3(5.0f, 10.0f, 15.0f);
    bodyData_.angularVelocity = Vec3(1.0f, 2.0f, 3.0f);

    EXPECT_EQ(bodyData_.linearVelocity, Vec3(5.0f, 10.0f, 15.0f));
    EXPECT_EQ(bodyData_.angularVelocity, Vec3(1.0f, 2.0f, 3.0f));
}

TEST_F(BodyInspectorTest, ModifyMassProperties) {
    bodyData_.mass = 50.0f;
    bodyData_.inertiaTensor = Vec3(2.0f, 3.0f, 4.0f);
    bodyData_.linearDamping = 0.1f;
    bodyData_.angularDamping = 0.2f;

    EXPECT_FLOAT_EQ(bodyData_.mass, 50.0f);
    EXPECT_EQ(bodyData_.inertiaTensor, Vec3(2.0f, 3.0f, 4.0f));
    EXPECT_FLOAT_EQ(bodyData_.linearDamping, 0.1f);
    EXPECT_FLOAT_EQ(bodyData_.angularDamping, 0.2f);
}

TEST_F(BodyInspectorTest, ModifyMaterial) {
    bodyData_.material.restitution = 0.8f;
    bodyData_.material.friction = 0.3f;
    bodyData_.material.rollingFriction = 0.05f;
    bodyData_.material.density = 2700.0f;

    EXPECT_FLOAT_EQ(bodyData_.material.restitution, 0.8f);
    EXPECT_FLOAT_EQ(bodyData_.material.friction, 0.3f);
    EXPECT_FLOAT_EQ(bodyData_.material.rollingFriction, 0.05f);
    EXPECT_FLOAT_EQ(bodyData_.material.density, 2700.0f);
}

TEST_F(BodyInspectorTest, ModifyFiltering) {
    bodyData_.filter.categoryBits = 0x0004;
    bodyData_.filter.maskBits = 0x000F;
    bodyData_.filter.groupIndex = -1;

    EXPECT_EQ(bodyData_.filter.categoryBits, 0x0004u);
    EXPECT_EQ(bodyData_.filter.maskBits, 0x000Fu);
    EXPECT_EQ(bodyData_.filter.groupIndex, -1);
}

TEST_F(BodyInspectorTest, ModifySleepState) {
    bodyData_.sleep.isSleeping = true;
    bodyData_.sleep.sleepTime = 2.5f;
    bodyData_.sleep.allowSleep = false;
    bodyData_.sleep.linearThreshold = 0.005f;
    bodyData_.sleep.angularThreshold = 0.008f;

    EXPECT_TRUE(bodyData_.sleep.isSleeping);
    EXPECT_FLOAT_EQ(bodyData_.sleep.sleepTime, 2.5f);
    EXPECT_FALSE(bodyData_.sleep.allowSleep);
    EXPECT_FLOAT_EQ(bodyData_.sleep.linearThreshold, 0.005f);
    EXPECT_FLOAT_EQ(bodyData_.sleep.angularThreshold, 0.008f);
}

// === Copy/Move Tests ===

TEST_F(BodyInspectorTest, CopyConstructor) {
    inspector_.setOpen(false);
    inspector_.setShowIdentity(false);
    inspector_.setUseEulerAngles(false);

    BodyInspector copy(inspector_);

    EXPECT_EQ(copy.isOpen(), inspector_.isOpen());
    EXPECT_EQ(copy.getShowIdentity(), inspector_.getShowIdentity());
    EXPECT_EQ(copy.getUseEulerAngles(), inspector_.getUseEulerAngles());
}

TEST_F(BodyInspectorTest, CopyAssignment) {
    inspector_.setOpen(false);
    inspector_.setShowTransform(false);

    BodyInspector other;
    other = inspector_;

    EXPECT_EQ(other.isOpen(), inspector_.isOpen());
    EXPECT_EQ(other.getShowTransform(), inspector_.getShowTransform());
}

TEST_F(BodyInspectorTest, MoveConstructor) {
    inspector_.setOpen(false);
    inspector_.setShowDynamics(false);

    bool wasOpen = inspector_.isOpen();
    bool showDynamics = inspector_.getShowDynamics();

    BodyInspector moved(std::move(inspector_));

    EXPECT_EQ(moved.isOpen(), wasOpen);
    EXPECT_EQ(moved.getShowDynamics(), showDynamics);
}

TEST_F(BodyInspectorTest, MoveAssignment) {
    inspector_.setOpen(false);
    inspector_.setShowMassProperties(false);

    bool wasOpen = inspector_.isOpen();
    bool showMass = inspector_.getShowMassProperties();

    BodyInspector other;
    other = std::move(inspector_);

    EXPECT_EQ(other.isOpen(), wasOpen);
    EXPECT_EQ(other.getShowMassProperties(), showMass);
}

// === Integration Tests ===

TEST_F(BodyInspectorTest, CompleteWorkflow) {
    // Create inspector and body data
    BodyInspector inspector;
    RigidBodyData data;

    // Configure body
    data.id = 123;
    data.type = BodyType::Dynamic;
    data.name = "Player";
    data.position = Vec3(10.0f, 5.0f, 0.0f);
    data.mass = 75.0f;
    data.shapeType = ShapeType::Capsule;
    data.shapeExtents = Vec3(0.5f, 1.8f, 0.0f);

    // Verify data was set correctly
    EXPECT_EQ(data.id, 123u);
    EXPECT_EQ(data.type, BodyType::Dynamic);
    EXPECT_EQ(data.name, "Player");
    EXPECT_EQ(data.position, Vec3(10.0f, 5.0f, 0.0f));
    EXPECT_FLOAT_EQ(data.mass, 75.0f);
    EXPECT_EQ(data.shapeType, ShapeType::Capsule);
    EXPECT_FLOAT_EQ(data.shapeExtents.x, 0.5f);
    EXPECT_FLOAT_EQ(data.shapeExtents.y, 1.8f);
}
