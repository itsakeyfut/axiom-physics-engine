#include "axiom/debug/debug_draw.hpp"

#include "axiom/core/logger.hpp"
#include "axiom/gpu/gpu_buffer.hpp"
#include "axiom/gpu/vk_graphics_pipeline.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_memory.hpp"
#include "axiom/gpu/vk_shader.hpp"
#include "axiom/math/mat4.hpp"
#include "axiom/math/quat.hpp"
#include "axiom/math/transform.hpp"
#include "axiom/math/vec3.hpp"
#include "axiom/math/vec4.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace axiom::debug {

using namespace axiom::math;
using namespace axiom::gpu;

namespace {
// Constants for drawing
constexpr float PI = 3.14159265359f;
constexpr float TWO_PI = 2.0f * PI;

// Standard axis colors
const Vec4 COLOR_RED(1.0f, 0.0f, 0.0f, 1.0f);
const Vec4 COLOR_GREEN(0.0f, 1.0f, 0.0f, 1.0f);
const Vec4 COLOR_BLUE(0.0f, 0.0f, 1.0f, 1.0f);
}  // namespace

DebugDraw::DebugDraw(VkContext* context, VkMemoryManager* memManager, const DebugDrawConfig& config)
    : context_(context),
      memManager_(memManager),
      config_(config),
      depthTestEnabled_(config.depthTestEnabled) {
    // Reserve initial capacity
    vertices_.reserve(config_.initialVertexCapacity);

    // Create vertex buffer
    vertexBuffer_ = std::make_unique<GpuBuffer>(
        memManager_, config_.initialVertexCapacity * sizeof(DebugVertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryUsage::CpuToGpu);

    // Create shaders and pipelines
    auto shaderResult = createShaders();
    if (shaderResult.isFailure()) {
        AXIOM_LOG_ERROR("DebugDraw", "Failed to create shaders: %s", shaderResult.errorMessage());
        return;
    }

    auto pipelineResult = initPipelines();
    if (pipelineResult.isFailure()) {
        AXIOM_LOG_ERROR("DebugDraw", "Failed to create pipelines: %s",
                        pipelineResult.errorMessage());
        return;
    }

    AXIOM_LOG_INFO("DebugDraw", "Initialized with capacity for %zu vertices",
                   config_.initialVertexCapacity);
}

DebugDraw::~DebugDraw() {
    // Smart pointers handle cleanup automatically
}

void DebugDraw::drawLine(const Vec3& start, const Vec3& end, const Vec4& color) {
    addLine(start, end, color);
}

void DebugDraw::drawBox(const Vec3& min, const Vec3& max, const Vec4& color) {
    // Draw 12 edges of the box
    // Bottom face (4 edges)
    addLine(Vec3(min.x, min.y, min.z), Vec3(max.x, min.y, min.z), color);
    addLine(Vec3(max.x, min.y, min.z), Vec3(max.x, min.y, max.z), color);
    addLine(Vec3(max.x, min.y, max.z), Vec3(min.x, min.y, max.z), color);
    addLine(Vec3(min.x, min.y, max.z), Vec3(min.x, min.y, min.z), color);

    // Top face (4 edges)
    addLine(Vec3(min.x, max.y, min.z), Vec3(max.x, max.y, min.z), color);
    addLine(Vec3(max.x, max.y, min.z), Vec3(max.x, max.y, max.z), color);
    addLine(Vec3(max.x, max.y, max.z), Vec3(min.x, max.y, max.z), color);
    addLine(Vec3(min.x, max.y, max.z), Vec3(min.x, max.y, min.z), color);

    // Vertical edges (4 edges)
    addLine(Vec3(min.x, min.y, min.z), Vec3(min.x, max.y, min.z), color);
    addLine(Vec3(max.x, min.y, min.z), Vec3(max.x, max.y, min.z), color);
    addLine(Vec3(max.x, min.y, max.z), Vec3(max.x, max.y, max.z), color);
    addLine(Vec3(min.x, min.y, max.z), Vec3(min.x, max.y, max.z), color);
}

void DebugDraw::drawBox(const Transform& transform, const Vec3& halfExtents, const Vec4& color) {
    // Generate 8 corners of the box in local space
    Vec3 corners[8] = {
        Vec3(-halfExtents.x, -halfExtents.y, -halfExtents.z),
        Vec3(halfExtents.x, -halfExtents.y, -halfExtents.z),
        Vec3(halfExtents.x, -halfExtents.y, halfExtents.z),
        Vec3(-halfExtents.x, -halfExtents.y, halfExtents.z),
        Vec3(-halfExtents.x, halfExtents.y, -halfExtents.z),
        Vec3(halfExtents.x, halfExtents.y, -halfExtents.z),
        Vec3(halfExtents.x, halfExtents.y, halfExtents.z),
        Vec3(-halfExtents.x, halfExtents.y, halfExtents.z),
    };

    // Transform corners to world space
    for (int i = 0; i < 8; ++i) {
        corners[i] = transform.transformPoint(corners[i]);
    }

    // Draw 12 edges
    // Bottom face
    addLine(corners[0], corners[1], color);
    addLine(corners[1], corners[2], color);
    addLine(corners[2], corners[3], color);
    addLine(corners[3], corners[0], color);

    // Top face
    addLine(corners[4], corners[5], color);
    addLine(corners[5], corners[6], color);
    addLine(corners[6], corners[7], color);
    addLine(corners[7], corners[4], color);

    // Vertical edges
    addLine(corners[0], corners[4], color);
    addLine(corners[1], corners[5], color);
    addLine(corners[2], corners[6], color);
    addLine(corners[3], corners[7], color);
}

void DebugDraw::drawSphere(const Vec3& center, float radius, const Vec4& color, int segments) {
    if (segments < 4)
        segments = 4;  // Minimum 4 segments

    // Draw meridians (longitude lines)
    for (int i = 0; i < segments; ++i) {
        float angle = TWO_PI * static_cast<float>(i) / static_cast<float>(segments);

        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        // Draw meridian (vertical circle)
        for (int j = 0; j < segments; ++j) {
            float theta = PI * static_cast<float>(j) / static_cast<float>(segments);
            float nextTheta = PI * static_cast<float>(j + 1) / static_cast<float>(segments);

            float sinT1 = std::sin(theta);
            float cosT1 = std::cos(theta);
            float sinT2 = std::sin(nextTheta);
            float cosT2 = std::cos(nextTheta);

            Vec3 p1 = center + Vec3(radius * sinT1 * cosA, radius * cosT1, radius * sinT1 * sinA);
            Vec3 p2 = center + Vec3(radius * sinT2 * cosA, radius * cosT2, radius * sinT2 * sinA);

            addLine(p1, p2, color);
        }
    }

    // Draw parallels (latitude lines)
    for (int j = 1; j < segments; ++j) {
        float theta = PI * static_cast<float>(j) / static_cast<float>(segments);
        float sinT = std::sin(theta);
        float cosT = std::cos(theta);
        float r = radius * sinT;
        float y = radius * cosT;

        for (int i = 0; i < segments; ++i) {
            float angle = TWO_PI * static_cast<float>(i) / static_cast<float>(segments);
            float nextAngle = TWO_PI * static_cast<float>(i + 1) / static_cast<float>(segments);

            Vec3 p1 = center + Vec3(r * std::cos(angle), y, r * std::sin(angle));
            Vec3 p2 = center + Vec3(r * std::cos(nextAngle), y, r * std::sin(nextAngle));

            addLine(p1, p2, color);
        }
    }
}

void DebugDraw::drawCapsule(const Vec3& start, const Vec3& end, float radius, const Vec4& color,
                            int segments) {
    if (segments < 4)
        segments = 4;

    // Calculate capsule axis
    Vec3 axis = end - start;
    float height = axis.length();
    if (height < 1e-6f) {
        // Degenerate capsule - just draw a sphere
        drawSphere(start, radius, color, segments);
        return;
    }

    Vec3 dir = axis / height;

    // Find perpendicular vectors
    Vec3 up = std::abs(dir.y) < 0.9f ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
    Vec3 right = cross(dir, up).normalized();
    Vec3 forward = cross(right, dir);

    // Draw cylinder body (vertical lines)
    for (int i = 0; i < segments; ++i) {
        float angle = TWO_PI * static_cast<float>(i) / static_cast<float>(segments);
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);

        Vec3 offset = right * x + forward * z;
        addLine(start + offset, end + offset, color);
    }

    // Draw end caps (hemispheres)
    for (int i = 0; i < segments; ++i) {
        float angle = TWO_PI * static_cast<float>(i) / static_cast<float>(segments);
        float nextAngle = TWO_PI * static_cast<float>(i + 1) / static_cast<float>(segments);

        // Draw rings on both hemispheres
        for (int j = 0; j < segments / 2; ++j) {
            float theta = PI * 0.5f * static_cast<float>(j) / static_cast<float>(segments / 2);
            float nextTheta = PI * 0.5f * static_cast<float>(j + 1) /
                              static_cast<float>(segments / 2);

            float r1 = radius * std::cos(theta);
            float r2 = radius * std::cos(nextTheta);
            float y1 = radius * std::sin(theta);
            float y2 = radius * std::sin(nextTheta);

            // Start hemisphere
            Vec3 p1 = start + dir * y1 + (right * std::cos(angle) + forward * std::sin(angle)) * r1;
            Vec3 p2 = start + dir * y2 + (right * std::cos(angle) + forward * std::sin(angle)) * r2;
            addLine(p1, p2, color);

            // End hemisphere
            Vec3 p3 = end - dir * y1 + (right * std::cos(angle) + forward * std::sin(angle)) * r1;
            Vec3 p4 = end - dir * y2 + (right * std::cos(angle) + forward * std::sin(angle)) * r2;
            addLine(p3, p4, color);

            // Draw latitude lines on start cap
            Vec3 p5 = start + dir * y1 +
                      (right * std::cos(nextAngle) + forward * std::sin(nextAngle)) * r1;
            addLine(p1, p5, color);

            // Draw latitude lines on end cap
            Vec3 p6 = end - dir * y1 +
                      (right * std::cos(nextAngle) + forward * std::sin(nextAngle)) * r1;
            addLine(p3, p6, color);
        }
    }
}

