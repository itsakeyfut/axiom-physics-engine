#pragma once

#include "axiom/core/result.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

namespace axiom::gpu {

// Forward declarations
class VkContext;
class ShaderModule;
class DescriptorSetLayout;

/// Vertex input binding description
/// Describes how vertex data is organized in vertex buffers
struct VertexInputBinding {
    uint32_t binding;             ///< Binding number
    uint32_t stride;              ///< Stride in bytes between consecutive elements
    VkVertexInputRate inputRate;  ///< Per-vertex or per-instance data
};

/// Vertex input attribute description
/// Describes a single vertex attribute (position, normal, color, etc.)
struct VertexInputAttribute {
    uint32_t location;  ///< Shader location
    uint32_t binding;   ///< Vertex buffer binding
    VkFormat format;    ///< Data format (e.g., VK_FORMAT_R32G32B32_SFLOAT for vec3)
    uint32_t offset;    ///< Offset in bytes within the vertex structure
};

/// Vulkan graphics pipeline wrapper
/// This class manages a VkPipeline and VkPipelineLayout for graphics rendering operations.
/// Graphics pipelines define the entire rendering state including shaders, vertex input,
/// rasterization, depth/stencil testing, and color blending.
///
/// Features:
/// - Vertex and fragment shader support
/// - Configurable vertex input layout
/// - Multiple primitive topologies (triangles, lines, points)
/// - Rasterization settings (culling, polygon mode, depth bias)
/// - Depth and stencil testing
/// - Color blending per attachment
/// - Dynamic state for runtime configuration
/// - Dynamic rendering support (VK_KHR_dynamic_rendering)
///
/// Example usage:
/// @code
/// // Create graphics pipeline
/// GraphicsPipelineBuilder builder(context);
/// auto pipelineResult = builder
///     .setVertexShader(*vertexShader)
///     .setFragmentShader(*fragmentShader)
///     .addVertexBinding({0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX})
///     .addVertexAttribute({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)})
///     .addVertexAttribute({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)})
///     .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
///     .setRasterization(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
///     .setDepthStencil(true, true, VK_COMPARE_OP_LESS)
///     .addColorBlendAttachment(false)
///     .build();
///
/// if (pipelineResult.isSuccess()) {
///     auto pipeline = std::move(pipelineResult.value());
///     // Use in render pass
///     pipeline->bind(cmdBuffer);
///     // Draw commands...
/// }
/// @endcode
class GraphicsPipeline {
public:
    /// Destructor - cleans up VkPipeline and VkPipelineLayout
    ~GraphicsPipeline();

    // Non-copyable, non-movable
    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
    GraphicsPipeline(GraphicsPipeline&&) = delete;
    GraphicsPipeline& operator=(GraphicsPipeline&&) = delete;

    /// Bind this pipeline to a command buffer
    /// Must be called before draw operations
    /// @param cmd Command buffer to bind to
    void bind(VkCommandBuffer cmd) const noexcept;

    /// Get the Vulkan pipeline handle
    /// @return VkPipeline handle
    VkPipeline get() const noexcept { return pipeline_; }

    /// Get the pipeline layout handle
    /// @return VkPipelineLayout handle
    VkPipelineLayout getLayout() const noexcept { return layout_; }

private:
    friend class GraphicsPipelineBuilder;

    /// Private constructor - use GraphicsPipelineBuilder instead
    /// @param context Valid VkContext pointer (must outlive this object)
    /// @param pipeline VkPipeline handle (ownership transferred)
    /// @param layout VkPipelineLayout handle (ownership transferred)
    GraphicsPipeline(VkContext* context, VkPipeline pipeline, VkPipelineLayout layout);

