#pragma once

#include "axiom/core/result.hpp"
#include "axiom/math/mat4.hpp"
#include "axiom/math/vec3.hpp"
#include "axiom/math/vec4.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace axiom {

// Forward declarations
namespace math {
struct Transform;
}  // namespace math

namespace gpu {
class VkContext;
class VkMemoryManager;
class GpuBuffer;
class GraphicsPipeline;
class ShaderModule;
}  // namespace gpu

namespace debug {

/// Configuration for debug draw system
struct DebugDrawConfig {
    size_t initialVertexCapacity = 10000;  ///< Initial capacity for vertex buffer
    bool depthTestEnabled = true;          ///< Default depth test state
};

/// Debug rendering system for visualizing physics primitives
/// This class provides an efficient batched rendering system for drawing debug primitives
/// like lines, boxes, spheres, and other shapes in 3D space. All primitives are accumulated
/// into a vertex buffer and rendered in a single draw call for performance.
///
/// Features:
/// - Dynamic vertex buffer with automatic resizing
/// - Batch rendering of all primitives
/// - Optional depth testing
/// - Support for basic 3D primitives
/// - Efficient line-based rendering
///
/// Example usage:
/// @code
/// DebugDraw debugDraw(context, &memManager);
///
/// void renderFrame() {
///     debugDraw.clear();
///
///     // Draw world grid
///     debugDraw.drawGrid(Vec3(0, 0, 0), 10.0f, 10, Vec4(0.3f, 0.3f, 0.3f, 1));
///
///     // Draw coordinate axes
///     debugDraw.drawAxis(Transform::identity(), 1.0f);
///
///     // Draw AABB
///     debugDraw.drawBox(aabb.min, aabb.max, Vec4(1, 1, 0, 1));
///
///     // Render all primitives
///     cmd.begin();
///     RenderPass::beginSimple(cmd, colorView, depthView, extent);
///     Mat4 viewProj = camera.getProjection() * camera.getView();
///     debugDraw.flush(cmd, viewProj);
///     RenderPass::end(cmd);
///     cmd.end();
/// }
/// @endcode
class DebugDraw {
public:
    /// Create a debug draw system
    /// @param context Vulkan context (must outlive this object)
    /// @param memManager Memory manager for buffer allocation (must outlive this object)
    /// @param config Configuration settings
    DebugDraw(gpu::VkContext* context, gpu::VkMemoryManager* memManager,
              const DebugDrawConfig& config = {});

    /// Destructor - cleans up resources
    ~DebugDraw();

    // Non-copyable (owns GPU resources)
    DebugDraw(const DebugDraw&) = delete;
    DebugDraw& operator=(const DebugDraw&) = delete;

    // Non-movable (simplified ownership)
    DebugDraw(DebugDraw&&) = delete;
    DebugDraw& operator=(DebugDraw&&) = delete;

    // === Primitive Drawing API ===

    /// Draw a line between two points
    /// @param start Start position
    /// @param end End position
    /// @param color Line color (RGBA)
    void drawLine(const math::Vec3& start, const math::Vec3& end, const math::Vec4& color);

    /// Draw a wireframe box defined by min and max corners
    /// @param min Minimum corner (world space)
    /// @param max Maximum corner (world space)
    /// @param color Box color (RGBA)
    void drawBox(const math::Vec3& min, const math::Vec3& max, const math::Vec4& color);

    /// Draw an oriented wireframe box
    /// @param transform Transform defining box position and orientation
    /// @param halfExtents Half-extents of the box (half-width, half-height, half-depth)
    /// @param color Box color (RGBA)
    void drawBox(const math::Transform& transform, const math::Vec3& halfExtents,
                 const math::Vec4& color);

    /// Draw a wireframe sphere
    /// @param center Sphere center (world space)
    /// @param radius Sphere radius
    /// @param color Sphere color (RGBA)
    /// @param segments Number of meridian/latitude segments (default 16)
    void drawSphere(const math::Vec3& center, float radius, const math::Vec4& color,
                    int segments = 16);

    /// Draw a wireframe capsule (cylinder with hemispherical caps)
    /// @param start Start point (center of first hemisphere)
    /// @param end End point (center of second hemisphere)
    /// @param radius Capsule radius
    /// @param color Capsule color (RGBA)
    /// @param segments Number of segments around circumference (default 8)
    void drawCapsule(const math::Vec3& start, const math::Vec3& end, float radius,
                     const math::Vec4& color, int segments = 8);