void DebugDraw::drawCone(const Vec3& base, const Vec3& tip, float radius, const Vec4& color,
                         int segments) {
    if (segments < 3)
        segments = 3;

    // Calculate cone axis
    Vec3 axis = tip - base;
    float height = axis.length();
    if (height < 1e-6f) {
        return;  // Degenerate cone
    }

    Vec3 dir = axis / height;

    // Find perpendicular vectors
    Vec3 up = std::abs(dir.y) < 0.9f ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
    Vec3 right = cross(dir, up).normalized();
    Vec3 forward = cross(right, dir);

    // Draw base circle and lines to tip
    for (int i = 0; i < segments; ++i) {
        float angle = TWO_PI * static_cast<float>(i) / static_cast<float>(segments);
        float nextAngle = TWO_PI * static_cast<float>(i + 1) / static_cast<float>(segments);

        Vec3 p1 = base + (right * std::cos(angle) + forward * std::sin(angle)) * radius;
        Vec3 p2 = base + (right * std::cos(nextAngle) + forward * std::sin(nextAngle)) * radius;

        // Base circle
        addLine(p1, p2, color);

        // Line to tip
        addLine(p1, tip, color);
    }
}

void DebugDraw::drawArrow(const Vec3& start, const Vec3& end, const Vec4& color, float headSize) {
    // Draw main line
    addLine(start, end, color);

    // Calculate arrow head
    Vec3 dir = end - start;
    float length = dir.length();
    if (length < 1e-6f) {
        return;  // Degenerate arrow
    }

    dir = dir / length;
    float coneHeight = length * headSize;
    float coneRadius = coneHeight * 0.5f;

    Vec3 coneBase = end - dir * coneHeight;

    // Draw cone for arrow head
    drawCone(coneBase, end, coneRadius, color, 8);
}

