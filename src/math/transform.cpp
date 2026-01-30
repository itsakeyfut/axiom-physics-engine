#include "axiom/math/transform.hpp"

#include "axiom/math/mat4.hpp"
#include "axiom/math/quat.hpp"
#include "axiom/math/vec3.hpp"

#include <cmath>

namespace axiom::math {

// Conversion methods

Mat4 Transform::toMatrix() const noexcept {
    // TRS order: M = T * R * S
    // First create scale matrix
    Mat4 scaleMat = Mat4::scaling(scale);
    // Then rotation matrix
    Mat4 rotMat = rotation.toMatrix();
    // Then translation matrix
    Mat4 transMat = Mat4::translation(position);

    // Combine: T * R * S
    return transMat * (rotMat * scaleMat);
}

Transform Transform::fromMatrix(const Mat4& m) noexcept {
    // Extract translation (last column)
    Vec3 pos(m.at(0, 3), m.at(1, 3), m.at(2, 3));

    // Extract scale from the length of each basis vector
    Vec3 xAxis(m.at(0, 0), m.at(1, 0), m.at(2, 0));
    Vec3 yAxis(m.at(0, 1), m.at(1, 1), m.at(2, 1));
    Vec3 zAxis(m.at(0, 2), m.at(1, 2), m.at(2, 2));

    float scaleX = xAxis.length();
    float scaleY = yAxis.length();
    float scaleZ = zAxis.length();
    Vec3 scl(scaleX, scaleY, scaleZ);

    // Create rotation matrix by normalizing the basis vectors
    Mat4 rotMat = m;
    if (scaleX > 0.0f) {
        rotMat.at(0, 0) /= scaleX;
        rotMat.at(1, 0) /= scaleX;
        rotMat.at(2, 0) /= scaleX;
    }
    if (scaleY > 0.0f) {
        rotMat.at(0, 1) /= scaleY;
        rotMat.at(1, 1) /= scaleY;
        rotMat.at(2, 1) /= scaleY;
    }
    if (scaleZ > 0.0f) {
        rotMat.at(0, 2) /= scaleZ;
        rotMat.at(1, 2) /= scaleZ;
        rotMat.at(2, 2) /= scaleZ;
    }

    // Extract rotation quaternion from rotation matrix
    Quat rot = Quat::fromMatrix(rotMat);

    return Transform(pos, rot, scl);
}

// Inverse transform

Transform Transform::inverse() const noexcept {
    // For a transform T = TRS, the inverse is:
    // T^-1 = S^-1 * R^-1 * T^-1

    // Inverse scale
    Vec3 invScale(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);

    // Inverse rotation
    Quat invRotation = rotation.conjugate();  // For unit quaternions, conjugate == inverse

    // Inverse translation: -R^-1 * S^-1 * T
    Vec3 scaledPos = Vec3(position.x * invScale.x, position.y * invScale.y,
                          position.z * invScale.z);
    Vec3 invPosition = -(invRotation * scaledPos);

    return Transform(invPosition, invRotation, invScale);
}

// Point and vector transformation

Vec3 Transform::transformPoint(const Vec3& point) const noexcept {
    // Apply scale
    Vec3 scaled(point.x * scale.x, point.y * scale.y, point.z * scale.z);
    // Apply rotation
    Vec3 rotated = rotation * scaled;
    // Apply translation
    return rotated + position;
}

Vec3 Transform::transformDirection(const Vec3& direction) const noexcept {
    // Apply scale
    Vec3 scaled(direction.x * scale.x, direction.y * scale.y, direction.z * scale.z);
    // Apply rotation (no translation for directions)
    return rotation * scaled;
}

Vec3 Transform::transformNormal(const Vec3& normal) const noexcept {
    // Normals require inverse transpose of the transformation
    // For non-uniform scaling, use inverse scale
    Vec3 invScale(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
    Vec3 scaled(normal.x * invScale.x, normal.y * invScale.y, normal.z * invScale.z);
    // Apply rotation
    Vec3 rotated = rotation * scaled;
    // Normalize the result
    return rotated.normalized();
}

Vec3 Transform::inverseTransformPoint(const Vec3& point) const noexcept {
    // Remove translation
    Vec3 translated = point - position;
    // Apply inverse rotation
    Quat invRotation = rotation.conjugate();
    Vec3 rotated = invRotation * translated;
    // Apply inverse scale
    Vec3 invScale(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
    return Vec3(rotated.x * invScale.x, rotated.y * invScale.y, rotated.z * invScale.z);
}

Vec3 Transform::inverseTransformDirection(const Vec3& direction) const noexcept {
    // Apply inverse rotation
    Quat invRotation = rotation.conjugate();
    Vec3 rotated = invRotation * direction;
    // Apply inverse scale
    Vec3 invScale(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
    return Vec3(rotated.x * invScale.x, rotated.y * invScale.y, rotated.z * invScale.z);
}

Vec3 Transform::inverseTransformNormal(const Vec3& normal) const noexcept {
    // Apply inverse rotation
    Quat invRotation = rotation.conjugate();
    Vec3 rotated = invRotation * normal;
    // Apply scale (not inverse for normals in inverse transform)
    Vec3 scaled(rotated.x * scale.x, rotated.y * scale.y, rotated.z * scale.z);
    // Normalize the result
    return scaled.normalized();
}

// Transform composition

Transform Transform::operator*(const Transform& other) const noexcept {
    // Combine two transforms: parent * child
    // Result applies child transform first, then parent

    // Combined rotation
    Quat combinedRotation = rotation * other.rotation;

    // Combined scale
    Vec3 combinedScale(scale.x * other.scale.x, scale.y * other.scale.y, scale.z * other.scale.z);

    // Combined position: parent.pos + parent.rot * (parent.scale * child.pos)
    Vec3 scaledChildPos(other.position.x * scale.x, other.position.y * scale.y,
                        other.position.z * scale.z);
    Vec3 rotatedChildPos = rotation * scaledChildPos;
    Vec3 combinedPosition = position + rotatedChildPos;

    return Transform(combinedPosition, combinedRotation, combinedScale);
}

}  // namespace axiom::math