    /// Draw a wireframe cone
    /// @param base Cone base center
    /// @param tip Cone tip position
    /// @param radius Base radius
    /// @param color Cone color (RGBA)
    /// @param segments Number of segments around base (default 8)
    void drawCone(const math::Vec3& base, const math::Vec3& tip, float radius,
                  const math::Vec4& color, int segments = 8);

    /// Draw an arrow (line with cone head)
    /// @param start Arrow start position
    /// @param end Arrow end position
    /// @param color Arrow color (RGBA)
    /// @param headSize Relative size of arrow head (0.0-1.0, default 0.1)
    void drawArrow(const math::Vec3& start, const math::Vec3& end, const math::Vec4& color,
                   float headSize = 0.1f);

    /// Draw a plane with normal indicator
    /// @param center Plane center position
    /// @param normal Plane normal vector
    /// @param size Plane display size
    /// @param color Plane color (RGBA)
    void drawPlane(const math::Vec3& center, const math::Vec3& normal, float size,
                   const math::Vec4& color);

    /// Draw coordinate axes (X=red, Y=green, Z=blue)
    /// @param transform Transform defining coordinate system
    /// @param size Length of each axis (default 1.0)
    void drawAxis(const math::Transform& transform, float size = 1.0f);

    /// Draw a grid in the XZ plane
    /// @param center Grid center position
    /// @param size Total grid size (world units)
    /// @param divisions Number of divisions per side
    /// @param color Grid line color (RGBA)
    void drawGrid(const math::Vec3& center, float size, int divisions, const math::Vec4& color);

    // === Rendering ===

    /// Render all accumulated debug primitives
    /// This must be called inside an active render pass with compatible format.
    /// All vertices are uploaded to GPU and drawn in a single draw call.
    /// @param cmd Command buffer to record draw commands
    /// @param viewProj View-projection matrix for camera transformation
    void flush(VkCommandBuffer cmd, const math::Mat4& viewProj);

    /// Clear all accumulated vertices
    /// Call this at the start of each frame before adding new primitives
    void clear() noexcept;

    // === Settings ===

    /// Enable or disable depth testing
    /// When disabled, debug primitives are always drawn on top
    /// @param enabled true to enable depth testing
    void setDepthTestEnabled(bool enabled) noexcept { depthTestEnabled_ = enabled; }

    /// Get current depth test state
    /// @return true if depth testing is enabled
    bool getDepthTestEnabled() const noexcept { return depthTestEnabled_; }

    /// Get current vertex count
    /// @return Number of vertices accumulated
    size_t getVertexCount() const noexcept { return vertices_.size(); }

private:
    /// Vertex format for debug rendering
    struct DebugVertex {
        math::Vec3 position;  ///< Vertex position (world space)
        math::Vec4 color;     ///< Vertex color (RGBA)
    };

    /// Add a single vertex to the buffer
    /// @param position Vertex position
    /// @param color Vertex color
    void addVertex(const math::Vec3& position, const math::Vec4& color);

    /// Add a line segment (2 vertices)
    /// @param start Line start position
    /// @param end Line end position
    /// @param color Line color
    void addLine(const math::Vec3& start, const math::Vec3& end, const math::Vec4& color);

    /// Ensure vertex buffer has capacity for additional vertices
    /// Reallocates if necessary
    /// @param additionalVertices Number of vertices to reserve space for
    void ensureCapacity(size_t additionalVertices);

    /// Initialize graphics pipelines
    /// @return Result indicating success or failure
    core::Result<void> initPipelines();

    /// Create shader modules
    /// @return Result indicating success or failure
    core::Result<void> createShaders();

    gpu::VkContext* context_;           ///< Vulkan context (not owned)
    gpu::VkMemoryManager* memManager_;  ///< Memory manager (not owned)
    DebugDrawConfig config_;            ///< Configuration settings

    std::vector<DebugVertex> vertices_;             ///< Accumulated vertices (CPU-side)
    std::unique_ptr<gpu::GpuBuffer> vertexBuffer_;  ///< GPU vertex buffer

    std::unique_ptr<gpu::ShaderModule> vertexShader_;    ///< Vertex shader
    std::unique_ptr<gpu::ShaderModule> fragmentShader_;  ///< Fragment shader

    std::unique_ptr<gpu::GraphicsPipeline> pipeline_;         ///< Pipeline with depth test
    std::unique_ptr<gpu::GraphicsPipeline> pipelineNoDepth_;  ///< Pipeline without depth test

    bool depthTestEnabled_;  ///< Current depth test state
};

}  // namespace debug
}  // namespace axiom
