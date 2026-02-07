#pragma once

#include "axiom/math/quat.hpp"
#include "axiom/math/transform.hpp"
#include "axiom/math/vec3.hpp"

#include <cstdint>
#include <string>

namespace axiom::gui {

/// Type of rigid body
enum class BodyType : uint32_t {
    Static = 0,     ///< Static body (infinite mass, never moves)
    Dynamic = 1,    ///< Dynamic body (finite mass, affected by forces)
    Kinematic = 2,  ///< Kinematic body (infinite mass, velocity-driven motion)
};

/// Shape type for collision
enum class ShapeType : uint32_t {
    Sphere = 0,    ///< Sphere shape
    Box = 1,       ///< Box shape
    Capsule = 2,   ///< Capsule shape
    Cylinder = 3,  ///< Cylinder shape
    Mesh = 4,      ///< Triangle mesh shape
    Convex = 5,    ///< Convex hull shape
};

/// Material properties for physics simulation
struct MaterialInfo {
    float restitution = 0.5f;      ///< Coefficient of restitution (bounciness, 0-1)
    float friction = 0.5f;         ///< Coefficient of friction (0-1)
    float rollingFriction = 0.0f;  ///< Rolling friction coefficient
    float density = 1000.0f;       ///< Material density (kg/m^3)
};

/// Collision filtering settings
struct FilterInfo {
    uint32_t categoryBits = 0x0001;  ///< Collision category bits
    uint32_t maskBits = 0xFFFF;      ///< Collision mask bits (what this body collides with)
    int16_t groupIndex = 0;          ///< Collision group index (negative = never collide)
};

/// Sleep state information
struct SleepInfo {
    bool isSleeping = false;         ///< Whether the body is currently sleeping
    float sleepTime = 0.0f;          ///< Time the body has been below sleep threshold (seconds)
    bool allowSleep = true;          ///< Whether sleeping is enabled for this body
    float linearThreshold = 0.01f;   ///< Linear velocity threshold for sleeping
    float angularThreshold = 0.01f;  ///< Angular velocity threshold for sleeping
};

/// Complete rigid body data for inspection and editing
///
/// This structure contains all the properties of a rigid body that can be
/// displayed and edited in the BodyInspector. It serves as a data transfer
/// object between the physics engine and the GUI.
struct RigidBodyData {
    // Identity
    uint32_t id = 0;                    ///< Unique body identifier
    BodyType type = BodyType::Dynamic;  ///< Body type
    std::string name = "";              ///< Optional body name

    // Transform
    math::Vec3 position = math::Vec3(0.0f, 0.0f, 0.0f);  ///< World-space position
    math::Quat rotation = math::Quat();                  ///< World-space rotation (quaternion)

    // Dynamics
    math::Vec3 linearVelocity = math::Vec3(0.0f, 0.0f, 0.0f);   ///< Linear velocity (m/s)
    math::Vec3 angularVelocity = math::Vec3(0.0f, 0.0f, 0.0f);  ///< Angular velocity (rad/s)

    // Mass properties
    float mass = 1.0f;                                        ///< Mass (kg)
    math::Vec3 inertiaTensor = math::Vec3(1.0f, 1.0f, 1.0f);  ///< Diagonal inertia tensor

    // Damping
    float linearDamping = 0.01f;   ///< Linear velocity damping (0-1)
    float angularDamping = 0.05f;  ///< Angular velocity damping (0-1)

    // Shape
    ShapeType shapeType = ShapeType::Box;                    ///< Collision shape type
    math::Vec3 shapeExtents = math::Vec3(1.0f, 1.0f, 1.0f);  ///< Shape-specific size parameters

    // Material and filtering
    MaterialInfo material;  ///< Material properties
    FilterInfo filter;      ///< Collision filtering

    // Sleep state
    SleepInfo sleep;  ///< Sleep state information
};

/// Body inspector panel for ImGui
///
/// This class provides a detailed inspector interface for viewing and editing
/// individual rigid body properties in real-time. It displays:
/// - Identity (ID, type, name)
/// - Transform (position, rotation as quaternion or Euler angles)
/// - Dynamics (linear/angular velocity)
/// - Mass properties (mass, inertia tensor)
/// - Damping settings
/// - Shape information
/// - Material properties (restitution, friction, density)
/// - Collision filtering
/// - Sleep state
///
/// All editable properties return modification flags so the caller can
/// apply changes back to the physics simulation.
///
/// Thread safety:
/// - Must be called from the main thread (ImGui requirement)
/// - Does not own or modify physics data directly
///
/// Example usage:
/// @code
/// BodyInspector inspector;
///
/// // In main loop
/// imgui.newFrame();
///
/// // Get body data from physics world
/// RigidBodyData bodyData;
/// bodyData.id = selectedBody->getId();
/// bodyData.type = selectedBody->getType();
/// bodyData.position = selectedBody->getPosition();
/// // ... populate other fields
///
/// // Render inspector
/// if (inspector.render(bodyData)) {
///     // Body data was modified, apply changes
///     selectedBody->setPosition(bodyData.position);
///     selectedBody->setRotation(bodyData.rotation);
///     // ... apply other changes
/// }
///
/// imgui.render(commandBuffer);
/// @endcode
class BodyInspector {
public:
    /// Create a body inspector panel
    BodyInspector();

    /// Destructor
    ~BodyInspector() = default;

    // Copyable and movable
    BodyInspector(const BodyInspector&) = default;
    BodyInspector& operator=(const BodyInspector&) = default;
    BodyInspector(BodyInspector&&) = default;
    BodyInspector& operator=(BodyInspector&&) = default;

