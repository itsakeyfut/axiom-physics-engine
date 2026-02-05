#include "axiom/gpu/vk_graphics_pipeline.hpp"

#include "axiom/core/error_code.hpp"
#include "axiom/core/logger.hpp"
#include "axiom/gpu/vk_descriptor.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_shader.hpp"

#include <algorithm>
#include <vector>

namespace axiom::gpu {

// ============================================================================
// GraphicsPipeline Implementation
// ============================================================================

GraphicsPipeline::GraphicsPipeline(VkContext* context, VkPipeline pipeline, VkPipelineLayout layout)
    : context_(context), pipeline_(pipeline), layout_(layout) {}

GraphicsPipeline::~GraphicsPipeline() {
    if (context_ && context_->getDevice()) {
        if (pipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(context_->getDevice(), pipeline_, nullptr);
            pipeline_ = VK_NULL_HANDLE;
        }
        if (layout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(context_->getDevice(), layout_, nullptr);
            layout_ = VK_NULL_HANDLE;
        }
    }
}

void GraphicsPipeline::bind(VkCommandBuffer cmd) const noexcept {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
}

// ============================================================================
// GraphicsPipelineBuilder Implementation
// ============================================================================

GraphicsPipelineBuilder::GraphicsPipelineBuilder(VkContext* context) : context_(context) {}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setVertexShader(const ShaderModule& shader) {
    vertexShader_ = &shader;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setFragmentShader(const ShaderModule& shader) {
    fragmentShader_ = &shader;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setGeometryShader(const ShaderModule& shader) {
    geometryShader_ = &shader;
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::addVertexBinding(const VertexInputBinding& binding) {
    vertexBindings_.push_back(binding);
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::addVertexAttribute(const VertexInputAttribute& attribute) {
    vertexAttributes_.push_back(attribute);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setInputAssembly(VkPrimitiveTopology topology,
                                                                   bool primitiveRestartEnable) {
    topology_ = topology;
    primitiveRestartEnable_ = primitiveRestartEnable;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setRasterization(VkCullModeFlags cullMode,
                                                                   VkFrontFace frontFace,
                                                                   VkPolygonMode polygonMode,
                                                                   float lineWidth) {
    cullMode_ = cullMode;
    frontFace_ = frontFace;
    polygonMode_ = polygonMode;
    lineWidth_ = lineWidth;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDepthBias(float constantFactor,
                                                               float slopeFactor, float clamp) {
    depthBiasEnable_ = true;
    depthBiasConstantFactor_ = constantFactor;
    depthBiasSlopeFactor_ = slopeFactor;
    depthBiasClamp_ = clamp;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDepthStencil(bool depthTestEnable,
                                                                  bool depthWriteEnable,
                                                                  VkCompareOp depthCompareOp,
                                                                  bool stencilTestEnable) {
    depthTestEnable_ = depthTestEnable;
    depthWriteEnable_ = depthWriteEnable;
    depthCompareOp_ = depthCompareOp;
    stencilTestEnable_ = stencilTestEnable;
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::addColorBlendAttachment(const ColorBlendAttachment& attachment) {
    colorBlendAttachments_.push_back(attachment);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addColorBlendAttachment(bool blendEnable) {
    if (blendEnable) {
        colorBlendAttachments_.push_back(ColorBlendAttachment::alphaBlend());
    } else {
        colorBlendAttachments_.push_back(ColorBlendAttachment::opaque());
    }
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setBlendConstants(float r, float g, float b,
                                                                    float a) {
    blendConstants_[0] = r;
    blendConstants_[1] = g;
    blendConstants_[2] = b;
    blendConstants_[3] = a;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addDynamicState(VkDynamicState state) {
    dynamicStates_.push_back(state);
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::setDescriptorSetLayout(const DescriptorSetLayout& layout) {
    descriptorSetLayouts_.clear();
    descriptorSetLayouts_.push_back(layout.get());
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::addDescriptorSetLayout(const DescriptorSetLayout& layout) {
    descriptorSetLayouts_.push_back(layout.get());
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::setPushConstantRange(const PushConstantRange& range) {
    pushConstantRange_ = range;
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::setRenderingFormats(const RenderingFormats& formats) {
    renderingFormats_ = formats;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setMultisampling(VkSampleCountFlagBits samples,
                                                                   bool sampleShadingEnable,
                                                                   float minSampleShading) {
    multisampleCount_ = samples;
    sampleShadingEnable_ = sampleShadingEnable;
    minSampleShading_ = minSampleShading;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setPipelineCache(VkPipelineCache cache) {
    pipelineCache_ = cache;
    return *this;
}

VkPipelineColorBlendAttachmentState
GraphicsPipelineBuilder::toVulkanBlendAttachment(const ColorBlendAttachment& attachment) {
    VkPipelineColorBlendAttachmentState state{};
    state.blendEnable = attachment.blendEnable ? VK_TRUE : VK_FALSE;
    state.srcColorBlendFactor = attachment.srcColorBlendFactor;
    state.dstColorBlendFactor = attachment.dstColorBlendFactor;
    state.colorBlendOp = attachment.colorBlendOp;
    state.srcAlphaBlendFactor = attachment.srcAlphaBlendFactor;
    state.dstAlphaBlendFactor = attachment.dstAlphaBlendFactor;
    state.alphaBlendOp = attachment.alphaBlendOp;
    state.colorWriteMask = attachment.colorWriteMask;
    return state;
}

core::Result<std::unique_ptr<GraphicsPipeline>> GraphicsPipelineBuilder::build() {
    // Validate context
    if (!context_) {
        return core::Result<std::unique_ptr<GraphicsPipeline>>::failure(
            core::ErrorCode::InvalidParameter, "VkContext is null");
    }

    // Validate shaders
    if (!vertexShader_) {
        return core::Result<std::unique_ptr<GraphicsPipeline>>::failure(
            core::ErrorCode::InvalidParameter, "Vertex shader not set");
    }

    if (vertexShader_->getStage() != ShaderStage::Vertex) {
        return core::Result<std::unique_ptr<GraphicsPipeline>>::failure(
            core::ErrorCode::InvalidParameter, "Vertex shader must be a vertex shader stage");
    }

    if (fragmentShader_ && fragmentShader_->getStage() != ShaderStage::Fragment) {
        return core::Result<std::unique_ptr<GraphicsPipeline>>::failure(
            core::ErrorCode::InvalidParameter, "Fragment shader must be a fragment shader stage");
    }

    if (geometryShader_ && geometryShader_->getStage() != ShaderStage::Geometry) {
        return core::Result<std::unique_ptr<GraphicsPipeline>>::failure(
            core::ErrorCode::InvalidParameter, "Geometry shader must be a geometry shader stage");
    }

    VkDevice device = context_->getDevice();

    // Create pipeline layout
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts_.size());
    layoutInfo.pSetLayouts = descriptorSetLayouts_.empty() ? nullptr : descriptorSetLayouts_.data();

    // Setup push constant range
    VkPushConstantRange pushConstantRangeVk{};
    if (pushConstantRange_.has_value()) {
        pushConstantRangeVk.stageFlags = pushConstantRange_->stages;
        pushConstantRangeVk.offset = pushConstantRange_->offset;
        pushConstantRangeVk.size = pushConstantRange_->size;

        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstantRangeVk;
    }

    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkResult result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout);
    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("VkGraphicsPipeline", "Failed to create pipeline layout: %d",
                        static_cast<int>(result));
        return core::Result<std::unique_ptr<GraphicsPipeline>>::failure(
            core::ErrorCode::GPU_OPERATION_FAILED, "Failed to create pipeline layout");
    }

    // Setup shader stages
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    // Vertex shader
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = vertexShader_->getStageFlags();
    vertShaderStageInfo.module = vertexShader_->get();
    vertShaderStageInfo.pName = vertexShader_->getEntryPoint();
    shaderStages.push_back(vertShaderStageInfo);

    // Fragment shader (optional)
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    if (fragmentShader_) {
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = fragmentShader_->getStageFlags();
        fragShaderStageInfo.module = fragmentShader_->get();
        fragShaderStageInfo.pName = fragmentShader_->getEntryPoint();
        shaderStages.push_back(fragShaderStageInfo);
    }

    // Geometry shader (optional)
    VkPipelineShaderStageCreateInfo geomShaderStageInfo{};
    if (geometryShader_) {
        geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        geomShaderStageInfo.stage = geometryShader_->getStageFlags();
        geomShaderStageInfo.module = geometryShader_->get();
        geomShaderStageInfo.pName = geometryShader_->getEntryPoint();
        shaderStages.push_back(geomShaderStageInfo);
    }

    // Vertex input state
    std::vector<VkVertexInputBindingDescription> vkBindings;
    for (const auto& binding : vertexBindings_) {
        VkVertexInputBindingDescription vkBinding{};
        vkBinding.binding = binding.binding;
        vkBinding.stride = binding.stride;
        vkBinding.inputRate = binding.inputRate;
        vkBindings.push_back(vkBinding);
    }

    std::vector<VkVertexInputAttributeDescription> vkAttributes;
    for (const auto& attribute : vertexAttributes_) {
        VkVertexInputAttributeDescription vkAttribute{};
        vkAttribute.location = attribute.location;
        vkAttribute.binding = attribute.binding;
        vkAttribute.format = attribute.format;
        vkAttribute.offset = attribute.offset;
        vkAttributes.push_back(vkAttribute);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vkBindings.size());
    vertexInputInfo.pVertexBindingDescriptions = vkBindings.empty() ? nullptr : vkBindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vkAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = vkAttributes.empty() ? nullptr
                                                                        : vkAttributes.data();

    // Input assembly state
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = topology_;
    inputAssembly.primitiveRestartEnable = primitiveRestartEnable_ ? VK_TRUE : VK_FALSE;

    // Viewport state (dynamic if viewport/scissor are in dynamic states)
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Check if viewport and scissor are dynamic
    bool hasViewportDynamic = std::find(dynamicStates_.begin(), dynamicStates_.end(),
                                        VK_DYNAMIC_STATE_VIEWPORT) != dynamicStates_.end();
    bool hasScissorDynamic = std::find(dynamicStates_.begin(), dynamicStates_.end(),
                                       VK_DYNAMIC_STATE_SCISSOR) != dynamicStates_.end();

    VkViewport viewport{};
    VkRect2D scissor{};
    if (!hasViewportDynamic) {
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = 1280.0f;  // Default width
        viewport.height = 720.0f;  // Default height
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        viewportState.pViewports = &viewport;
    }

    if (!hasScissorDynamic) {
        scissor.offset = {0, 0};
        scissor.extent = {1280, 720};  // Default extent
        viewportState.pScissors = &scissor;
    }

    // Rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = polygonMode_;
    rasterizer.lineWidth = lineWidth_;
    rasterizer.cullMode = cullMode_;
    rasterizer.frontFace = frontFace_;
    rasterizer.depthBiasEnable = depthBiasEnable_ ? VK_TRUE : VK_FALSE;
    rasterizer.depthBiasConstantFactor = depthBiasConstantFactor_;
    rasterizer.depthBiasClamp = depthBiasClamp_;
    rasterizer.depthBiasSlopeFactor = depthBiasSlopeFactor_;

    // Multisampling state
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = sampleShadingEnable_ ? VK_TRUE : VK_FALSE;
    multisampling.rasterizationSamples = multisampleCount_;
    multisampling.minSampleShading = minSampleShading_;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // Depth and stencil state
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = depthTestEnable_ ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = depthWriteEnable_ ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = depthCompareOp_;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = stencilTestEnable_ ? VK_TRUE : VK_FALSE;

    // Color blending state
    std::vector<VkPipelineColorBlendAttachmentState> vkBlendAttachments;
    for (const auto& attachment : colorBlendAttachments_) {
        vkBlendAttachments.push_back(toVulkanBlendAttachment(attachment));
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = static_cast<uint32_t>(vkBlendAttachments.size());
    colorBlending.pAttachments = vkBlendAttachments.empty() ? nullptr : vkBlendAttachments.data();
    colorBlending.blendConstants[0] = blendConstants_[0];
    colorBlending.blendConstants[1] = blendConstants_[1];
    colorBlending.blendConstants[2] = blendConstants_[2];
    colorBlending.blendConstants[3] = blendConstants_[3];

    // Dynamic state
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates_.size());
    dynamicState.pDynamicStates = dynamicStates_.empty() ? nullptr : dynamicStates_.data();

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = dynamicStates_.empty() ? nullptr : &dynamicState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;  // Using dynamic rendering
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    // Setup dynamic rendering (VK_KHR_dynamic_rendering)
    VkPipelineRenderingCreateInfoKHR renderingInfo{};
    if (renderingFormats_.has_value()) {
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        renderingInfo.colorAttachmentCount = static_cast<uint32_t>(
            renderingFormats_->colorFormats.size());
        renderingInfo.pColorAttachmentFormats = renderingFormats_->colorFormats.empty()
                                                    ? nullptr
                                                    : renderingFormats_->colorFormats.data();
        renderingInfo.depthAttachmentFormat = renderingFormats_->depthFormat;
        renderingInfo.stencilAttachmentFormat = renderingFormats_->stencilFormat;

        pipelineInfo.pNext = &renderingInfo;
    }

    VkPipeline pipeline = VK_NULL_HANDLE;
    result = vkCreateGraphicsPipelines(device, pipelineCache_, 1, &pipelineInfo, nullptr,
                                       &pipeline);

    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("VkGraphicsPipeline", "Failed to create graphics pipeline: %d",
                        static_cast<int>(result));
        vkDestroyPipelineLayout(device, layout, nullptr);
        return core::Result<std::unique_ptr<GraphicsPipeline>>::failure(
            core::ErrorCode::GPU_OPERATION_FAILED, "Failed to create graphics pipeline");
    }

    AXIOM_LOG_INFO("VkGraphicsPipeline", "Graphics pipeline created successfully");

    // Create GraphicsPipeline object
    auto graphicsPipeline = std::unique_ptr<GraphicsPipeline>(
        new GraphicsPipeline(context_, pipeline, layout));

    return core::Result<std::unique_ptr<GraphicsPipeline>>::success(std::move(graphicsPipeline));
}

}  // namespace axiom::gpu
