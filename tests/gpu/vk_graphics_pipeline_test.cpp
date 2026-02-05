#include "axiom/gpu/vk_command.hpp"
#include "axiom/gpu/vk_descriptor.hpp"
#include "axiom/gpu/vk_graphics_pipeline.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_shader.hpp"

#include <gtest/gtest.h>

#include <filesystem>

using namespace axiom::gpu;
using namespace axiom::core;

// Test fixture for GraphicsPipeline tests
class VkGraphicsPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create Vulkan context
        auto contextResult = VkContext::create();
        if (contextResult.isSuccess()) {
            context_ = std::move(contextResult.value());
        } else {
            GTEST_SKIP() << "Vulkan not available: " << contextResult.errorMessage()
                         << " (this is expected in CI environments without GPU)";
        }

        // Load vertex shader
        if (std::filesystem::exists(vertexShaderPath_)) {
            auto shaderResult = ShaderModule::createFromFile(context_.get(), vertexShaderPath_,
                                                             ShaderStage::Vertex);
            if (shaderResult.isSuccess()) {
                vertexShader_ = std::move(shaderResult.value());
            }
        }

        // Load fragment shader
        if (std::filesystem::exists(fragmentShaderPath_)) {
            auto shaderResult = ShaderModule::createFromFile(context_.get(), fragmentShaderPath_,
                                                             ShaderStage::Fragment);
            if (shaderResult.isSuccess()) {
                fragmentShader_ = std::move(shaderResult.value());
            }
        }

        // Skip tests if shaders not available
        if (!vertexShader_ || !fragmentShader_) {
            GTEST_SKIP() << "Test shaders not found (compile shaders/test/*.vert/frag with slangc)";
        }
    }

    void TearDown() override {
        fragmentShader_.reset();
        vertexShader_.reset();
        context_.reset();
    }

    std::unique_ptr<VkContext> context_;
    std::unique_ptr<ShaderModule> vertexShader_;
    std::unique_ptr<ShaderModule> fragmentShader_;
    const std::string vertexShaderPath_ = "shaders/test/simple.vert.spv";
    const std::string fragmentShaderPath_ = "shaders/test/simple.frag.spv";
};

// Test basic graphics pipeline creation
TEST_F(VkGraphicsPipelineTest, CreateBasicPipeline) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(vertexShader_, nullptr);
    ASSERT_NE(fragmentShader_, nullptr);

    GraphicsPipelineBuilder builder(context_.get());
    auto result = builder.setVertexShader(*vertexShader_)
                      .setFragmentShader(*fragmentShader_)
                      .setRenderingFormats(
                          {{VK_FORMAT_R8G8B8A8_UNORM}, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED})
                      .build();

    ASSERT_TRUE(result.isSuccess()) << "Failed: " << result.errorMessage();

    auto pipeline = std::move(result.value());
    EXPECT_NE(pipeline->get(), VK_NULL_HANDLE);
    EXPECT_NE(pipeline->getLayout(), VK_NULL_HANDLE);
}

// Test pipeline creation with vertex input
TEST_F(VkGraphicsPipelineTest, CreatePipelineWithVertexInput) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(vertexShader_, nullptr);
    ASSERT_NE(fragmentShader_, nullptr);

    // Define vertex input (position + normal)
    struct Vertex {
        float position[3];
        float normal[3];
    };

    GraphicsPipelineBuilder builder(context_.get());
    auto result = builder.setVertexShader(*vertexShader_)
                      .setFragmentShader(*fragmentShader_)
                      .addVertexBinding({0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX})
                      .addVertexAttribute({0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0})
                      .addVertexAttribute(
                          {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)})
                      .setRenderingFormats(
                          {{VK_FORMAT_R8G8B8A8_UNORM}, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED})
                      .build();

    ASSERT_TRUE(result.isSuccess());
    auto pipeline = std::move(result.value());
    EXPECT_NE(pipeline->get(), VK_NULL_HANDLE);
}

