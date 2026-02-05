#include "axiom/gpu/vk_command.hpp"
#include "axiom/gpu/vk_compute_pipeline.hpp"
#include "axiom/gpu/vk_descriptor.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_memory.hpp"
#include "axiom/gpu/vk_shader.hpp"
#include "axiom/gpu/vk_sync.hpp"

#include <gtest/gtest.h>

#include <filesystem>

using namespace axiom::gpu;
using namespace axiom::core;

// Test fixture for ComputePipeline tests
class VkComputePipelineTest : public ::testing::Test {
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

        // Load test shader
        if (!std::filesystem::exists(testShaderPath_)) {
            GTEST_SKIP() << "Test shader not found: " << testShaderPath_
                         << " (compile shaders/test/simple.comp with slangc)";
        }

        auto shaderResult = ShaderModule::createFromFile(context_.get(), testShaderPath_,
                                                         ShaderStage::Compute);
        if (shaderResult.isFailure()) {
            GTEST_SKIP() << "Failed to load test shader: " << shaderResult.errorMessage();
        }
        shader_ = std::move(shaderResult.value());
    }

    void TearDown() override {
        shader_.reset();
        context_.reset();
    }

    std::unique_ptr<VkContext> context_;
    std::unique_ptr<ShaderModule> shader_;
    const std::string testShaderPath_ = "shaders/test/simple.comp.spv";
};

// Test basic compute pipeline creation
TEST_F(VkComputePipelineTest, CreateBasicPipeline) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(shader_, nullptr);

    ComputePipelineBuilder builder(context_.get());
    auto result = builder.setShader(*shader_).build();

    ASSERT_TRUE(result.isSuccess()) << "Failed: " << result.errorMessage();

    auto pipeline = std::move(result.value());
    EXPECT_NE(pipeline->get(), VK_NULL_HANDLE);
    EXPECT_NE(pipeline->getLayout(), VK_NULL_HANDLE);
}

// Test pipeline creation with descriptor set layout
TEST_F(VkComputePipelineTest, CreatePipelineWithDescriptorLayout) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(shader_, nullptr);

    // Create descriptor set layout
    DescriptorSetLayoutBuilder layoutBuilder(context_.get());
    auto layoutResult =
        layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();

    ASSERT_TRUE(layoutResult.isSuccess());
    auto layout = std::move(layoutResult.value());

    // Create pipeline with descriptor layout
    ComputePipelineBuilder builder(context_.get());
    auto result = builder.setShader(*shader_).setDescriptorSetLayout(*layout).build();

    ASSERT_TRUE(result.isSuccess());
    auto pipeline = std::move(result.value());
    EXPECT_NE(pipeline->get(), VK_NULL_HANDLE);
    EXPECT_NE(pipeline->getLayout(), VK_NULL_HANDLE);
}

// Test pipeline creation with multiple descriptor set layouts
TEST_F(VkComputePipelineTest, CreatePipelineWithMultipleDescriptorLayouts) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(shader_, nullptr);

    // Create first descriptor set layout
    DescriptorSetLayoutBuilder layoutBuilder1(context_.get());
    auto layoutResult1 = layoutBuilder1
                             .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                         VK_SHADER_STAGE_COMPUTE_BIT)
                             .build();
    ASSERT_TRUE(layoutResult1.isSuccess());
    auto layout1 = std::move(layoutResult1.value());

    // Create second descriptor set layout
    DescriptorSetLayoutBuilder layoutBuilder2(context_.get());
    auto layoutResult2 = layoutBuilder2
                             .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                         VK_SHADER_STAGE_COMPUTE_BIT)
                             .build();
    ASSERT_TRUE(layoutResult2.isSuccess());
    auto layout2 = std::move(layoutResult2.value());

    // Create pipeline with multiple layouts
    ComputePipelineBuilder builder(context_.get());
    auto result = builder.setShader(*shader_)
                      .addDescriptorSetLayout(*layout1)
                      .addDescriptorSetLayout(*layout2)
                      .build();

    ASSERT_TRUE(result.isSuccess());
    auto pipeline = std::move(result.value());
    EXPECT_NE(pipeline->get(), VK_NULL_HANDLE);
}

// Test pipeline creation with push constants
TEST_F(VkComputePipelineTest, CreatePipelineWithPushConstants) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(shader_, nullptr);

    ComputePipelineBuilder builder(context_.get());
    auto result = builder.setShader(*shader_).setPushConstantRange({0, 128}).build();

    ASSERT_TRUE(result.isSuccess());
    auto pipeline = std::move(result.value());
    EXPECT_NE(pipeline->get(), VK_NULL_HANDLE);
}

