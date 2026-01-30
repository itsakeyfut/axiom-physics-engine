#include "axiom/math/aabb.hpp"

#include "axiom/math/mat4.hpp"
#include "axiom/math/vec3.hpp"

namespace axiom::math {

AABB AABB::transform(const Mat4& m) const noexcept {
    // Transform all 8 corners of the AABB and compute the bounding box
    // This is the most accurate method but requires transforming 8 points

    // Get the 8 corners of the AABB
    Vec3 corners[8] = {
        Vec3(min.x, min.y, min.z),  // 0: min corner
        Vec3(max.x, min.y, min.z),  // 1
        Vec3(min.x, max.y, min.z),  // 2
        Vec3(max.x, max.y, min.z),  // 3
        Vec3(min.x, min.y, max.z),  // 4
        Vec3(max.x, min.y, max.z),  // 5
        Vec3(min.x, max.y, max.z),  // 6
        Vec3(max.x, max.y, max.z)   // 7: max corner
    };

    // Transform the first corner to initialize the result AABB
    Vec3 transformedCorner = m.transformPoint(corners[0]);
    AABB result(transformedCorner);

    // Transform remaining corners and expand the AABB
    for (int i = 1; i < 8; ++i) {
        transformedCorner = m.transformPoint(corners[i]);
        result.expand(transformedCorner);
    }

    return result;
}

}  // namespace axiom::math