// Test pipeline creation with different primitive topologies
TEST_F(VkGraphicsPipelineTest, CreatePipelineWithDifferentTopologies) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(vertexShader_, nullptr);
    ASSERT_NE(fragmentShader_, nullptr);

    // Test triangle list
    {
        GraphicsPipelineBuilder builder(context_.get());
        auto result = builder.setVertexShader(*vertexShader_)
                          .setFragmentShader(*fragmentShader_)
                          .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false)
                          .setRenderingFormats({{VK_FORMAT_R8G8B8A8_UNORM},
                                                VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_UNDEFINED})
                          .build();

        ASSERT_TRUE(result.isSuccess());
    }

    // Test line list
    {
        GraphicsPipelineBuilder builder(context_.get());
        auto result = builder.setVertexShader(*vertexShader_)
                          .setFragmentShader(*fragmentShader_)
                          .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, false)
                          .setRenderingFormats({{VK_FORMAT_R8G8B8A8_UNORM},
                                                VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_UNDEFINED})
                          .build();

        ASSERT_TRUE(result.isSuccess());
    }

    // Test point list
    {
        GraphicsPipelineBuilder builder(context_.get());
        auto result = builder.setVertexShader(*vertexShader_)
                          .setFragmentShader(*fragmentShader_)
                          .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_POINT_LIST, false)
                          .setRenderingFormats({{VK_FORMAT_R8G8B8A8_UNORM},
                                                VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_UNDEFINED})
                          .build();

        ASSERT_TRUE(result.isSuccess());
    }
}

// Test pipeline creation with different cull modes
TEST_F(VkGraphicsPipelineTest, CreatePipelineWithDifferentCullModes) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(vertexShader_, nullptr);
    ASSERT_NE(fragmentShader_, nullptr);

    // Test back face culling
    {
        GraphicsPipelineBuilder builder(context_.get());
        auto result = builder.setVertexShader(*vertexShader_)
                          .setFragmentShader(*fragmentShader_)
                          .setRasterization(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                          .setRenderingFormats({{VK_FORMAT_R8G8B8A8_UNORM},
                                                VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_UNDEFINED})
                          .build();

        ASSERT_TRUE(result.isSuccess());
    }

    // Test front face culling
    {
        GraphicsPipelineBuilder builder(context_.get());
        auto result = builder.setVertexShader(*vertexShader_)
                          .setFragmentShader(*fragmentShader_)
                          .setRasterization(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                          .setRenderingFormats({{VK_FORMAT_R8G8B8A8_UNORM},
                                                VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_UNDEFINED})
                          .build();

        ASSERT_TRUE(result.isSuccess());
    }

    // Test no culling
    {
        GraphicsPipelineBuilder builder(context_.get());
        auto result = builder.setVertexShader(*vertexShader_)
                          .setFragmentShader(*fragmentShader_)
                          .setRasterization(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                          .setRenderingFormats({{VK_FORMAT_R8G8B8A8_UNORM},
                                                VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_UNDEFINED})
                          .build();

        ASSERT_TRUE(result.isSuccess());
    }
}

// Test pipeline creation with depth testing configurations
TEST_F(VkGraphicsPipelineTest, CreatePipelineWithDepthTesting) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(vertexShader_, nullptr);
    ASSERT_NE(fragmentShader_, nullptr);

    // Depth test enabled with write
    {
        GraphicsPipelineBuilder builder(context_.get());
        auto result = builder.setVertexShader(*vertexShader_)
                          .setFragmentShader(*fragmentShader_)
                          .setDepthStencil(true, true, VK_COMPARE_OP_LESS)
                          .setRenderingFormats({{VK_FORMAT_R8G8B8A8_UNORM},
                                                VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_UNDEFINED})
                          .build();

        ASSERT_TRUE(result.isSuccess());
    }

    // Depth test enabled without write
    {
        GraphicsPipelineBuilder builder(context_.get());
        auto result = builder.setVertexShader(*vertexShader_)
                          .setFragmentShader(*fragmentShader_)
                          .setDepthStencil(true, false, VK_COMPARE_OP_LESS_OR_EQUAL)
                          .setRenderingFormats({{VK_FORMAT_R8G8B8A8_UNORM},
                                                VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_UNDEFINED})
                          .build();

        ASSERT_TRUE(result.isSuccess());
    }

    // Depth test disabled
    {
        GraphicsPipelineBuilder builder(context_.get());
        auto result = builder.setVertexShader(*vertexShader_)
                          .setFragmentShader(*fragmentShader_)
                          .setDepthStencil(false, false, VK_COMPARE_OP_LESS)
                          .setRenderingFormats({{VK_FORMAT_R8G8B8A8_UNORM},
                                                VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_UNDEFINED})
                          .build();

        ASSERT_TRUE(result.isSuccess());
    }
}

// Test pipeline creation with color blending
TEST_F(VkGraphicsPipelineTest, CreatePipelineWithColorBlending) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(vertexShader_, nullptr);
    ASSERT_NE(fragmentShader_, nullptr);

    // Opaque blending
    {
        GraphicsPipelineBuilder builder(context_.get());
        auto result = builder.setVertexShader(*vertexShader_)
                          .setFragmentShader(*fragmentShader_)
                          .addColorBlendAttachment(false)
                          .setRenderingFormats({{VK_FORMAT_R8G8B8A8_UNORM},
                                                VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_UNDEFINED})
                          .build();

        ASSERT_TRUE(result.isSuccess());
    }

    // Alpha blending
    {
        GraphicsPipelineBuilder builder(context_.get());
        auto result = builder.setVertexShader(*vertexShader_)
                          .setFragmentShader(*fragmentShader_)
                          .addColorBlendAttachment(
                              GraphicsPipelineBuilder::ColorBlendAttachment::alphaBlend())
                          .setRenderingFormats({{VK_FORMAT_R8G8B8A8_UNORM},
                                                VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_UNDEFINED})
                          .build();

        ASSERT_TRUE(result.isSuccess());
    }

    // Additive blending
    {
        GraphicsPipelineBuilder builder(context_.get());
        auto result = builder.setVertexShader(*vertexShader_)
                          .setFragmentShader(*fragmentShader_)
                          .addColorBlendAttachment(
                              GraphicsPipelineBuilder::ColorBlendAttachment::additiveBlend())
                          .setRenderingFormats({{VK_FORMAT_R8G8B8A8_UNORM},
                                                VK_FORMAT_D32_SFLOAT,
                                                VK_FORMAT_UNDEFINED})
                          .build();

        ASSERT_TRUE(result.isSuccess());
    }
}

