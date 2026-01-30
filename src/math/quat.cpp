#include "axiom/math/quat.hpp"

#include "axiom/math/mat4.hpp"
#include "axiom/math/vec3.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <cmath>

namespace axiom::math {

// Helper function to convert Quat to glm::quat
inline glm::quat toGLM(const Quat& q) noexcept {
    return glm::quat(q.w, q.x, q.y, q.z);  // GLM uses (w, x, y, z) order
}

// Helper function to convert glm::quat to Quat
inline Quat fromGLM(const glm::quat& q) noexcept {
    return Quat(q.x, q.y, q.z, q.w);  // Our format is (x, y, z, w)
}

// Quaternion operations

Quat Quat::operator*(const Quat& other) const noexcept {
    glm::quat a = toGLM(*this);
    glm::quat b = toGLM(other);
    return fromGLM(a * b);
}

Vec3 Quat::operator*(const Vec3& v) const noexcept {
    glm::quat q = toGLM(*this);
    glm::vec3 vec(v.x, v.y, v.z);
    glm::vec3 result = q * vec;
    return Vec3(result.x, result.y, result.z);
}

Quat Quat::inverse() const noexcept {
    glm::quat q = toGLM(*this);
    return fromGLM(glm::inverse(q));
}

float Quat::length() const noexcept {
    return std::sqrt(lengthSquared());
}

Quat Quat::normalized() const noexcept {
    glm::quat q = toGLM(*this);
    return fromGLM(glm::normalize(q));
}

Quat& Quat::normalize() noexcept {
    float len = length();
    if (len > 0.0f) {
        float invLen = 1.0f / len;
        x *= invLen;
        y *= invLen;
        z *= invLen;
        w *= invLen;
    }
    return *this;
}

// Static factory methods

Quat Quat::fromAxisAngle(const Vec3& axis, float angleRadians) noexcept {
    glm::vec3 axisGLM(axis.x, axis.y, axis.z);
    glm::quat q = glm::angleAxis(angleRadians, axisGLM);
    return fromGLM(q);
}

Quat Quat::fromEuler(float pitch, float yaw, float roll) noexcept {
    // GLM uses intrinsic rotations in the order: roll (Z), yaw (Y), pitch (X)
    glm::vec3 eulerAngles(pitch, yaw, roll);
    glm::quat q(eulerAngles);
    return fromGLM(q);
}

Quat Quat::fromMatrix(const Mat4& m) noexcept {
    // Extract the 3x3 rotation part from the 4x4 matrix
    glm::mat3 rotMat;
    rotMat[0][0] = m.at(0, 0);
    rotMat[0][1] = m.at(1, 0);
    rotMat[0][2] = m.at(2, 0);
    rotMat[1][0] = m.at(0, 1);
    rotMat[1][1] = m.at(1, 1);
    rotMat[1][2] = m.at(2, 1);
    rotMat[2][0] = m.at(0, 2);
    rotMat[2][1] = m.at(1, 2);
    rotMat[2][2] = m.at(2, 2);

    glm::quat q = glm::quat_cast(rotMat);
    return fromGLM(q);
}

Quat Quat::slerp(const Quat& a, const Quat& b, float t) noexcept {
    glm::quat qa = toGLM(a);
    glm::quat qb = toGLM(b);
    glm::quat result = glm::slerp(qa, qb, t);
    return fromGLM(result);
}

Quat Quat::nlerp(const Quat& a, const Quat& b, float t) noexcept {
    // Linear interpolation
    float oneMinusT = 1.0f - t;
    Quat result(a.x * oneMinusT + b.x * t, a.y * oneMinusT + b.y * t, a.z * oneMinusT + b.z * t,
                a.w * oneMinusT + b.w * t);

    // Normalize the result
    return result.normalized();
}

// Conversion methods

Mat4 Quat::toMatrix() const noexcept {
    glm::quat q = toGLM(*this);
    glm::mat4 mat = glm::mat4_cast(q);

    // Convert glm::mat4 to Mat4
    Mat4 result;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            result.at(row, col) = mat[col][row];
        }
    }
    return result;
}

void Quat::toAxisAngle(Vec3& outAxis, float& outAngle) const noexcept {
    glm::quat q = toGLM(*this);
    outAngle = glm::angle(q);
    glm::vec3 axis = glm::axis(q);
    outAxis = Vec3(axis.x, axis.y, axis.z);
}

void Quat::toEuler(float& outPitch, float& outYaw, float& outRoll) const noexcept {
    glm::quat q = toGLM(*this);
    glm::vec3 euler = glm::eulerAngles(q);
    outPitch = euler.x;
    outYaw = euler.y;
    outRoll = euler.z;
}

}  // namespace axiom::math
