#include "axiom/math/constants.hpp"
#include "axiom/math/mat4.hpp"
#include "axiom/math/vec3.hpp"
#include "axiom/math/vec4.hpp"

#include <gtest/gtest.h>

#include <cmath>

using namespace axiom::math;

// Helper function for floating point comparison
constexpr float TEST_EPSILON = 1e-6f;

bool almostEqual(float a, float b, float epsilon = TEST_EPSILON) {
    return std::fabs(a - b) < epsilon;
}

bool almostEqual(const Mat4& a, const Mat4& b, float epsilon = TEST_EPSILON) {
    for (size_t i = 0; i < 16; ++i) {
        if (!almostEqual(a.m[i], b.m[i], epsilon)) {
            return false;
        }
    }
    return true;
}

// Constructor tests

TEST(Mat4Test, DefaultConstructor) {
    Mat4 m;
    // Should initialize to identity matrix
    EXPECT_FLOAT_EQ(m.m[0], 1.0f);
    EXPECT_FLOAT_EQ(m.m[5], 1.0f);
    EXPECT_FLOAT_EQ(m.m[10], 1.0f);
    EXPECT_FLOAT_EQ(m.m[15], 1.0f);

    EXPECT_FLOAT_EQ(m.m[1], 0.0f);
    EXPECT_FLOAT_EQ(m.m[2], 0.0f);
    EXPECT_FLOAT_EQ(m.m[4], 0.0f);
}

TEST(Mat4Test, ValueConstructor) {
    Mat4 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f,
           15.0f, 16.0f);

    // Column 0
    EXPECT_FLOAT_EQ(m.m[0], 1.0f);
    EXPECT_FLOAT_EQ(m.m[1], 2.0f);
    EXPECT_FLOAT_EQ(m.m[2], 3.0f);
    EXPECT_FLOAT_EQ(m.m[3], 4.0f);

    // Column 1
    EXPECT_FLOAT_EQ(m.m[4], 5.0f);
    EXPECT_FLOAT_EQ(m.m[5], 6.0f);
    EXPECT_FLOAT_EQ(m.m[6], 7.0f);
    EXPECT_FLOAT_EQ(m.m[7], 8.0f);

    // Column 2
    EXPECT_FLOAT_EQ(m.m[8], 9.0f);
    EXPECT_FLOAT_EQ(m.m[9], 10.0f);
    EXPECT_FLOAT_EQ(m.m[10], 11.0f);
    EXPECT_FLOAT_EQ(m.m[11], 12.0f);

    // Column 3
    EXPECT_FLOAT_EQ(m.m[12], 13.0f);
    EXPECT_FLOAT_EQ(m.m[13], 14.0f);
    EXPECT_FLOAT_EQ(m.m[14], 15.0f);
    EXPECT_FLOAT_EQ(m.m[15], 16.0f);
}

TEST(Mat4Test, ArrayConstructor) {
    float data[16] = {1.0f, 2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,  8.0f,
                      9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f};

    Mat4 m(data);

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_FLOAT_EQ(m.m[i], data[i]);
    }
}

// Accessor tests

TEST(Mat4Test, ArrayAccess) {
    Mat4 m;
    m[0] = 5.0f;
    m[5] = 10.0f;
    m[10] = 15.0f;
    m[15] = 20.0f;

    EXPECT_FLOAT_EQ(m[0], 5.0f);
    EXPECT_FLOAT_EQ(m[5], 10.0f);
    EXPECT_FLOAT_EQ(m[10], 15.0f);
    EXPECT_FLOAT_EQ(m[15], 20.0f);
}