// Test pipeline creation with dynamic states
TEST_F(VkGraphicsPipelineTest, CreatePipelineWithDynamicStates) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(vertexShader_, nullptr);
    ASSERT_NE(fragmentShader_, nullptr);

    GraphicsPipelineBuilder builder(context_.get());
    auto result = builder.setVertexShader(*vertexShader_)
                      .setFragmentShader(*fragmentShader_)
                      .addDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
                      .addDynamicState(VK_DYNAMIC_STATE_SCISSOR)
                      .addDynamicState(VK_DYNAMIC_STATE_LINE_WIDTH)
                      .addColorBlendAttachment(false)
                      .setRenderingFormats(
                          {{VK_FORMAT_R8G8B8A8_UNORM}, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED})
                      .build();

    ASSERT_TRUE(result.isSuccess());
    auto pipeline = std::move(result.value());
    EXPECT_NE(pipeline->get(), VK_NULL_HANDLE);
}

// Test pipeline creation with descriptor set layout
TEST_F(VkGraphicsPipelineTest, CreatePipelineWithDescriptorLayout) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(vertexShader_, nullptr);
    ASSERT_NE(fragmentShader_, nullptr);

    // Create descriptor set layout
    DescriptorSetLayoutBuilder layoutBuilder(context_.get());
    auto layoutResult = layoutBuilder
                            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                            .build();

    ASSERT_TRUE(layoutResult.isSuccess());
    auto layout = std::move(layoutResult.value());

    // Create pipeline with descriptor layout
    GraphicsPipelineBuilder builder(context_.get());
    auto result = builder.setVertexShader(*vertexShader_)
                      .setFragmentShader(*fragmentShader_)
                      .setDescriptorSetLayout(*layout)
                      .addColorBlendAttachment(false)
                      .setRenderingFormats(
                          {{VK_FORMAT_R8G8B8A8_UNORM}, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED})
                      .build();

    ASSERT_TRUE(result.isSuccess());
    auto pipeline = std::move(result.value());
    EXPECT_NE(pipeline->get(), VK_NULL_HANDLE);
}