// Test pipeline creation with specialization constants
TEST_F(VkComputePipelineTest, CreatePipelineWithSpecializationConstants) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(shader_, nullptr);

    struct SpecData {
        uint32_t workGroupSize = 512;
        uint32_t iterations = 100;
    } specData;

    ComputePipelineBuilder builder(context_.get());
    auto result =
        builder.setShader(*shader_)
            .addSpecializationConstant({0, offsetof(SpecData, workGroupSize), sizeof(uint32_t)})
            .addSpecializationConstant({1, offsetof(SpecData, iterations), sizeof(uint32_t)})
            .setSpecializationData(&specData, sizeof(specData))
            .build();

    ASSERT_TRUE(result.isSuccess());
    auto pipeline = std::move(result.value());
    EXPECT_NE(pipeline->get(), VK_NULL_HANDLE);
}

// Test pipeline creation without shader (should fail)
TEST_F(VkComputePipelineTest, CreatePipelineWithoutShader_Fails) {
    ASSERT_NE(context_, nullptr);

    ComputePipelineBuilder builder(context_.get());
    auto result = builder.build();

    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test pipeline creation with null context (should fail)
TEST(VkComputePipelineStaticTest, CreatePipelineWithNullContext_Fails) {
    ComputePipelineBuilder builder(nullptr);
    auto result = builder.build();

    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test pipeline bind operation
TEST_F(VkComputePipelineTest, BindPipeline) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(shader_, nullptr);

    // Create pipeline
    ComputePipelineBuilder builder(context_.get());
    auto pipelineResult = builder.setShader(*shader_).build();
    ASSERT_TRUE(pipelineResult.isSuccess());
    auto pipeline = std::move(pipelineResult.value());

    // Create command buffer
    CommandPool pool(context_.get(), context_->getComputeQueueFamily(),
                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VkCommandBuffer cmd = pool.allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Bind pipeline (should not crash)
    pipeline->bind(cmd);

    vkEndCommandBuffer(cmd);

    // Test passes if no crash occurred
    SUCCEED();
}

// Test pipeline dispatch operation
TEST_F(VkComputePipelineTest, DispatchCompute) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(shader_, nullptr);

    // Create pipeline
    ComputePipelineBuilder builder(context_.get());
    auto pipelineResult = builder.setShader(*shader_).build();
    ASSERT_TRUE(pipelineResult.isSuccess());
    auto pipeline = std::move(pipelineResult.value());

    // Create command buffer
    CommandPool pool(context_.get(), context_->getComputeQueueFamily(),
                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VkCommandBuffer cmd = pool.allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    // Record commands
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    pipeline->bind(cmd);
    pipeline->dispatch(cmd, 1, 1, 1);

    vkEndCommandBuffer(cmd);

    // Test passes if no crash occurred
    SUCCEED();
}

// Test pipeline dispatch with multiple dimensions
TEST_F(VkComputePipelineTest, DispatchComputeMultiDimensional) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(shader_, nullptr);

    // Create pipeline
    ComputePipelineBuilder builder(context_.get());
    auto pipelineResult = builder.setShader(*shader_).build();
    ASSERT_TRUE(pipelineResult.isSuccess());
    auto pipeline = std::move(pipelineResult.value());

    // Create command buffer
    CommandPool pool(context_.get(), context_->getComputeQueueFamily(),
                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VkCommandBuffer cmd = pool.allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    // Record commands
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    pipeline->bind(cmd);
    pipeline->dispatch(cmd, 64, 32, 16);

    vkEndCommandBuffer(cmd);

    SUCCEED();
}

// ============================================================================
// PipelineCache Tests
// ============================================================================

class VkPipelineCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create Vulkan context
        auto contextResult = VkContext::create();
        if (contextResult.isSuccess()) {
            context_ = std::move(contextResult.value());
        } else {
            GTEST_SKIP() << "Vulkan not available: " << contextResult.errorMessage();
        }
    }

    void TearDown() override {
        context_.reset();
        // Clean up test cache file
        if (std::filesystem::exists(testCachePath_)) {
            std::filesystem::remove(testCachePath_);
        }
    }

    std::unique_ptr<VkContext> context_;
    const std::string testCachePath_ = "test_pipeline_cache.bin";
};