void DebugDraw::drawPlane(const Vec3& center, const Vec3& normal, float size, const Vec4& color) {
    Vec3 n = normal.normalized();

    // Find perpendicular vectors
    Vec3 up = std::abs(n.y) < 0.9f ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
    Vec3 right = cross(n, up).normalized();
    Vec3 forward = cross(right, n);

    float halfSize = size * 0.5f;

    // Draw plane edges
    Vec3 corners[4] = {
        center + (right * halfSize + forward * halfSize),
        center + (right * halfSize - forward * halfSize),
        center + (-right * halfSize - forward * halfSize),
        center + (-right * halfSize + forward * halfSize),
    };

    addLine(corners[0], corners[1], color);
    addLine(corners[1], corners[2], color);
    addLine(corners[2], corners[3], color);
    addLine(corners[3], corners[0], color);

    // Draw normal indicator
    drawArrow(center, center + n * size * 0.3f, color, 0.2f);
}

void DebugDraw::drawAxis(const Transform& transform, float size) {
    Vec3 origin = transform.position;
    Vec3 xAxis = transform.transformDirection(Vec3(size, 0, 0));
    Vec3 yAxis = transform.transformDirection(Vec3(0, size, 0));
    Vec3 zAxis = transform.transformDirection(Vec3(0, 0, size));

    // X axis - red
    drawArrow(origin, origin + xAxis, COLOR_RED, 0.15f);

    // Y axis - green
    drawArrow(origin, origin + yAxis, COLOR_GREEN, 0.15f);

    // Z axis - blue
    drawArrow(origin, origin + zAxis, COLOR_BLUE, 0.15f);
}

