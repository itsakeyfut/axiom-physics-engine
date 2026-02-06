#include "axiom/debug/physics_debug_draw.hpp"

#include "axiom/math/constants.hpp"
#include "axiom/math/quat.hpp"

#include <cmath>

namespace axiom::debug {

PhysicsDebugDraw::PhysicsDebugDraw(DebugDraw* debugDraw, const PhysicsDebugDrawConfig& config)
    : debugDraw_(debugDraw), config_(config) {}

void PhysicsDebugDraw::drawRigidBody(const DebugRigidBody& body) {
    // Determine color based on island index if islands flag is set
    math::Vec4 shapeColor = config_.shapeColor;
    if (hasFlag(config_.flags, PhysicsDebugFlags::Islands)) {
        shapeColor = getIslandColor(body.islandIndex);
    }

    // Darken color if body is asleep
    if (!body.isAwake) {
        shapeColor = shapeColor * 0.5f;
    }

    // Draw collision shape
    if (hasFlag(config_.flags, PhysicsDebugFlags::Shapes)) {
        drawCollisionShape(body.shape, shapeColor);
    }

    // Draw AABB
    if (hasFlag(config_.flags, PhysicsDebugFlags::AABBs)) {
        drawAABB(body.aabb, config_.aabbColor);
    }

    // Draw local axes
    if (hasFlag(config_.flags, PhysicsDebugFlags::LocalAxes)) {
        debugDraw_->drawAxis(body.transform, 1.0f);
    }

    // Draw center of mass
    if (hasFlag(config_.flags, PhysicsDebugFlags::CenterOfMass)) {
        math::Vec3 comWorld = body.transform.transformPoint(body.centerOfMass);
        drawCenterOfMass(comWorld);
    }

    // Draw linear velocity
    if (hasFlag(config_.flags, PhysicsDebugFlags::Velocities)) {
        math::Vec3 comWorld = body.transform.transformPoint(body.centerOfMass);
        drawVelocity(comWorld, body.linearVelocity);
    }

    // Draw angular velocity
    if (hasFlag(config_.flags, PhysicsDebugFlags::AngularVelocities)) {
        math::Vec3 comWorld = body.transform.transformPoint(body.centerOfMass);
        drawAngularVelocity(comWorld, body.angularVelocity);
    }

    // Draw force
    if (hasFlag(config_.flags, PhysicsDebugFlags::Forces)) {
        math::Vec3 comWorld = body.transform.transformPoint(body.centerOfMass);
        drawForce(comWorld, body.force);
    }
}

void PhysicsDebugDraw::drawCollisionShape(const DebugShape& shape, const math::Vec4& color) {
    switch (shape.type) {
    case ShapeType::Sphere:
        drawSphere(shape, color);
        break;
    case ShapeType::Box:
        drawBox(shape, color);
        break;
    case ShapeType::Capsule:
        drawCapsule(shape, color);
        break;
    case ShapeType::Plane:
        drawPlane(shape, color);
        break;
    case ShapeType::Convex:
        drawConvexHull(shape, color);
        break;
    case ShapeType::Mesh:
        // Mesh rendering not yet implemented (would need triangle edges)
        break;
    }
}

void PhysicsDebugDraw::drawContactPoint(const DebugContactPoint& contact) {
    if (!hasFlag(config_.flags, PhysicsDebugFlags::Contacts)) {
        return;
    }

    // Draw contact point as a small cross
    const float crossSize = 0.05f;
    math::Vec3 offset1(crossSize, 0, 0);
    math::Vec3 offset2(0, crossSize, 0);
    math::Vec3 offset3(0, 0, crossSize);

    debugDraw_->drawLine(contact.position - offset1, contact.position + offset1,
                         config_.contactColor);
    debugDraw_->drawLine(contact.position - offset2, contact.position + offset2,
                         config_.contactColor);
    debugDraw_->drawLine(contact.position - offset3, contact.position + offset3,
                         config_.contactColor);

    // Draw contact normal as an arrow
    math::Vec3 normalEnd = contact.position + contact.normal * config_.contactNormalLength;
    debugDraw_->drawArrow(contact.position, normalEnd, config_.contactColor, 0.2f);

    // Draw penetration depth as text/marker (if significant)
    if (contact.penetrationDepth > 0.001f) {
        math::Vec3 depthEnd = contact.position - contact.normal * contact.penetrationDepth;
        debugDraw_->drawLine(contact.position, depthEnd,
                             math::Vec4(1.0f, 0.5f, 0.0f, 1.0f));  // Orange for penetration
    }
}

void PhysicsDebugDraw::drawConstraint(const DebugConstraint& constraint) {
    if (!hasFlag(config_.flags, PhysicsDebugFlags::Constraints)) {
        return;
    }

    // Draw line between anchor points
    debugDraw_->drawLine(constraint.anchorA, constraint.anchorB, constraint.color);

    // Draw small markers at anchor points
    const float markerSize = 0.05f;
    debugDraw_->drawSphere(constraint.anchorA, markerSize, constraint.color, 4);
    debugDraw_->drawSphere(constraint.anchorB, markerSize, constraint.color, 4);
}

void PhysicsDebugDraw::drawAABB(const math::AABB& aabb, const math::Vec4& color) {
    if (!hasFlag(config_.flags, PhysicsDebugFlags::AABBs)) {
        return;
    }

    debugDraw_->drawBox(aabb.min, aabb.max, color);
}

void PhysicsDebugDraw::drawVelocity(const math::Vec3& position, const math::Vec3& velocity) {
    if (!hasFlag(config_.flags, PhysicsDebugFlags::Velocities)) {
        return;
    }

    // Skip if velocity is negligible
    float velocityMagnitude = velocity.length();
    if (velocityMagnitude < 0.001f) {
        return;
    }

    math::Vec3 end = position + velocity * config_.velocityScale;
    debugDraw_->drawArrow(position, end, config_.velocityColor, 0.15f);
}

void PhysicsDebugDraw::drawForce(const math::Vec3& position, const math::Vec3& force) {
    if (!hasFlag(config_.flags, PhysicsDebugFlags::Forces)) {
        return;
    }

    // Skip if force is negligible
    float forceMagnitude = force.length();
    if (forceMagnitude < 0.001f) {
        return;
    }

    math::Vec3 end = position + force * config_.forceScale;
    debugDraw_->drawArrow(position, end, config_.forceColor, 0.15f);
}

void PhysicsDebugDraw::drawAngularVelocity(const math::Vec3& position,
                                           const math::Vec3& angularVelocity) {
    if (!hasFlag(config_.flags, PhysicsDebugFlags::AngularVelocities)) {
        return;
    }

    // Skip if angular velocity is negligible
    float angularSpeed = angularVelocity.length();
    if (angularSpeed < 0.001f) {
        return;
    }

    // Draw angular velocity as a circular arc with arrow
    // The axis of rotation is the direction of angularVelocity
    // The magnitude is the rotation speed in radians/second

    math::Vec3 axis = angularVelocity.normalized();
    float radius = config_.angularVelocityScale;

    // Create a perpendicular vector for the circle
    math::Vec3 perpendicular;
    if (std::abs(axis.x) < 0.9f) {
        perpendicular = math::Vec3(1, 0, 0).cross(axis).normalized();
    } else {
        perpendicular = math::Vec3(0, 1, 0).cross(axis).normalized();
    }

    // Draw a circular arc around the axis
    const int segments = 16;
    const float arcAngle = math::PI_F;  // 180 degree arc

    for (int i = 0; i < segments; ++i) {
        float angle1 = (static_cast<float>(i) / segments) * arcAngle;
        float angle2 = (static_cast<float>(i + 1) / segments) * arcAngle;

        // Rotate perpendicular vector around axis
        math::Quat rot1 = math::Quat::fromAxisAngle(axis, angle1);
        math::Quat rot2 = math::Quat::fromAxisAngle(axis, angle2);

        math::Vec3 p1 = position + (rot1 * perpendicular) * radius;
        math::Vec3 p2 = position + (rot2 * perpendicular) * radius;

        debugDraw_->drawLine(p1, p2, config_.velocityColor);
    }

    // Draw arrow head at the end of the arc
    math::Quat endRot = math::Quat::fromAxisAngle(axis, arcAngle);
    math::Vec3 arcEnd = position + (endRot * perpendicular) * radius;
    math::Vec3 tangent = axis.cross(endRot * perpendicular).normalized();
    debugDraw_->drawCone(arcEnd + tangent * 0.05f, arcEnd, 0.05f, config_.velocityColor, 4);
}

void PhysicsDebugDraw::drawCenterOfMass(const math::Vec3& position) {
    if (!hasFlag(config_.flags, PhysicsDebugFlags::CenterOfMass)) {
        return;
    }

    // Draw small cross at center of mass
    const float size = 0.1f;
    math::Vec4 color(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow

    debugDraw_->drawLine(position - math::Vec3(size, 0, 0), position + math::Vec3(size, 0, 0),
                         color);
    debugDraw_->drawLine(position - math::Vec3(0, size, 0), position + math::Vec3(0, size, 0),
                         color);
    debugDraw_->drawLine(position - math::Vec3(0, 0, size), position + math::Vec3(0, 0, size),
                         color);
}

void PhysicsDebugDraw::setDepthTestEnabled(bool enabled) noexcept {
    config_.depthTestEnabled = enabled;
    debugDraw_->setDepthTestEnabled(enabled);
}

// === Private shape drawing methods ===

void PhysicsDebugDraw::drawSphere(const DebugShape& shape, const math::Vec4& color) {
    debugDraw_->drawSphere(shape.transform.position, shape.radius, color);
}

void PhysicsDebugDraw::drawBox(const DebugShape& shape, const math::Vec4& color) {
    debugDraw_->drawBox(shape.transform, shape.halfExtents, color);
}

void PhysicsDebugDraw::drawCapsule(const DebugShape& shape, const math::Vec4& color) {
    // Calculate capsule endpoints from transform and height
    math::Vec3 halfHeight(0, shape.height * 0.5f,
                          0);  // Capsule oriented along Y-axis in local space
    math::Vec3 localStart = -halfHeight;
    math::Vec3 localEnd = halfHeight;

    // Transform to world space
    math::Vec3 start = shape.transform.transformPoint(localStart);
    math::Vec3 end = shape.transform.transformPoint(localEnd);

    debugDraw_->drawCapsule(start, end, shape.radius, color);
}

void PhysicsDebugDraw::drawPlane(const DebugShape& shape, const math::Vec4& color) {
    // Draw a finite representation of the infinite plane
    const float planeSize = 10.0f;
    math::Vec3 center = shape.transform.position;
    math::Vec3 normal = shape.transform.transformDirection(shape.normal).normalized();

    debugDraw_->drawPlane(center, normal, planeSize, color);
}

void PhysicsDebugDraw::drawConvexHull(const DebugShape& shape, const math::Vec4& color) {
    if (shape.vertices == nullptr || shape.indices == nullptr) {
        return;
    }

    // Convert raw vertex data to Vec3 array
    std::vector<math::Vec3> vertices;
    vertices.reserve(shape.vertexCount);
    for (size_t i = 0; i < shape.vertexCount; ++i) {
        vertices.emplace_back(shape.vertices[i * 3 + 0], shape.vertices[i * 3 + 1],
                              shape.vertices[i * 3 + 2]);
    }

    // Convert raw index data to vector
    std::vector<uint32_t> indices(shape.indices, shape.indices + shape.indexCount);

    debugDraw_->drawConvexHull(vertices, indices, shape.transform, color);
}

math::Vec4 PhysicsDebugDraw::getIslandColor(uint32_t islandIndex) const noexcept {
    // Generate a deterministic color based on island index using a simple hash
    // This ensures the same island always gets the same color across frames
    const uint32_t hash = islandIndex * 2654435761u;  // Golden ratio prime

    // Extract RGB components from hash
    float r = static_cast<float>((hash >> 0) & 0xFF) / 255.0f;
    float g = static_cast<float>((hash >> 8) & 0xFF) / 255.0f;
    float b = static_cast<float>((hash >> 16) & 0xFF) / 255.0f;

    // Ensure minimum brightness for visibility
    const float minBrightness = 0.3f;
    r = minBrightness + r * (1.0f - minBrightness);
    g = minBrightness + g * (1.0f - minBrightness);
    b = minBrightness + b * (1.0f - minBrightness);

    return math::Vec4(r, g, b, 1.0f);
}

}  // namespace axiom::debug
