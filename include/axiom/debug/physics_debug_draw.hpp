#pragma once

#include "axiom/debug/debug_draw.hpp"
#include "axiom/math/aabb.hpp"
#include "axiom/math/transform.hpp"
#include "axiom/math/vec3.hpp"
#include "axiom/math/vec4.hpp"

#include <cstdint>

namespace axiom {

// Forward declarations for future physics types
namespace collision {
struct Shape;
struct ContactPoint;
}  // namespace collision

namespace dynamics {
class RigidBody;
class Constraint;
}  // namespace dynamics

namespace debug {

/// Flags controlling which physics debug visualizations are drawn
enum class PhysicsDebugFlags : uint32_t {
    None = 0,                    ///< No debug drawing
    Shapes = 1 << 0,             ///< Draw collision shape outlines
    AABBs = 1 << 1,              ///< Draw axis-aligned bounding boxes
    Contacts = 1 << 2,           ///< Draw contact points and normals
    Constraints = 1 << 3,        ///< Draw constraint/joint connections
    Velocities = 1 << 4,         ///< Draw linear velocity vectors
    AngularVelocities = 1 << 5,  ///< Draw angular velocity indicators
    Forces = 1 << 6,             ///< Draw force vectors
    Islands = 1 << 7,            ///< Color-code simulation islands
    CenterOfMass = 1 << 8,       ///< Draw center of mass markers
    LocalAxes = 1 << 9,          ///< Draw local coordinate frames
    All = 0xFFFFFFFF             ///< Enable all debug visualizations
};

/// Bitwise OR operator for PhysicsDebugFlags
constexpr PhysicsDebugFlags operator|(PhysicsDebugFlags a, PhysicsDebugFlags b) noexcept {
    return static_cast<PhysicsDebugFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/// Bitwise AND operator for PhysicsDebugFlags
constexpr PhysicsDebugFlags operator&(PhysicsDebugFlags a, PhysicsDebugFlags b) noexcept {
    return static_cast<PhysicsDebugFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/// Bitwise OR assignment operator for PhysicsDebugFlags
constexpr PhysicsDebugFlags& operator|=(PhysicsDebugFlags& a, PhysicsDebugFlags b) noexcept {
    a = a | b;
    return a;
}

/// Bitwise AND assignment operator for PhysicsDebugFlags
constexpr PhysicsDebugFlags& operator&=(PhysicsDebugFlags& a, PhysicsDebugFlags b) noexcept {
    a = a & b;
    return a;
}

/// Check if a flag is set
constexpr bool hasFlag(PhysicsDebugFlags flags, PhysicsDebugFlags flag) noexcept {
    return (flags & flag) != PhysicsDebugFlags::None;
}

/// Configuration for physics debug visualization
struct PhysicsDebugDrawConfig {
    PhysicsDebugFlags flags = PhysicsDebugFlags::Shapes | PhysicsDebugFlags::Contacts;
    bool depthTestEnabled = true;       ///< Enable depth testing for debug primitives
    float contactNormalLength = 0.3f;   ///< Length of contact normal arrows
    float velocityScale = 0.1f;         ///< Scale factor for velocity vectors
    float forceScale = 0.001f;          ///< Scale factor for force vectors
    float angularVelocityScale = 0.5f;  ///< Scale factor for angular velocity visualization
    math::Vec4 shapeColor = math::Vec4(0.0f, 1.0f, 0.0f, 1.0f);    ///< Default shape color (green)
    math::Vec4 aabbColor = math::Vec4(1.0f, 1.0f, 0.0f, 1.0f);     ///< AABB color (yellow)
    math::Vec4 contactColor = math::Vec4(1.0f, 0.0f, 0.0f, 1.0f);  ///< Contact point color (red)
    math::Vec4 velocityColor = math::Vec4(0.0f, 0.5f, 1.0f,
                                          1.0f);                 ///< Velocity vector color (blue)
    math::Vec4 forceColor = math::Vec4(1.0f, 0.5f, 0.0f, 1.0f);  ///< Force vector color (orange)
};

/// Shape types for debug visualization
/// These match the collision system's shape types (to be implemented in Phase 3-4)
enum class ShapeType {
    Sphere,   ///< Sphere shape
    Box,      ///< Oriented box shape
    Capsule,  ///< Capsule shape (cylinder with hemispherical caps)
    Plane,    ///< Infinite plane
    Convex,   ///< Convex hull
    Mesh      ///< Triangle mesh (concave)
};

/// Simplified shape data for debug drawing (used until full collision system is implemented)
struct DebugShape {
    ShapeType type;
    math::Transform transform;

    // Shape-specific parameters (union-like usage)
    float radius = 0.0f;     ///< Sphere/Capsule radius
    math::Vec3 halfExtents;  ///< Box half-extents
    float height = 0.0f;     ///< Capsule height
    math::Vec3 normal;       ///< Plane normal

    // For convex/mesh (future use)
    const float* vertices = nullptr;    ///< Pointer to vertex data
    size_t vertexCount = 0;             ///< Number of vertices
    const uint32_t* indices = nullptr;  ///< Pointer to index data
    size_t indexCount = 0;              ///< Number of indices
};

/// Simplified rigid body data for debug drawing (used until full dynamics system is implemented)
struct DebugRigidBody {
    math::Transform transform;   ///< Body transform
    math::Vec3 linearVelocity;   ///< Linear velocity
    math::Vec3 angularVelocity;  ///< Angular velocity
    math::Vec3 force;            ///< Total force
    math::Vec3 centerOfMass;     ///< Center of mass (local space)
    math::AABB aabb;             ///< World-space AABB
    DebugShape shape;            ///< Collision shape
    bool isAwake = true;         ///< Sleep state
    uint32_t islandIndex = 0;    ///< Simulation island index (for color coding)
};

/// Simplified contact point data for debug drawing
struct DebugContactPoint {
    math::Vec3 position;            ///< Contact point position (world space)
    math::Vec3 normal;              ///< Contact normal (world space)
    float penetrationDepth = 0.0f;  ///< Penetration depth
};

/// Simplified constraint data for debug drawing
struct DebugConstraint {
    math::Vec3 anchorA;  ///< Anchor point on body A (world space)
    math::Vec3 anchorB;  ///< Anchor point on body B (world space)
    math::Vec4 color = math::Vec4(0.8f, 0.2f, 0.8f, 1.0f);  ///< Constraint color (purple)
};

/// Physics debug visualization system
/// This class wraps DebugDraw to provide higher-level physics-specific visualization.
/// It can draw rigid bodies, collision shapes, contact points, constraints, AABBs,
/// velocity vectors, force vectors, and more.
///
/// Example usage:
/// @code
/// PhysicsDebugDraw physicsDebug(&debugDraw);
/// physicsDebug.setFlags(PhysicsDebugFlags::Shapes | PhysicsDebugFlags::Velocities);
///
/// // Draw a rigid body
/// DebugRigidBody body;
/// body.shape.type = ShapeType::Box;
/// body.shape.halfExtents = Vec3(1.0f, 1.0f, 1.0f);
/// body.transform = Transform(Vec3(0, 5, 0), Quat::identity());
/// physicsDebug.drawRigidBody(body);
///
/// // Draw contact points
/// DebugContactPoint contact;
/// contact.position = Vec3(0, 0, 0);
/// contact.normal = Vec3(0, 1, 0);
/// contact.penetrationDepth = 0.1f;
/// physicsDebug.drawContactPoint(contact);
/// @endcode
class PhysicsDebugDraw {
public:
    /// Create a physics debug draw system
    /// @param debugDraw Underlying debug draw system (must outlive this object)
    /// @param config Configuration settings
    explicit PhysicsDebugDraw(DebugDraw* debugDraw, const PhysicsDebugDrawConfig& config = {});

    /// Destructor
    ~PhysicsDebugDraw() = default;

    // Non-copyable
    PhysicsDebugDraw(const PhysicsDebugDraw&) = delete;
    PhysicsDebugDraw& operator=(const PhysicsDebugDraw&) = delete;

    // Non-movable (simplified ownership)
    PhysicsDebugDraw(PhysicsDebugDraw&&) = delete;
    PhysicsDebugDraw& operator=(PhysicsDebugDraw&&) = delete;

    // === Drawing API ===

    /// Draw a rigid body with all enabled visualizations
    /// @param body Rigid body data
    void drawRigidBody(const DebugRigidBody& body);

    /// Draw a collision shape
    /// @param shape Shape to draw
    /// @param color Optional color override
    void drawCollisionShape(const DebugShape& shape,
                            const math::Vec4& color = math::Vec4(0, 1, 0, 1));

    /// Draw a contact point with normal
    /// @param contact Contact point data
    void drawContactPoint(const DebugContactPoint& contact);

    /// Draw a constraint/joint connection
    /// @param constraint Constraint data
    void drawConstraint(const DebugConstraint& constraint);

    /// Draw an axis-aligned bounding box
    /// @param aabb Bounding box to draw
    /// @param color Optional color override
    void drawAABB(const math::AABB& aabb, const math::Vec4& color = math::Vec4(1, 1, 0, 1));

    /// Draw a velocity vector from a position
    /// @param position Start position (world space)
    /// @param velocity Velocity vector
    void drawVelocity(const math::Vec3& position, const math::Vec3& velocity);

    /// Draw a force vector from a position
    /// @param position Application point (world space)
    /// @param force Force vector
    void drawForce(const math::Vec3& position, const math::Vec3& force);

    /// Draw an angular velocity indicator
    /// @param position Center position (world space)
    /// @param angularVelocity Angular velocity vector (axis = direction, magnitude = speed)
    void drawAngularVelocity(const math::Vec3& position, const math::Vec3& angularVelocity);

    /// Draw a center of mass marker
    /// @param position Center of mass position (world space)
    void drawCenterOfMass(const math::Vec3& position);

    // === Configuration ===

    /// Set debug visualization flags
    /// @param flags Flags controlling which visualizations are drawn
    void setFlags(PhysicsDebugFlags flags) noexcept { config_.flags = flags; }

    /// Get current debug visualization flags
    /// @return Current flags
    PhysicsDebugFlags getFlags() const noexcept { return config_.flags; }

    /// Enable or disable depth testing
    /// @param enabled true to enable depth testing
    void setDepthTestEnabled(bool enabled) noexcept;

    /// Get current depth test state
    /// @return true if depth testing is enabled
    bool getDepthTestEnabled() const noexcept { return config_.depthTestEnabled; }

    /// Set configuration
    /// @param config New configuration
    void setConfig(const PhysicsDebugDrawConfig& config) noexcept { config_ = config; }

    /// Get current configuration
    /// @return Current configuration
    const PhysicsDebugDrawConfig& getConfig() const noexcept { return config_; }

private:
    /// Draw a sphere shape
    void drawSphere(const DebugShape& shape, const math::Vec4& color);

    /// Draw a box shape
    void drawBox(const DebugShape& shape, const math::Vec4& color);

    /// Draw a capsule shape
    void drawCapsule(const DebugShape& shape, const math::Vec4& color);

    /// Draw a plane shape
    void drawPlane(const DebugShape& shape, const math::Vec4& color);

    /// Draw a convex hull shape
    void drawConvexHull(const DebugShape& shape, const math::Vec4& color);

    /// Get a color for a simulation island (deterministic color based on index)
    math::Vec4 getIslandColor(uint32_t islandIndex) const noexcept;

    DebugDraw* debugDraw_;           ///< Underlying debug draw system (not owned)
    PhysicsDebugDrawConfig config_;  ///< Configuration settings
};

}  // namespace debug
}  // namespace axiom