void DebugDraw::drawGrid(const Vec3& center, float size, int divisions, const Vec4& color) {
    if (divisions < 1)
        divisions = 1;

    float halfSize = size * 0.5f;
    float step = size / static_cast<float>(divisions);

    // Draw lines along X axis
    for (int i = 0; i <= divisions; ++i) {
        float z = -halfSize + step * static_cast<float>(i);
        Vec3 start = center + Vec3(-halfSize, 0, z);
        Vec3 end = center + Vec3(halfSize, 0, z);
        addLine(start, end, color);
    }

    // Draw lines along Z axis
    for (int i = 0; i <= divisions; ++i) {
        float x = -halfSize + step * static_cast<float>(i);
        Vec3 start = center + Vec3(x, 0, -halfSize);
        Vec3 end = center + Vec3(x, 0, halfSize);
        addLine(start, end, color);
    }
}

void DebugDraw::flush(VkCommandBuffer cmd, const Mat4& viewProj) {
    if (vertices_.empty()) {
        return;  // Nothing to draw
    }

    // Ensure vertex buffer is large enough
    size_t requiredSize = vertices_.size() * sizeof(DebugVertex);
    if (vertexBuffer_->getSize() < requiredSize) {
        // Resize vertex buffer
        auto resizeResult = vertexBuffer_->resize(requiredSize * 2);  // Double size for growth
        if (resizeResult.isFailure()) {
            AXIOM_LOG_ERROR("DebugDraw", "Failed to resize vertex buffer: %s",
                            resizeResult.errorMessage());
            return;
        }
    }

    // Upload vertices to GPU
    auto uploadResult = vertexBuffer_->upload(vertices_.data(), requiredSize);
    if (uploadResult.isFailure()) {
        AXIOM_LOG_ERROR("DebugDraw", "Failed to upload vertices: %s", uploadResult.errorMessage());
        return;
    }

    // Select pipeline based on depth test setting
    GraphicsPipeline* activePipeline = depthTestEnabled_ ? pipeline_.get() : pipelineNoDepth_.get();
    if (!activePipeline) {
        AXIOM_LOG_ERROR("DebugDraw", "Pipeline not initialized");
        return;
    }

    // Bind pipeline
    activePipeline->bind(cmd);

    // Push view-projection matrix
    vkCmdPushConstants(cmd, activePipeline->getLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(Mat4), &viewProj);

    // Bind vertex buffer
    VkBuffer vb = vertexBuffer_->getBuffer();
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &offset);

    // Draw all vertices as lines
    vkCmdDraw(cmd, static_cast<uint32_t>(vertices_.size()), 1, 0, 0);
}

void DebugDraw::clear() noexcept {
    vertices_.clear();
}

void DebugDraw::addVertex(const Vec3& position, const Vec4& color) {
    vertices_.push_back({position, color});
}

void DebugDraw::addLine(const Vec3& start, const Vec3& end, const Vec4& color) {
    addVertex(start, color);
    addVertex(end, color);
}

void DebugDraw::ensureCapacity(size_t additionalVertices) {
    if (vertices_.capacity() < vertices_.size() + additionalVertices) {
        vertices_.reserve((vertices_.size() + additionalVertices) * 2);
    }
}

core::Result<void> DebugDraw::createShaders() {
    // Load vertex shader
    auto vertResult = ShaderModule::createFromFile(context_, "shaders/debug/line.vert.spv",
                                                   ShaderStage::Vertex);
    if (vertResult.isFailure()) {
        AXIOM_LOG_ERROR("DebugDraw", "Failed to load vertex shader: %s", vertResult.errorMessage());
        return core::Result<void>::failure(vertResult.errorCode(), "Failed to load vertex shader");
    }
    vertexShader_ = std::move(vertResult.value());

    // Load fragment shader
    auto fragResult = ShaderModule::createFromFile(context_, "shaders/debug/line.frag.spv",
                                                   ShaderStage::Fragment);
    if (fragResult.isFailure()) {
        AXIOM_LOG_ERROR("DebugDraw", "Failed to load fragment shader: %s",
                        fragResult.errorMessage());
        return core::Result<void>::failure(fragResult.errorCode(),
                                           "Failed to load fragment shader");
    }
    fragmentShader_ = std::move(fragResult.value());

    return core::Result<void>::success();
}