TEST(Mat4Test, RowColumnAccess) {
    Mat4 m;
    m.at(0, 0) = 1.0f;
    m.at(1, 1) = 2.0f;
    m.at(2, 2) = 3.0f;
    m.at(3, 3) = 4.0f;

    EXPECT_FLOAT_EQ(m.at(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(m.at(1, 1), 2.0f);
    EXPECT_FLOAT_EQ(m.at(2, 2), 3.0f);
    EXPECT_FLOAT_EQ(m.at(3, 3), 4.0f);

    // Test off-diagonal
    m.at(0, 1) = 5.0f;
    EXPECT_FLOAT_EQ(m.at(0, 1), 5.0f);
    EXPECT_FLOAT_EQ(m.m[4], 5.0f);  // Column 1, row 0
}

// Factory method tests

TEST(Mat4Test, IdentityMatrix) {
    Mat4 m = Mat4::identity();

    // Diagonal should be 1
    EXPECT_FLOAT_EQ(m.m[0], 1.0f);
    EXPECT_FLOAT_EQ(m.m[5], 1.0f);
    EXPECT_FLOAT_EQ(m.m[10], 1.0f);
    EXPECT_FLOAT_EQ(m.m[15], 1.0f);

    // Off-diagonal should be 0
    for (size_t i = 0; i < 16; ++i) {
        if (i != 0 && i != 5 && i != 10 && i != 15) {
            EXPECT_FLOAT_EQ(m.m[i], 0.0f);
        }
    }
}

TEST(Mat4Test, ZeroMatrix) {
    Mat4 m = Mat4::zero();

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_FLOAT_EQ(m.m[i], 0.0f);
    }
}

TEST(Mat4Test, TranslationMatrix) {
    Vec3 t(1.0f, 2.0f, 3.0f);
    Mat4 m = Mat4::translation(t);

    // Translation values should be in column 3
    EXPECT_FLOAT_EQ(m.m[12], 1.0f);
    EXPECT_FLOAT_EQ(m.m[13], 2.0f);
    EXPECT_FLOAT_EQ(m.m[14], 3.0f);

    // Should be identity except for translation column
    EXPECT_FLOAT_EQ(m.m[0], 1.0f);
    EXPECT_FLOAT_EQ(m.m[5], 1.0f);
    EXPECT_FLOAT_EQ(m.m[10], 1.0f);
    EXPECT_FLOAT_EQ(m.m[15], 1.0f);
}

TEST(Mat4Test, ScalingMatrix) {
    Vec3 s(2.0f, 3.0f, 4.0f);
    Mat4 m = Mat4::scaling(s);

    // Diagonal should have scaling factors
    EXPECT_FLOAT_EQ(m.m[0], 2.0f);
    EXPECT_FLOAT_EQ(m.m[5], 3.0f);
    EXPECT_FLOAT_EQ(m.m[10], 4.0f);
    EXPECT_FLOAT_EQ(m.m[15], 1.0f);
}

TEST(Mat4Test, UniformScalingMatrix) {
    Mat4 m = Mat4::scaling(2.5f);

    EXPECT_FLOAT_EQ(m.m[0], 2.5f);
    EXPECT_FLOAT_EQ(m.m[5], 2.5f);
    EXPECT_FLOAT_EQ(m.m[10], 2.5f);
    EXPECT_FLOAT_EQ(m.m[15], 1.0f);
}

TEST(Mat4Test, RotationX) {
    float angle = PI<float> / 2.0f;  // 90 degrees
    Mat4 m = Mat4::rotationX(angle);

    // Rotating around X axis, so first column should remain (1,0,0,0)
    EXPECT_TRUE(almostEqual(m.m[0], 1.0f));
    EXPECT_TRUE(almostEqual(m.m[1], 0.0f));
    EXPECT_TRUE(almostEqual(m.m[2], 0.0f));
    EXPECT_TRUE(almostEqual(m.m[3], 0.0f));

    // Test rotation: rotate (0,1,0) around X should give (0,0,1)
    Vec3 v(0.0f, 1.0f, 0.0f);
    Vec3 result = m.transformVector(v);
    EXPECT_TRUE(almostEqual(result.x, 0.0f));
    EXPECT_TRUE(almostEqual(result.y, 0.0f));
    EXPECT_TRUE(almostEqual(result.z, 1.0f));
}

TEST(Mat4Test, RotationY) {
    float angle = PI<float> / 2.0f;  // 90 degrees
    Mat4 m = Mat4::rotationY(angle);

    // Test rotation: rotate (1,0,0) around Y should give (0,0,-1)
    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 result = m.transformVector(v);
    EXPECT_TRUE(almostEqual(result.x, 0.0f));
    EXPECT_TRUE(almostEqual(result.y, 0.0f));
    EXPECT_TRUE(almostEqual(result.z, -1.0f));
}

TEST(Mat4Test, RotationZ) {
    float angle = PI<float> / 2.0f;  // 90 degrees
    Mat4 m = Mat4::rotationZ(angle);

    // Test rotation: rotate (1,0,0) around Z should give (0,1,0)
    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 result = m.transformVector(v);
    EXPECT_TRUE(almostEqual(result.x, 0.0f));
    EXPECT_TRUE(almostEqual(result.y, 1.0f));
    EXPECT_TRUE(almostEqual(result.z, 0.0f));
}

TEST(Mat4Test, RotationAxis) {
    // Rotate around Z axis by 90 degrees
    Vec3 axis(0.0f, 0.0f, 1.0f);
    float angle = PI<float> / 2.0f;
    Mat4 m = Mat4::rotationAxis(axis, angle);

    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 result = m.transformVector(v);
    EXPECT_TRUE(almostEqual(result.x, 0.0f));
    EXPECT_TRUE(almostEqual(result.y, 1.0f));
    EXPECT_TRUE(almostEqual(result.z, 0.0f));
}

// Matrix operation tests

TEST(Mat4Test, MatrixMultiplication) {
    Mat4 a = Mat4::translation(Vec3(1.0f, 2.0f, 3.0f));
    Mat4 b = Mat4::scaling(Vec3(2.0f, 2.0f, 2.0f));

    Mat4 c = a * b;

    // Result should scale then translate
    Vec3 v(1.0f, 1.0f, 1.0f);
    Vec3 result = c.transformPoint(v);

    EXPECT_TRUE(almostEqual(result.x, 3.0f));  // 1*2 + 1 = 3
    EXPECT_TRUE(almostEqual(result.y, 4.0f));  // 1*2 + 2 = 4
    EXPECT_TRUE(almostEqual(result.z, 5.0f));  // 1*2 + 3 = 5
}

TEST(Mat4Test, IdentityMultiplication) {
    Mat4 a = Mat4::translation(Vec3(1.0f, 2.0f, 3.0f));
    Mat4 identity = Mat4::identity();

    Mat4 c1 = a * identity;
    Mat4 c2 = identity * a;

    EXPECT_TRUE(almostEqual(c1, a));
    EXPECT_TRUE(almostEqual(c2, a));
}

TEST(Mat4Test, VectorMultiplication) {
    Mat4 m = Mat4::scaling(Vec3(2.0f, 3.0f, 4.0f));
    Vec4 v(1.0f, 1.0f, 1.0f, 1.0f);

    Vec4 result = m * v;

    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 3.0f);
    EXPECT_FLOAT_EQ(result.z, 4.0f);
    EXPECT_FLOAT_EQ(result.w, 1.0f);
}