    VkContext* context_;                        ///< Vulkan context (not owned)
    VkPipeline pipeline_ = VK_NULL_HANDLE;      ///< Vulkan pipeline handle
    VkPipelineLayout layout_ = VK_NULL_HANDLE;  ///< Vulkan pipeline layout handle
};

/// Builder for creating graphics pipelines
/// This class provides a fluent interface for building graphics pipelines with
/// vertex/fragment shaders, vertex input layout, rasterization settings, depth/stencil,
/// color blending, and dynamic state configuration.
///
/// Example usage:
/// @code
/// GraphicsPipelineBuilder builder(context);
///
/// // Configure viewport and scissor as dynamic state
/// auto pipelineResult = builder
///     .setVertexShader(*vertShader)
///     .setFragmentShader(*fragShader)
///     .addVertexBinding({0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX})
///     .addVertexAttribute({0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0})
///     .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false)
///     .setRasterization(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE,
///                       VK_POLYGON_MODE_FILL)
///     .setDepthStencil(true, true, VK_COMPARE_OP_LESS)
///     .addColorBlendAttachment(false)
///     .addDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
///     .addDynamicState(VK_DYNAMIC_STATE_SCISSOR)
///     .addDescriptorSetLayout(*layout)
///     .setPipelineCache(cache.get())
///     .build();
/// @endcode
class GraphicsPipelineBuilder {
public:
    /// Push constant range specification
    /// Defines a range of push constants that can be updated quickly without descriptor sets
    struct PushConstantRange {
        uint32_t offset;            ///< Offset in bytes (typically 0)
        uint32_t size;              ///< Size in bytes (max 128 bytes recommended)
        VkShaderStageFlags stages;  ///< Shader stages that access this range
    };

    /// Color blend attachment state
    /// Configures blending for a single color attachment
    struct ColorBlendAttachment {
        bool blendEnable;                      ///< Enable blending for this attachment
        VkBlendFactor srcColorBlendFactor;     ///< Source color blend factor
        VkBlendFactor dstColorBlendFactor;     ///< Destination color blend factor
        VkBlendOp colorBlendOp;                ///< Color blend operation
        VkBlendFactor srcAlphaBlendFactor;     ///< Source alpha blend factor
        VkBlendFactor dstAlphaBlendFactor;     ///< Destination alpha blend factor
        VkBlendOp alphaBlendOp;                ///< Alpha blend operation
        VkColorComponentFlags colorWriteMask;  ///< Color write mask

        /// Create default opaque blend attachment (no blending)
        static ColorBlendAttachment opaque() {
            return {false,
                    VK_BLEND_FACTOR_ONE,
                    VK_BLEND_FACTOR_ZERO,
                    VK_BLEND_OP_ADD,
                    VK_BLEND_FACTOR_ONE,
                    VK_BLEND_FACTOR_ZERO,
                    VK_BLEND_OP_ADD,
                    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                        VK_COLOR_COMPONENT_A_BIT};
        }

        /// Create alpha blend attachment (src_alpha, 1 - src_alpha)
        static ColorBlendAttachment alphaBlend() {
            return {true,
                    VK_BLEND_FACTOR_SRC_ALPHA,
                    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    VK_BLEND_OP_ADD,
                    VK_BLEND_FACTOR_ONE,
                    VK_BLEND_FACTOR_ZERO,
                    VK_BLEND_OP_ADD,
                    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                        VK_COLOR_COMPONENT_A_BIT};
        }

        /// Create additive blend attachment (src + dst)
        static ColorBlendAttachment additiveBlend() {
            return {true,
                    VK_BLEND_FACTOR_ONE,
                    VK_BLEND_FACTOR_ONE,
                    VK_BLEND_OP_ADD,
                    VK_BLEND_FACTOR_ONE,
                    VK_BLEND_FACTOR_ONE,
                    VK_BLEND_OP_ADD,
                    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                        VK_COLOR_COMPONENT_A_BIT};
        }
    };

    /// Rendering format configuration for dynamic rendering
    struct RenderingFormats {
        std::vector<VkFormat> colorFormats;            ///< Color attachment formats
        VkFormat depthFormat = VK_FORMAT_UNDEFINED;    ///< Depth attachment format
        VkFormat stencilFormat = VK_FORMAT_UNDEFINED;  ///< Stencil attachment format
    };

    /// Create a builder for graphics pipelines
    /// @param context Valid VkContext pointer
    explicit GraphicsPipelineBuilder(VkContext* context);

    /// Set the vertex shader module
    /// This is required - pipeline creation will fail without a vertex shader
    /// @param shader Vertex shader module (must remain valid until build())
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& setVertexShader(const ShaderModule& shader);