// Test pipeline creation with push constants
TEST_F(VkGraphicsPipelineTest, CreatePipelineWithPushConstants) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(vertexShader_, nullptr);
    ASSERT_NE(fragmentShader_, nullptr);

    GraphicsPipelineBuilder builder(context_.get());
    auto result = builder.setVertexShader(*vertexShader_)
                      .setFragmentShader(*fragmentShader_)
                      .setPushConstantRange({0, 128, VK_SHADER_STAGE_VERTEX_BIT})
                      .addColorBlendAttachment(false)
                      .setRenderingFormats(
                          {{VK_FORMAT_R8G8B8A8_UNORM}, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED})
                      .build();

    ASSERT_TRUE(result.isSuccess());
    auto pipeline = std::move(result.value());
    EXPECT_NE(pipeline->get(), VK_NULL_HANDLE);
}

// Test pipeline creation with wireframe mode
TEST_F(VkGraphicsPipelineTest, CreatePipelineWithWireframeMode) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(vertexShader_, nullptr);
    ASSERT_NE(fragmentShader_, nullptr);

    GraphicsPipelineBuilder builder(context_.get());
    auto result = builder.setVertexShader(*vertexShader_)
                      .setFragmentShader(*fragmentShader_)
                      .setRasterization(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                        VK_POLYGON_MODE_LINE, 1.0f)
                      .addColorBlendAttachment(false)
                      .setRenderingFormats(
                          {{VK_FORMAT_R8G8B8A8_UNORM}, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED})
                      .build();

    ASSERT_TRUE(result.isSuccess());
    auto pipeline = std::move(result.value());
    EXPECT_NE(pipeline->get(), VK_NULL_HANDLE);
}

// Test pipeline creation with multisampling
TEST_F(VkGraphicsPipelineTest, CreatePipelineWithMultisampling) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(vertexShader_, nullptr);
    ASSERT_NE(fragmentShader_, nullptr);

    GraphicsPipelineBuilder builder(context_.get());
    auto result = builder.setVertexShader(*vertexShader_)
                      .setFragmentShader(*fragmentShader_)
                      .setMultisampling(VK_SAMPLE_COUNT_4_BIT, false, 1.0f)
                      .addColorBlendAttachment(false)
                      .setRenderingFormats(
                          {{VK_FORMAT_R8G8B8A8_UNORM}, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED})
                      .build();

    ASSERT_TRUE(result.isSuccess());
    auto pipeline = std::move(result.value());
    EXPECT_NE(pipeline->get(), VK_NULL_HANDLE);
}

// Test pipeline creation failure with no vertex shader
TEST_F(VkGraphicsPipelineTest, CreatePipelineFailsWithoutVertexShader) {
    ASSERT_NE(context_, nullptr);

    GraphicsPipelineBuilder builder(context_.get());
    auto result = builder.setFragmentShader(*fragmentShader_)
                      .setRenderingFormats(
                          {{VK_FORMAT_R8G8B8A8_UNORM}, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED})
                      .build();

    ASSERT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test pipeline bind command
TEST_F(VkGraphicsPipelineTest, PipelineBindCommand) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(vertexShader_, nullptr);
    ASSERT_NE(fragmentShader_, nullptr);

    // Create pipeline
    GraphicsPipelineBuilder builder(context_.get());
    auto pipelineResult = builder.setVertexShader(*vertexShader_)
                              .setFragmentShader(*fragmentShader_)
                              .addColorBlendAttachment(false)
                              .setRenderingFormats({{VK_FORMAT_R8G8B8A8_UNORM},
                                                    VK_FORMAT_D32_SFLOAT,
                                                    VK_FORMAT_UNDEFINED})
                              .build();

    ASSERT_TRUE(pipelineResult.isSuccess());
    auto pipeline = std::move(pipelineResult.value());

    // Create command buffer for testing
    CommandPool commandPool(context_.get(), context_->getGraphicsQueueFamily());
    VkCommandBuffer cmdBuf = commandPool.allocate();

    ASSERT_NE(cmdBuf, VK_NULL_HANDLE);

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuf, &beginInfo);

    // Bind pipeline (should not crash)
    EXPECT_NO_THROW(pipeline->bind(cmdBuf));

    // End command buffer
    vkEndCommandBuffer(cmdBuf);

    // Free command buffer
    commandPool.free(cmdBuf);
}