TEST(Mat4Test, TransformPoint) {
    Mat4 m = Mat4::translation(Vec3(10.0f, 20.0f, 30.0f));
    Vec3 v(1.0f, 2.0f, 3.0f);

    Vec3 result = m.transformPoint(v);

    EXPECT_FLOAT_EQ(result.x, 11.0f);
    EXPECT_FLOAT_EQ(result.y, 22.0f);
    EXPECT_FLOAT_EQ(result.z, 33.0f);
}

TEST(Mat4Test, TransformVector) {
    Mat4 m = Mat4::translation(Vec3(10.0f, 20.0f, 30.0f));
    Vec3 v(1.0f, 2.0f, 3.0f);

    Vec3 result = m.transformVector(v);

    // Vectors are not affected by translation (w=0)
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST(Mat4Test, Transpose) {
    Mat4 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f,
           15.0f, 16.0f);

    Mat4 t = m.transpose();

    // Check that rows become columns
    EXPECT_FLOAT_EQ(t.at(0, 0), m.at(0, 0));
    EXPECT_FLOAT_EQ(t.at(0, 1), m.at(1, 0));
    EXPECT_FLOAT_EQ(t.at(0, 2), m.at(2, 0));
    EXPECT_FLOAT_EQ(t.at(0, 3), m.at(3, 0));

    EXPECT_FLOAT_EQ(t.at(1, 0), m.at(0, 1));
    EXPECT_FLOAT_EQ(t.at(2, 0), m.at(0, 2));
    EXPECT_FLOAT_EQ(t.at(3, 0), m.at(0, 3));
}