core::Result<void> DebugDraw::initPipelines() {
    if (!vertexShader_ || !fragmentShader_) {
        return core::Result<void>::failure(
            core::ErrorCode::GPU_INVALID_OPERATION,
            "Shaders not loaded - call createShaders() first or compile shaders");
    }

    // Define vertex input layout
    VertexInputBinding binding{};
    binding.binding = 0;
    binding.stride = sizeof(DebugVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VertexInputAttribute positionAttr{};
    positionAttr.location = 0;
    positionAttr.binding = 0;
    positionAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
    positionAttr.offset = offsetof(DebugVertex, position);

    VertexInputAttribute colorAttr{};
    colorAttr.location = 1;
    colorAttr.binding = 0;
    colorAttr.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    colorAttr.offset = offsetof(DebugVertex, color);

    // Push constant range for view-projection matrix
    GraphicsPipelineBuilder::PushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(Mat4);
    pushConstant.stages = VK_SHADER_STAGE_VERTEX_BIT;

    // Rendering formats (common formats for debug rendering)
    GraphicsPipelineBuilder::RenderingFormats formats{};
    formats.colorFormats = {VK_FORMAT_R8G8B8A8_UNORM};  // Standard RGBA format
    formats.depthFormat = VK_FORMAT_D32_SFLOAT;         // Standard depth format

    // Create pipeline WITH depth testing
    {
        GraphicsPipelineBuilder builder(context_);
        auto result = builder.setVertexShader(*vertexShader_)
                          .setFragmentShader(*fragmentShader_)
                          .addVertexBinding(binding)
                          .addVertexAttribute(positionAttr)
                          .addVertexAttribute(colorAttr)
                          .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, false)
                          .setRasterization(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                            VK_POLYGON_MODE_FILL, 1.0f)
                          .setDepthStencil(true, false, VK_COMPARE_OP_LESS_OR_EQUAL, false)
                          .addColorBlendAttachment(
                              GraphicsPipelineBuilder::ColorBlendAttachment::alphaBlend())
                          .addDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
                          .addDynamicState(VK_DYNAMIC_STATE_SCISSOR)
                          .setPushConstantRange(pushConstant)
                          .setRenderingFormats(formats)
                          .build();

        if (result.isFailure()) {
            AXIOM_LOG_ERROR("DebugDraw", "Failed to create pipeline with depth test: %s",
                            result.errorMessage());
            return core::Result<void>::failure(result.errorCode(),
                                               "Failed to create pipeline with depth test");
        }
        pipeline_ = std::move(result.value());
    }

    // Create pipeline WITHOUT depth testing (always on top)
    {
        GraphicsPipelineBuilder builder(context_);
        auto result = builder.setVertexShader(*vertexShader_)
                          .setFragmentShader(*fragmentShader_)
                          .addVertexBinding(binding)
                          .addVertexAttribute(positionAttr)
                          .addVertexAttribute(colorAttr)
                          .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, false)
                          .setRasterization(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                            VK_POLYGON_MODE_FILL, 1.0f)
                          .setDepthStencil(false, false, VK_COMPARE_OP_ALWAYS, false)
                          .addColorBlendAttachment(
                              GraphicsPipelineBuilder::ColorBlendAttachment::alphaBlend())
                          .addDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
                          .addDynamicState(VK_DYNAMIC_STATE_SCISSOR)
                          .setPushConstantRange(pushConstant)
                          .setRenderingFormats(formats)
                          .build();

        if (result.isFailure()) {
            AXIOM_LOG_ERROR("DebugDraw", "Failed to create pipeline without depth test: %s",
                            result.errorMessage());
            return core::Result<void>::failure(result.errorCode(),
                                               "Failed to create pipeline without depth test");
        }
        pipelineNoDepth_ = std::move(result.value());
    }

    AXIOM_LOG_INFO("DebugDraw", "Successfully created debug draw pipelines");
    return core::Result<void>::success();
}

}  // namespace axiom::debug
