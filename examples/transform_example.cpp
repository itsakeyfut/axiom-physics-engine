// Example demonstrating Transform usage for hierarchical transformations
// This example shows how to use Transform for parent-child relationships

#include "axiom/math/constants.hpp"
#include "axiom/math/quat.hpp"
#include "axiom/math/transform.hpp"
#include "axiom/math/vec3.hpp"

#include <iomanip>
#include <iostream>

using namespace axiom::math;

void printVec3(const char* label, const Vec3& v) {
    std::cout << std::setw(25) << label << ": (" << std::fixed << std::setprecision(3) << v.x
              << ", " << v.y << ", " << v.z << ")" << std::endl;
}

void printTransform(const char* label, const Transform& t) {
    std::cout << "\n" << label << ":" << std::endl;
    printVec3("  Position", t.position);
    std::cout << std::setw(25) << "  Rotation (quat)" << ": (" << std::fixed << std::setprecision(3)
              << t.rotation.x << ", " << t.rotation.y << ", " << t.rotation.z << ", "
              << t.rotation.w << ")" << std::endl;
    printVec3("  Scale", t.scale);
}

int main() {
    std::cout << "=== Axiom Transform Example ===" << std::endl;

    // Example 1: Basic transform creation
    std::cout << "\n--- Example 1: Basic Transform ---" << std::endl;
    Transform identity = Transform::identity();
    printTransform("Identity Transform", identity);

    // Example 2: Transform with position and rotation
    std::cout << "\n--- Example 2: Position + Rotation ---" << std::endl;
    Vec3 position(5.0f, 0.0f, 0.0f);
    Quat rotation = Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 2.0f);
    Transform transform(position, rotation);
    printTransform("Transform", transform);

    // Example 3: Transforming points
    std::cout << "\n--- Example 3: Point Transformation ---" << std::endl;
    Vec3 localPoint(1.0f, 0.0f, 0.0f);
    Vec3 worldPoint = transform.transformPoint(localPoint);
    printVec3("Local point", localPoint);
    printVec3("World point", worldPoint);

    // Example 4: Parent-child hierarchy
    std::cout << "\n--- Example 4: Parent-Child Hierarchy ---" << std::endl;

    // Create parent transform (arm)
    Transform parent(Vec3(10.0f, 0.0f, 0.0f),                               // Position
                     Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 4.0f),  // 45 degree rotation
                     Vec3(1.0f, 1.0f, 1.0f)                                 // No scale
    );
    printTransform("Parent (Arm)", parent);

    // Create child transform (hand, relative to arm)
    Transform child(
        Vec3(5.0f, 0.0f, 0.0f),                                // Offset from parent
        Quat::fromAxisAngle(Vec3::unitZ(), PI<float> / 4.0f),  // Additional 45 degree rotation
        Vec3(1.0f, 1.0f, 1.0f)                                 // No scale
    );
    printTransform("Child (Hand)", child);

    // Combine transforms to get world transform of child
    Transform worldTransform = parent * child;
    printTransform("World Transform (Hand)", worldTransform);

    // Example 5: Inverse transform
    std::cout << "\n--- Example 5: Inverse Transform ---" << std::endl;
    Transform inverse = transform.inverse();
    printTransform("Original", transform);
    printTransform("Inverse", inverse);

    // Verify: original * inverse = identity
    Transform shouldBeIdentity = transform * inverse;
    printTransform("Original * Inverse", shouldBeIdentity);

    // Example 6: Matrix conversion
    std::cout << "\n--- Example 6: Matrix Conversion ---" << std::endl;
    Transform t(Vec3(1.0f, 2.0f, 3.0f), Quat::fromAxisAngle(Vec3::unitY(), PI<float> / 6.0f),
                Vec3(2.0f, 2.0f, 2.0f));
    Mat4 matrix = t.toMatrix();
    Transform roundTrip = Transform::fromMatrix(matrix);

    printTransform("Original", t);
    printTransform("From Matrix", roundTrip);

    std::cout << "\n=== End of Examples ===" << std::endl;
    return 0;
}