    /// Set the fragment shader module
    /// This is required for rasterization pipelines
    /// @param shader Fragment shader module (must remain valid until build())
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& setFragmentShader(const ShaderModule& shader);

    /// Set the geometry shader module (optional)
    /// @param shader Geometry shader module (must remain valid until build())
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& setGeometryShader(const ShaderModule& shader);

    /// Add a vertex input binding description
    /// Defines how vertex data is organized in a vertex buffer
    /// @param binding Vertex input binding
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& addVertexBinding(const VertexInputBinding& binding);

    /// Add a vertex input attribute description
    /// Defines a single vertex attribute (position, normal, color, etc.)
    /// @param attribute Vertex input attribute
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& addVertexAttribute(const VertexInputAttribute& attribute);

    /// Set input assembly configuration
    /// Defines how vertices are assembled into primitives
    /// @param topology Primitive topology (triangles, lines, points, etc.)
    /// @param primitiveRestartEnable Enable primitive restart with special index value
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& setInputAssembly(VkPrimitiveTopology topology,
                                              bool primitiveRestartEnable = false);

    /// Set rasterization configuration
    /// @param cullMode Face culling mode (none, front, back, or both)
    /// @param frontFace Front face winding order (clockwise or counter-clockwise)
    /// @param polygonMode Polygon rasterization mode (fill, line, or point)
    /// @param lineWidth Line width for line rasterization (default 1.0)
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& setRasterization(VkCullModeFlags cullMode, VkFrontFace frontFace,
                                              VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL,
                                              float lineWidth = 1.0f);

    /// Enable depth bias (useful for shadow mapping)
    /// @param constantFactor Constant depth bias factor
    /// @param slopeFactor Slope-scaled depth bias factor
    /// @param clamp Maximum depth bias
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& setDepthBias(float constantFactor, float slopeFactor, float clamp);

    /// Set depth and stencil testing configuration
    /// @param depthTestEnable Enable depth testing
    /// @param depthWriteEnable Enable depth writes
    /// @param depthCompareOp Depth comparison operation
    /// @param stencilTestEnable Enable stencil testing (default false)
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& setDepthStencil(bool depthTestEnable, bool depthWriteEnable,
                                             VkCompareOp depthCompareOp,
                                             bool stencilTestEnable = false);

    /// Add a color blend attachment state
    /// Call once for each color attachment in the render pass
    /// @param attachment Color blend attachment configuration
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& addColorBlendAttachment(const ColorBlendAttachment& attachment);

    /// Add a color blend attachment with default opaque settings
    /// Convenience method for attachments without blending
    /// @param blendEnable Enable blending (default false)
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& addColorBlendAttachment(bool blendEnable = false);

    /// Set global blend constants
    /// Used by some blend factors (e.g., VK_BLEND_FACTOR_CONSTANT_COLOR)
    /// @param r Red component (0.0 - 1.0)
    /// @param g Green component (0.0 - 1.0)
    /// @param b Blue component (0.0 - 1.0)
    /// @param a Alpha component (0.0 - 1.0)
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& setBlendConstants(float r, float g, float b, float a);

    /// Add a dynamic state
    /// Dynamic states can be changed at command buffer recording time
    /// Common dynamic states: VIEWPORT, SCISSOR, LINE_WIDTH, DEPTH_BIAS
    /// @param state Dynamic state to enable
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& addDynamicState(VkDynamicState state);

    /// Set a single descriptor set layout (replaces previous layouts)
    /// Use this when you have only one descriptor set
    /// @param layout Descriptor set layout (must remain valid until build())
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& setDescriptorSetLayout(const DescriptorSetLayout& layout);

    /// Add a descriptor set layout (appends to existing layouts)
    /// Use this to build pipelines with multiple descriptor sets
    /// @param layout Descriptor set layout to add (must remain valid until build())
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& addDescriptorSetLayout(const DescriptorSetLayout& layout);

    /// Set push constant range
    /// Push constants provide fast data updates without descriptor sets
    /// @param range Push constant range specification
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& setPushConstantRange(const PushConstantRange& range);

