#include "axiom/math/mat4.hpp"

#include "axiom/math/vec3.hpp"
#include "axiom/math/vec4.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstring>

namespace axiom::math {

// Helper function to convert Mat4 to glm::mat4
inline glm::mat4 toGLM(const Mat4& mat) noexcept {
    return glm::make_mat4(mat.m);
}

// Helper function to convert glm::mat4 to Mat4
inline Mat4 fromGLM(const glm::mat4& mat) noexcept {
    Mat4 result;
    std::memcpy(result.m, glm::value_ptr(mat), 16 * sizeof(float));
    return result;
}

// Constructors

Mat4::Mat4() noexcept {
    // Initialize to identity matrix
    m[0] = 1.0f;
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[4] = 0.0f;
    m[5] = 1.0f;
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 0.0f;
    m[9] = 0.0f;
    m[10] = 1.0f;
    m[11] = 0.0f;
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;
}

Mat4::Mat4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13,
           float m20, float m21, float m22, float m23, float m30, float m31, float m32,
           float m33) noexcept {
    // Column 0
    m[0] = m00;
    m[1] = m01;
    m[2] = m02;
    m[3] = m03;
    // Column 1
    m[4] = m10;
    m[5] = m11;
    m[6] = m12;
    m[7] = m13;
    // Column 2
    m[8] = m20;
    m[9] = m21;
    m[10] = m22;
    m[11] = m23;
    // Column 3
    m[12] = m30;
    m[13] = m31;
    m[14] = m32;
    m[15] = m33;
}

Mat4::Mat4(const float* data) noexcept {
    std::memcpy(m, data, 16 * sizeof(float));
}

// Matrix operations

Mat4 Mat4::operator*(const Mat4& other) const noexcept {
    glm::mat4 a = toGLM(*this);
    glm::mat4 b = toGLM(other);
    return fromGLM(a * b);
}

Vec4 Mat4::operator*(const Vec4& v) const noexcept {
    glm::mat4 mat = toGLM(*this);
    glm::vec4 vec(v.x, v.y, v.z, v.w);
    glm::vec4 result = mat * vec;
    return Vec4(result.x, result.y, result.z, result.w);
}

Vec3 Mat4::transformPoint(const Vec3& v) const noexcept {
    glm::mat4 mat = toGLM(*this);
    glm::vec4 vec(v.x, v.y, v.z, 1.0f);
    glm::vec4 result = mat * vec;
    return Vec3(result.x, result.y, result.z);
}

Vec3 Mat4::transformVector(const Vec3& v) const noexcept {
    glm::mat4 mat = toGLM(*this);
    glm::vec4 vec(v.x, v.y, v.z, 0.0f);
    glm::vec4 result = mat * vec;
    return Vec3(result.x, result.y, result.z);
}

Mat4 Mat4::transpose() const noexcept {
    glm::mat4 mat = toGLM(*this);
    return fromGLM(glm::transpose(mat));
}

Mat4 Mat4::inverse() const noexcept {
    glm::mat4 mat = toGLM(*this);
    glm::mat4 inv = glm::inverse(mat);
    return fromGLM(inv);
}

float Mat4::determinant() const noexcept {
    glm::mat4 mat = toGLM(*this);
    return glm::determinant(mat);
}

// Comparison operators

bool Mat4::operator==(const Mat4& other) const noexcept {
    for (size_t i = 0; i < 16; ++i) {
        if (m[i] != other.m[i]) {
            return false;
        }
    }
    return true;
}

// Static factory methods

Mat4 Mat4::identity() noexcept {
    Mat4 result;
    // Default constructor already creates identity
    return result;
}

Mat4 Mat4::zero() noexcept {
    Mat4 result;
    for (size_t i = 0; i < 16; ++i) {
        result.m[i] = 0.0f;
    }
    return result;
}

Mat4 Mat4::translation(const Vec3& t) noexcept {
    glm::mat4 mat = glm::translate(glm::mat4(1.0f), glm::vec3(t.x, t.y, t.z));
    return fromGLM(mat);
}

Mat4 Mat4::rotation(const Quat& /* q */) noexcept {
    // Quaternion not implemented yet, will be added in Issue #10
    // For now, return identity
    return identity();
}

Mat4 Mat4::rotationX(float angleRadians) noexcept {
    glm::mat4 mat = glm::rotate(glm::mat4(1.0f), angleRadians, glm::vec3(1.0f, 0.0f, 0.0f));
    return fromGLM(mat);
}

Mat4 Mat4::rotationY(float angleRadians) noexcept {
    glm::mat4 mat = glm::rotate(glm::mat4(1.0f), angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
    return fromGLM(mat);
}

Mat4 Mat4::rotationZ(float angleRadians) noexcept {
    glm::mat4 mat = glm::rotate(glm::mat4(1.0f), angleRadians, glm::vec3(0.0f, 0.0f, 1.0f));
    return fromGLM(mat);
}

Mat4 Mat4::rotationAxis(const Vec3& axis, float angleRadians) noexcept {
    glm::mat4 mat = glm::rotate(glm::mat4(1.0f), angleRadians, glm::vec3(axis.x, axis.y, axis.z));
    return fromGLM(mat);
}

Mat4 Mat4::scaling(const Vec3& s) noexcept {
    glm::mat4 mat = glm::scale(glm::mat4(1.0f), glm::vec3(s.x, s.y, s.z));
    return fromGLM(mat);
}

Mat4 Mat4::scaling(float s) noexcept {
    return scaling(Vec3(s, s, s));
}

Mat4 Mat4::perspective(float fovYRadians, float aspectRatio, float nearPlane,
                       float farPlane) noexcept {
    glm::mat4 mat = glm::perspective(fovYRadians, aspectRatio, nearPlane, farPlane);
    return fromGLM(mat);
}

Mat4 Mat4::orthographic(float left, float right, float bottom, float top, float nearPlane,
                        float farPlane) noexcept {
    glm::mat4 mat = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
    return fromGLM(mat);
}

Mat4 Mat4::lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) noexcept {
    glm::mat4 mat = glm::lookAt(glm::vec3(eye.x, eye.y, eye.z),
                                glm::vec3(target.x, target.y, target.z),
                                glm::vec3(up.x, up.y, up.z));
    return fromGLM(mat);
}

}  // namespace axiom::math