TEST(Mat4Test, TransposeIdentity) {
    Mat4 identity = Mat4::identity();
    Mat4 transposed = identity.transpose();

    EXPECT_TRUE(almostEqual(identity, transposed));
}

TEST(Mat4Test, Determinant) {
    Mat4 identity = Mat4::identity();
    EXPECT_TRUE(almostEqual(identity.determinant(), 1.0f));

    Mat4 scale = Mat4::scaling(Vec3(2.0f, 3.0f, 4.0f));
    EXPECT_TRUE(almostEqual(scale.determinant(), 24.0f));  // 2 * 3 * 4 = 24
}

TEST(Mat4Test, DeterminantZero) {
    // Create a singular matrix (determinant = 0)
    Mat4 m = Mat4::zero();
    EXPECT_TRUE(almostEqual(m.determinant(), 0.0f));
}

TEST(Mat4Test, Inverse) {
    Mat4 m = Mat4::translation(Vec3(1.0f, 2.0f, 3.0f));
    Mat4 inv = m.inverse();

    // m * inv should give identity
    Mat4 result = m * inv;
    Mat4 identity = Mat4::identity();

    EXPECT_TRUE(almostEqual(result, identity));
}

TEST(Mat4Test, InverseScaling) {
    Mat4 m = Mat4::scaling(Vec3(2.0f, 3.0f, 4.0f));
    Mat4 inv = m.inverse();

    // Inverse of scaling should be 1/scale
    EXPECT_TRUE(almostEqual(inv.m[0], 0.5f));
    EXPECT_TRUE(almostEqual(inv.m[5], 1.0f / 3.0f));
    EXPECT_TRUE(almostEqual(inv.m[10], 0.25f));
}

TEST(Mat4Test, InverseRotation) {
    Mat4 m = Mat4::rotationZ(PI<float> / 4.0f);  // 45 degrees
    Mat4 inv = m.inverse();

    // m * inv should give identity
    Mat4 result = m * inv;
    Mat4 identity = Mat4::identity();

    EXPECT_TRUE(almostEqual(result, identity));
}

TEST(Mat4Test, InversePrecision) {
    // Test that inverse calculation has sufficient precision (< 1e-6 relative error)
    Mat4 m = Mat4::rotationY(0.7f);
    Mat4 scale = Mat4::scaling(Vec3(1.5f, 2.0f, 2.5f));
    Mat4 trans = Mat4::translation(Vec3(3.0f, 4.0f, 5.0f));

    // Complex transformation
    Mat4 combined = trans * m * scale;
    Mat4 inv = combined.inverse();

    // combined * inv should be identity
    Mat4 result = combined * inv;
    Mat4 identity = Mat4::identity();

    // Check each element has relative error < 1e-6
    for (size_t i = 0; i < 16; ++i) {
        float expected = identity.m[i];
        float actual = result.m[i];
        float absError = std::fabs(actual - expected);

        // For values close to 0, use absolute error
        if (std::fabs(expected) < TEST_EPSILON) {
            EXPECT_LT(absError, TEST_EPSILON) << "Element " << i;
        } else {
            // For non-zero values, use relative error
            float relError = absError / std::fabs(expected);
            EXPECT_LT(relError, TEST_EPSILON) << "Element " << i;
        }
    }
}