    /// Set rendering formats for dynamic rendering (VK_KHR_dynamic_rendering)
    /// Required when using dynamic rendering instead of render passes
    /// @param formats Rendering format configuration
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& setRenderingFormats(const RenderingFormats& formats);

    /// Set multisampling configuration
    /// @param samples Number of samples per pixel (default VK_SAMPLE_COUNT_1_BIT)
    /// @param sampleShadingEnable Enable sample shading (default false)
    /// @param minSampleShading Minimum sample shading fraction (default 1.0)
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& setMultisampling(VkSampleCountFlagBits samples,
                                              bool sampleShadingEnable = false,
                                              float minSampleShading = 1.0f);

    /// Set pipeline cache for faster pipeline creation
    /// @param cache VkPipelineCache handle (use PipelineCache class to manage)
    /// @return Reference to this builder for method chaining
    GraphicsPipelineBuilder& setPipelineCache(VkPipelineCache cache);

    /// Build the graphics pipeline
    /// Creates VkPipelineLayout and VkPipeline based on the configured settings
    /// @return Result containing unique_ptr to GraphicsPipeline or error code
    core::Result<std::unique_ptr<GraphicsPipeline>> build();

private:
    /// Convert ColorBlendAttachment to VkPipelineColorBlendAttachmentState
    static VkPipelineColorBlendAttachmentState
    toVulkanBlendAttachment(const ColorBlendAttachment& attachment);

    VkContext* context_;                            ///< Vulkan context (not owned)
    const ShaderModule* vertexShader_ = nullptr;    ///< Vertex shader (not owned)
    const ShaderModule* fragmentShader_ = nullptr;  ///< Fragment shader (not owned)
    const ShaderModule* geometryShader_ = nullptr;  ///< Geometry shader (not owned)

    std::vector<VertexInputBinding> vertexBindings_;      ///< Vertex input bindings
    std::vector<VertexInputAttribute> vertexAttributes_;  ///< Vertex input attributes

    VkPrimitiveTopology topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  ///< Primitive topology
    bool primitiveRestartEnable_ = false;  ///< Primitive restart enable

    VkCullModeFlags cullMode_ = VK_CULL_MODE_BACK_BIT;         ///< Face culling mode
    VkFrontFace frontFace_ = VK_FRONT_FACE_COUNTER_CLOCKWISE;  ///< Front face winding
    VkPolygonMode polygonMode_ = VK_POLYGON_MODE_FILL;         ///< Polygon mode
    float lineWidth_ = 1.0f;                                   ///< Line width
    bool depthBiasEnable_ = false;                             ///< Depth bias enable
    float depthBiasConstantFactor_ = 0.0f;                     ///< Depth bias constant
    float depthBiasSlopeFactor_ = 0.0f;                        ///< Depth bias slope
    float depthBiasClamp_ = 0.0f;                              ///< Depth bias clamp

    bool depthTestEnable_ = true;                      ///< Depth test enable
    bool depthWriteEnable_ = true;                     ///< Depth write enable
    VkCompareOp depthCompareOp_ = VK_COMPARE_OP_LESS;  ///< Depth compare operation
    bool stencilTestEnable_ = false;                   ///< Stencil test enable

    std::vector<ColorBlendAttachment> colorBlendAttachments_;  ///< Color blend attachments
    float blendConstants_[4] = {0.0f, 0.0f, 0.0f, 0.0f};       ///< Blend constants

    std::vector<VkDynamicState> dynamicStates_;                ///< Dynamic states
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts_;  ///< Descriptor set layouts
    std::optional<PushConstantRange> pushConstantRange_;       ///< Push constant range
    std::optional<RenderingFormats> renderingFormats_;         ///< Rendering formats

    VkSampleCountFlagBits multisampleCount_ = VK_SAMPLE_COUNT_1_BIT;  ///< Sample count
    bool sampleShadingEnable_ = false;                                ///< Sample shading enable
    float minSampleShading_ = 1.0f;                                   ///< Min sample shading

    VkPipelineCache pipelineCache_ = VK_NULL_HANDLE;  ///< Pipeline cache (optional)
};

}  // namespace axiom::gpu