// Test pipeline cache creation
TEST_F(VkPipelineCacheTest, CreateCache) {
    ASSERT_NE(context_, nullptr);

    auto result = PipelineCache::create(context_.get());
    ASSERT_TRUE(result.isSuccess()) << "Failed: " << result.errorMessage();

    auto cache = std::move(result.value());
    EXPECT_NE(cache->get(), VK_NULL_HANDLE);
}

// Test pipeline cache creation with null context
TEST(VkPipelineCacheStaticTest, CreateCacheWithNullContext_Fails) {
    auto result = PipelineCache::create(nullptr);
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test pipeline cache save (empty cache)
TEST_F(VkPipelineCacheTest, SaveEmptyCache) {
    ASSERT_NE(context_, nullptr);

    auto cacheResult = PipelineCache::create(context_.get());
    ASSERT_TRUE(cacheResult.isSuccess());
    auto cache = std::move(cacheResult.value());

    // Save empty cache (should succeed, might not create file)
    auto result = cache->save(testCachePath_);
    EXPECT_TRUE(result.isSuccess());
}

// Test pipeline cache load from non-existent file (should succeed)
TEST_F(VkPipelineCacheTest, LoadNonExistentFile) {
    ASSERT_NE(context_, nullptr);

    auto cacheResult = PipelineCache::create(context_.get());
    ASSERT_TRUE(cacheResult.isSuccess());
    auto cache = std::move(cacheResult.value());

    // Load from non-existent file (should succeed, just log info)
    auto result = cache->load("nonexistent_cache.bin");
    EXPECT_TRUE(result.isSuccess());
}

// Test pipeline creation with cache
TEST_F(VkPipelineCacheTest, CreatePipelineWithCache) {
    ASSERT_NE(context_, nullptr);

    // Skip if test shader doesn't exist
    const std::string testShaderPath = "shaders/test/simple.comp.spv";
    if (!std::filesystem::exists(testShaderPath)) {
        GTEST_SKIP() << "Test shader not found: " << testShaderPath;
    }

    // Load shader
    auto shaderResult = ShaderModule::createFromFile(context_.get(), testShaderPath,
                                                     ShaderStage::Compute);
    if (shaderResult.isFailure()) {
        GTEST_SKIP() << "Failed to load test shader: " << shaderResult.errorMessage();
    }
    auto shader = std::move(shaderResult.value());

    // Create pipeline cache
    auto cacheResult = PipelineCache::create(context_.get());
    ASSERT_TRUE(cacheResult.isSuccess());
    auto cache = std::move(cacheResult.value());

    // Create pipeline with cache
    ComputePipelineBuilder builder(context_.get());
    auto pipelineResult = builder.setShader(*shader).setPipelineCache(cache->get()).build();

    ASSERT_TRUE(pipelineResult.isSuccess());
    auto pipeline = std::move(pipelineResult.value());
    EXPECT_NE(pipeline->get(), VK_NULL_HANDLE);
}

// Integration test: Full pipeline workflow
TEST_F(VkComputePipelineTest, IntegrationTest_FullWorkflow) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(shader_, nullptr);

    // Create descriptor set layout
    DescriptorSetLayoutBuilder layoutBuilder(context_.get());
    auto layoutResult =
        layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();
    ASSERT_TRUE(layoutResult.isSuccess());
    auto layout = std::move(layoutResult.value());

    // Create pipeline cache
    auto cacheResult = PipelineCache::create(context_.get());
    ASSERT_TRUE(cacheResult.isSuccess());
    auto cache = std::move(cacheResult.value());

    // Setup push constants
    struct PushConstants {
        uint32_t count;
        float scale;
    };

    // Create pipeline
    ComputePipelineBuilder builder(context_.get());
    auto pipelineResult = builder.setShader(*shader_)
                              .setDescriptorSetLayout(*layout)
                              .setPushConstantRange({0, sizeof(PushConstants)})
                              .setPipelineCache(cache->get())
                              .build();

    ASSERT_TRUE(pipelineResult.isSuccess());
    auto pipeline = std::move(pipelineResult.value());

    // Create command pool and buffer
    CommandPool pool(context_.get(), context_->getComputeQueueFamily(),
                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VkCommandBuffer cmd = pool.allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    // Record commands
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Bind pipeline
    pipeline->bind(cmd);

    // Push constants
    PushConstants constants{1024, 2.0f};
    vkCmdPushConstants(cmd, pipeline->getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(constants), &constants);

    // Dispatch
    uint32_t groupCount = (constants.count + 255) / 256;
    pipeline->dispatch(cmd, groupCount, 1, 1);

    vkEndCommandBuffer(cmd);

    SUCCEED();
}