    /// Render the body inspector panel
    ///
    /// Displays all body properties in an organized, collapsible panel layout.
    /// Properties are grouped into logical sections.
    ///
    /// @param bodyData Rigid body data to display and edit
    /// @return true if any property was modified by user interaction, false otherwise
    bool render(RigidBodyData& bodyData);

    /// Render the body inspector panel with custom window title
    ///
    /// @param bodyData Rigid body data to display and edit
    /// @param windowTitle Custom window title
    /// @return true if any property was modified, false otherwise
    bool render(RigidBodyData& bodyData, const char* windowTitle);

    // === Window state ===

    /// Set whether the inspector window is open
    /// @param open true to show window, false to hide
    void setOpen(bool open) noexcept { isOpen_ = open; }

    /// Get whether the inspector window is open
    /// @return true if window is visible
    bool isOpen() const noexcept { return isOpen_; }

    /// Toggle the inspector window open/closed state
    void toggleOpen() noexcept { isOpen_ = !isOpen_; }

    // === Section visibility ===

    /// Set whether to show the identity section
    /// @param show true to show, false to hide
    void setShowIdentity(bool show) noexcept { showIdentity_ = show; }

    /// Get whether the identity section is shown
    /// @return true if visible
    bool getShowIdentity() const noexcept { return showIdentity_; }

    /// Set whether to show the transform section
    /// @param show true to show, false to hide
    void setShowTransform(bool show) noexcept { showTransform_ = show; }

    /// Get whether the transform section is shown
    /// @return true if visible
    bool getShowTransform() const noexcept { return showTransform_; }

    /// Set whether to show the dynamics section
    /// @param show true to show, false to hide
    void setShowDynamics(bool show) noexcept { showDynamics_ = show; }

    /// Get whether the dynamics section is shown
    /// @return true if visible
    bool getShowDynamics() const noexcept { return showDynamics_; }

    /// Set whether to show the mass properties section
    /// @param show true to show, false to hide
    void setShowMassProperties(bool show) noexcept { showMassProperties_ = show; }

    /// Get whether the mass properties section is shown
    /// @return true if visible
    bool getShowMassProperties() const noexcept { return showMassProperties_; }

    /// Set whether to show the shape section
    /// @param show true to show, false to hide
    void setShowShape(bool show) noexcept { showShape_ = show; }

    /// Get whether the shape section is shown
    /// @return true if visible
    bool getShowShape() const noexcept { return showShape_; }

    /// Set whether to show the material section
    /// @param show true to show, false to hide
    void setShowMaterial(bool show) noexcept { showMaterial_ = show; }

    /// Get whether the material section is shown
    /// @return true if visible
    bool getShowMaterial() const noexcept { return showMaterial_; }

    /// Set whether to show the filtering section
    /// @param show true to show, false to hide
    void setShowFiltering(bool show) noexcept { showFiltering_ = show; }

    /// Get whether the filtering section is shown
    /// @return true if visible
    bool getShowFiltering() const noexcept { return showFiltering_; }

    /// Set whether to show the sleep section
    /// @param show true to show, false to hide
    void setShowSleep(bool show) noexcept { showSleep_ = show; }

    /// Get whether the sleep section is shown
    /// @return true if visible
    bool getShowSleep() const noexcept { return showSleep_; }

    // === Display options ===

    /// Set whether to use Euler angles for rotation display (true) or quaternion (false)
    /// @param useEuler true for Euler angles (degrees), false for quaternion components
    void setUseEulerAngles(bool useEuler) noexcept { useEulerAngles_ = useEuler; }

    /// Get whether Euler angles are used for rotation display
    /// @return true if using Euler angles, false if using quaternion
    bool getUseEulerAngles() const noexcept { return useEulerAngles_; }

private:
    /// Render the identity section (ID, type, name)
    /// @param bodyData Body data (mutable)
    /// @return true if modified
    bool renderIdentitySection(RigidBodyData& bodyData);

    /// Render the transform section (position, rotation)
    /// @param bodyData Body data (mutable)
    /// @return true if modified
    bool renderTransformSection(RigidBodyData& bodyData);

    /// Render the dynamics section (velocities)
    /// @param bodyData Body data (mutable)
    /// @return true if modified
    bool renderDynamicsSection(RigidBodyData& bodyData);

    /// Render the mass properties section (mass, inertia, damping)
    /// @param bodyData Body data (mutable)
    /// @return true if modified
    bool renderMassPropertiesSection(RigidBodyData& bodyData);

    /// Render the shape section
    /// @param bodyData Body data (mutable)
    /// @return true if modified
    bool renderShapeSection(RigidBodyData& bodyData);

    /// Render the material section
    /// @param bodyData Body data (mutable)
    /// @return true if modified
    bool renderMaterialSection(RigidBodyData& bodyData);

    /// Render the filtering section
    /// @param bodyData Body data (mutable)
    /// @return true if modified
    bool renderFilteringSection(RigidBodyData& bodyData);

    /// Render the sleep section
    /// @param bodyData Body data (mutable)
    /// @return true if modified
    bool renderSleepSection(RigidBodyData& bodyData);

    // Window state
    bool isOpen_ = true;  ///< Whether the inspector window is visible

    // Section visibility
    bool showIdentity_ = true;        ///< Show identity section
    bool showTransform_ = true;       ///< Show transform section
    bool showDynamics_ = true;        ///< Show dynamics section
    bool showMassProperties_ = true;  ///< Show mass properties section
    bool showShape_ = true;           ///< Show shape section
    bool showMaterial_ = true;        ///< Show material section
    bool showFiltering_ = true;       ///< Show filtering section
    bool showSleep_ = true;           ///< Show sleep section

    // Display options
    bool useEulerAngles_ = true;  ///< Use Euler angles (true) or quaternion (false) for rotation
};

}  // namespace axiom::gui