// Comparison tests

TEST(Mat4Test, Equality) {
    Mat4 a = Mat4::identity();
    Mat4 b = Mat4::identity();

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(Mat4Test, Inequality) {
    Mat4 a = Mat4::identity();
    Mat4 b = Mat4::zero();

    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a == b);
}

// Projection matrix tests

TEST(Mat4Test, PerspectiveProjection) {
    float fov = PI<float> / 2.0f;  // 90 degrees
    float aspect = 16.0f / 9.0f;
    float near = 0.1f;
    float far = 100.0f;

    Mat4 proj = Mat4::perspective(fov, aspect, near, far);

    // Perspective matrix should not be identity
    EXPECT_FALSE(almostEqual(proj, Mat4::identity()));

    // w component should be modified by -z (perspective divide)
    // For a point at z=-10, the w component should become 10
    Vec4 v(1.0f, 1.0f, -10.0f, 1.0f);
    Vec4 result = proj * v;
    EXPECT_TRUE(almostEqual(result.w, 10.0f, 0.01f));
}

TEST(Mat4Test, OrthographicProjection) {
    Mat4 ortho = Mat4::orthographic(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);

    // Orthographic matrix should not be identity
    EXPECT_FALSE(almostEqual(ortho, Mat4::identity()));

    // w component should remain unchanged (no perspective divide)
    Vec4 v(0.5f, 0.5f, -1.0f, 1.0f);
    Vec4 result = ortho * v;
    EXPECT_FLOAT_EQ(result.w, 1.0f);
}

TEST(Mat4Test, LookAt) {
    Vec3 eye(0.0f, 0.0f, 5.0f);
    Vec3 target(0.0f, 0.0f, 0.0f);
    Vec3 up(0.0f, 1.0f, 0.0f);

    Mat4 view = Mat4::lookAt(eye, target, up);

    // View matrix should not be identity
    EXPECT_FALSE(almostEqual(view, Mat4::identity()));

    // The eye position when transformed should be at origin
    Vec3 transformedEye = view.transformPoint(eye);
    EXPECT_TRUE(almostEqual(transformedEye.x, 0.0f));
    EXPECT_TRUE(almostEqual(transformedEye.y, 0.0f));
    EXPECT_TRUE(almostEqual(transformedEye.z, 0.0f));
}

// Edge case tests

TEST(Mat4Test, TransformationComposition) {
    // Test TRS (Translation, Rotation, Scale) order
    Vec3 scale(2.0f, 2.0f, 2.0f);
    float angle = PI<float> / 4.0f;
    Vec3 translation(10.0f, 20.0f, 30.0f);

    Mat4 S = Mat4::scaling(scale);
    Mat4 R = Mat4::rotationZ(angle);
    Mat4 T = Mat4::translation(translation);

    // Standard transformation order: T * R * S
    Mat4 transform = T * R * S;

    Vec3 point(1.0f, 0.0f, 0.0f);

    // Apply transformations manually
    Vec3 scaled = Vec3(point.x * scale.x, point.y * scale.y, point.z * scale.z);
    Vec3 rotated = R.transformVector(scaled);
    Vec3 translated = rotated + translation;

    // Apply using combined matrix
    Vec3 result = transform.transformPoint(point);

    EXPECT_TRUE(almostEqual(result.x, translated.x));
    EXPECT_TRUE(almostEqual(result.y, translated.y));
    EXPECT_TRUE(almostEqual(result.z, translated.z));
}

TEST(Mat4Test, DoubleTranspose) {
    Mat4 m(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f,
           15.0f, 16.0f);

    Mat4 tt = m.transpose().transpose();

    EXPECT_TRUE(almostEqual(m, tt));
}

TEST(Mat4Test, DoubleInverse) {
    Mat4 m = Mat4::rotationY(0.5f);
    Mat4 ii = m.inverse().inverse();

    EXPECT_TRUE(almostEqual(m, ii));
}
