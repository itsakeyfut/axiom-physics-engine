// Integration test for Vulkan compute shader infrastructure
// Tests the complete compute pipeline with a simple array addition shader

#include "axiom/gpu/gpu_buffer.hpp"
#include "axiom/gpu/vk_command.hpp"
#include "axiom/gpu/vk_compute_pipeline.hpp"
#include "axiom/gpu/vk_descriptor.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_memory.hpp"
#include "axiom/gpu/vk_shader.hpp"
#include "axiom/gpu/vk_sync.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <vector>

using namespace axiom::gpu;
using namespace axiom::core;

// Test fixture for compute shader integration tests
class ComputeShaderTest : public ::testing::Test {
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

        // Create memory manager
        auto managerResult = VkMemoryManager::create(context_.get());
        if (managerResult.isSuccess()) {
            memManager_ = std::move(managerResult.value());
        } else {
            GTEST_SKIP() << "Failed to create memory manager: " << managerResult.errorMessage();
        }

        // Check if test shader exists
        if (!std::filesystem::exists(testShaderPath_)) {
            GTEST_SKIP() << "Test shader not found: " << testShaderPath_
                         << " (compile shaders/test/array_add.slang with slangc)";
        }
    }

    void TearDown() override {
        memManager_.reset();
        context_.reset();
    }

    std::unique_ptr<VkContext> context_;
    std::unique_ptr<VkMemoryManager> memManager_;
    const std::string testShaderPath_ = "shaders/test/array_add.comp.spv";
};

// Test basic array addition on GPU
TEST_F(ComputeShaderTest, ArrayAddition) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memManager_, nullptr);

    // Test configuration
    constexpr size_t elementCount = 1024;

    // Prepare test data
    std::vector<float> inputA(elementCount);
    std::vector<float> inputB(elementCount);
    std::vector<float> expected(elementCount);

    for (size_t i = 0; i < elementCount; ++i) {
        inputA[i] = static_cast<float>(i);
        inputB[i] = static_cast<float>(i * 2);
        expected[i] = inputA[i] + inputB[i];
    }

    // Create GPU buffers
    StorageBuffer<float> bufferA(memManager_.get(), elementCount);
    StorageBuffer<float> bufferB(memManager_.get(), elementCount);
    StorageBuffer<float> bufferOutput(memManager_.get(), elementCount);

    // Upload input data to GPU
    auto uploadResultA = bufferA.upload(inputA);
    ASSERT_TRUE(uploadResultA.isSuccess())
        << "Failed to upload buffer A: " << uploadResultA.errorMessage();

    auto uploadResultB = bufferB.upload(inputB);
    ASSERT_TRUE(uploadResultB.isSuccess())
        << "Failed to upload buffer B: " << uploadResultB.errorMessage();

    // Load compute shader
    auto shaderResult = ShaderModule::createFromFile(context_.get(), testShaderPath_,
                                                     ShaderStage::Compute);
    ASSERT_TRUE(shaderResult.isSuccess())
        << "Failed to load shader: " << shaderResult.errorMessage();
    auto shader = std::move(shaderResult.value());

    // Create descriptor set layout
    DescriptorSetLayoutBuilder layoutBuilder(context_.get());
    auto layoutResult =
        layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();
    ASSERT_TRUE(layoutResult.isSuccess())
        << "Failed to create descriptor set layout: " << layoutResult.errorMessage();
    auto layout = std::move(layoutResult.value());

    // Create descriptor pool
    std::vector<DescriptorPool::PoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3}};
    auto poolResult = DescriptorPool::create(context_.get(), poolSizes, 1);
    ASSERT_TRUE(poolResult.isSuccess())
        << "Failed to create descriptor pool: " << poolResult.errorMessage();
    auto pool = std::move(poolResult.value());

    // Allocate descriptor set
    auto descriptorSetResult = pool->allocate(*layout);
    ASSERT_TRUE(descriptorSetResult.isSuccess())
        << "Failed to allocate descriptor set: " << descriptorSetResult.errorMessage();

    // Bind resources to descriptor set
    DescriptorSet descriptorSet(context_.get(), descriptorSetResult.value());
    descriptorSet.bindBuffer(0, bufferA.getBuffer(), 0, bufferA.getSize());
    descriptorSet.bindBuffer(1, bufferB.getBuffer(), 0, bufferB.getSize());
    descriptorSet.bindBuffer(2, bufferOutput.getBuffer(), 0, bufferOutput.getSize());
    descriptorSet.update();

    // Create compute pipeline with push constants
    struct PushConstants {
        uint32_t count;
    };

    ComputePipelineBuilder pipelineBuilder(context_.get());
    auto pipelineResult = pipelineBuilder.setShader(*shader)
                              .setDescriptorSetLayout(*layout)
                              .setPushConstantRange({0, sizeof(PushConstants)})
                              .build();
    ASSERT_TRUE(pipelineResult.isSuccess())
        << "Failed to create compute pipeline: " << pipelineResult.errorMessage();
    auto pipeline = std::move(pipelineResult.value());

    // Create command pool and allocate command buffer
    CommandPool cmdPool(context_.get(), context_->getComputeQueueFamily(),
                        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VkCommandBuffer cmd = cmdPool.allocate();
    ASSERT_NE(cmd, VK_NULL_HANDLE);

    // Record compute commands
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkResult result = vkBeginCommandBuffer(cmd, &beginInfo);
    ASSERT_EQ(result, VK_SUCCESS) << "Failed to begin command buffer";

    // Bind pipeline
    pipeline->bind(cmd);

    // Bind descriptor set
    VkDescriptorSet descSet = descriptorSet.get();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->getLayout(), 0, 1,
                            &descSet, 0, nullptr);

    // Push constants
    PushConstants pushConstants{static_cast<uint32_t>(elementCount)};
    vkCmdPushConstants(cmd, pipeline->getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(pushConstants), &pushConstants);

    // Dispatch compute shader (256 threads per workgroup)
    uint32_t groupCount = (static_cast<uint32_t>(elementCount) + 255) / 256;
    pipeline->dispatch(cmd, groupCount, 1, 1);

    result = vkEndCommandBuffer(cmd);
    ASSERT_EQ(result, VK_SUCCESS) << "Failed to end command buffer";

    // Submit command buffer and wait for completion
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkQueue computeQueue = context_->getComputeQueue();
    result = vkQueueSubmit(computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    ASSERT_EQ(result, VK_SUCCESS) << "Failed to submit command buffer";

    // Wait for GPU to finish
    result = vkQueueWaitIdle(computeQueue);
    ASSERT_EQ(result, VK_SUCCESS) << "Failed to wait for queue idle";

    // Download results from GPU
    std::vector<float> output;
    auto downloadResult = bufferOutput.download(output);
    ASSERT_TRUE(downloadResult.isSuccess())
        << "Failed to download results: " << downloadResult.errorMessage();

    // Verify results against CPU calculation
    ASSERT_EQ(output.size(), elementCount);
    for (size_t i = 0; i < elementCount; ++i) {
        EXPECT_FLOAT_EQ(output[i], expected[i]) << "Mismatch at index " << i << ": GPU computed "
                                                << output[i] << " but expected " << expected[i];
    }
}

// Test compute shader with different array sizes
TEST_F(ComputeShaderTest, ArrayAddition_VariousSizes) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memManager_, nullptr);

    // Test with various sizes including edge cases
    std::vector<size_t> testSizes = {1, 16, 127, 256, 257, 512, 1000, 2048};

    for (size_t elementCount : testSizes) {
        // Prepare test data
        std::vector<float> inputA(elementCount);
        std::vector<float> inputB(elementCount);

        for (size_t i = 0; i < elementCount; ++i) {
            inputA[i] = static_cast<float>(i) * 0.5f;
            inputB[i] = static_cast<float>(i) * 1.5f;
        }

        // Create GPU buffers
        StorageBuffer<float> bufferA(memManager_.get(), elementCount);
        StorageBuffer<float> bufferB(memManager_.get(), elementCount);
        StorageBuffer<float> bufferOutput(memManager_.get(), elementCount);

        // Upload data
        bufferA.upload(inputA);
        bufferB.upload(inputB);

        // Load shader
        auto shaderResult = ShaderModule::createFromFile(context_.get(), testShaderPath_,
                                                         ShaderStage::Compute);
        ASSERT_TRUE(shaderResult.isSuccess());
        auto shader = std::move(shaderResult.value());

        // Create descriptor set layout
        DescriptorSetLayoutBuilder layoutBuilder(context_.get());
        auto layoutResult =
            layoutBuilder
                .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                .build();
        ASSERT_TRUE(layoutResult.isSuccess());
        auto layout = std::move(layoutResult.value());

        // Create descriptor pool
        std::vector<DescriptorPool::PoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3}};
        auto poolResult = DescriptorPool::create(context_.get(), poolSizes, 1);
        ASSERT_TRUE(poolResult.isSuccess());
        auto pool = std::move(poolResult.value());

        // Allocate and update descriptor set
        auto descriptorSetResult = pool->allocate(*layout);
        ASSERT_TRUE(descriptorSetResult.isSuccess());

        DescriptorSet descriptorSet(context_.get(), descriptorSetResult.value());
        descriptorSet.bindBuffer(0, bufferA.getBuffer(), 0, bufferA.getSize());
        descriptorSet.bindBuffer(1, bufferB.getBuffer(), 0, bufferB.getSize());
        descriptorSet.bindBuffer(2, bufferOutput.getBuffer(), 0, bufferOutput.getSize());
        descriptorSet.update();

        // Create pipeline
        struct PushConstants {
            uint32_t count;
        };

        ComputePipelineBuilder pipelineBuilder(context_.get());
        auto pipelineResult = pipelineBuilder.setShader(*shader)
                                  .setDescriptorSetLayout(*layout)
                                  .setPushConstantRange({0, sizeof(PushConstants)})
                                  .build();
        ASSERT_TRUE(pipelineResult.isSuccess());
        auto pipeline = std::move(pipelineResult.value());

        // Execute compute
        CommandPool cmdPool(context_.get(), context_->getComputeQueueFamily(),
                            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        VkCommandBuffer cmd = cmdPool.allocate();

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        pipeline->bind(cmd);
        VkDescriptorSet descSet = descriptorSet.get();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->getLayout(), 0, 1,
                                &descSet, 0, nullptr);

        PushConstants pushConstants{static_cast<uint32_t>(elementCount)};
        vkCmdPushConstants(cmd, pipeline->getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                           sizeof(pushConstants), &pushConstants);

        uint32_t groupCount = (static_cast<uint32_t>(elementCount) + 255) / 256;
        pipeline->dispatch(cmd, groupCount, 1, 1);

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        VkQueue computeQueue = context_->getComputeQueue();
        vkQueueSubmit(computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(computeQueue);

        // Verify results
        std::vector<float> output;
        bufferOutput.download(output);

        ASSERT_EQ(output.size(), elementCount) << "Size mismatch for test size " << elementCount;
        for (size_t i = 0; i < elementCount; ++i) {
            float expected = inputA[i] + inputB[i];
            EXPECT_FLOAT_EQ(output[i], expected)
                << "Mismatch at index " << i << " for test size " << elementCount;
        }
    }
}

// Test compute shader with floating point precision
TEST_F(ComputeShaderTest, ArrayAddition_FloatPrecision) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memManager_, nullptr);

    constexpr size_t elementCount = 512;

    // Test with various floating point values including edge cases
    std::vector<float> inputA(elementCount);
    std::vector<float> inputB(elementCount);

    for (size_t i = 0; i < elementCount; ++i) {
        inputA[i] = static_cast<float>(i) * 0.123456789f;
        inputB[i] = static_cast<float>(i) * 0.987654321f;
    }

    // Add some edge cases
    inputA[0] = 0.0f;
    inputB[0] = 0.0f;
    inputA[1] = -1.0f;
    inputB[1] = 1.0f;
    inputA[2] = 1e-6f;
    inputB[2] = 1e6f;

    // Create buffers
    StorageBuffer<float> bufferA(memManager_.get(), elementCount);
    StorageBuffer<float> bufferB(memManager_.get(), elementCount);
    StorageBuffer<float> bufferOutput(memManager_.get(), elementCount);

    bufferA.upload(inputA);
    bufferB.upload(inputB);

    // Load shader
    auto shaderResult = ShaderModule::createFromFile(context_.get(), testShaderPath_,
                                                     ShaderStage::Compute);
    ASSERT_TRUE(shaderResult.isSuccess());
    auto shader = std::move(shaderResult.value());

    // Create descriptor set layout
    DescriptorSetLayoutBuilder layoutBuilder(context_.get());
    auto layoutResult =
        layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();
    ASSERT_TRUE(layoutResult.isSuccess());
    auto layout = std::move(layoutResult.value());

    // Create descriptor pool
    std::vector<DescriptorPool::PoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3}};
    auto poolResult = DescriptorPool::create(context_.get(), poolSizes, 1);
    ASSERT_TRUE(poolResult.isSuccess());
    auto pool = std::move(poolResult.value());

    // Allocate and update descriptor set
    auto descriptorSetResult = pool->allocate(*layout);
    ASSERT_TRUE(descriptorSetResult.isSuccess());

    DescriptorSet descriptorSet(context_.get(), descriptorSetResult.value());
    descriptorSet.bindBuffer(0, bufferA.getBuffer(), 0, bufferA.getSize());
    descriptorSet.bindBuffer(1, bufferB.getBuffer(), 0, bufferB.getSize());
    descriptorSet.bindBuffer(2, bufferOutput.getBuffer(), 0, bufferOutput.getSize());
    descriptorSet.update();

    // Create pipeline
    struct PushConstants {
        uint32_t count;
    };

    ComputePipelineBuilder pipelineBuilder(context_.get());
    auto pipelineResult = pipelineBuilder.setShader(*shader)
                              .setDescriptorSetLayout(*layout)
                              .setPushConstantRange({0, sizeof(PushConstants)})
                              .build();
    ASSERT_TRUE(pipelineResult.isSuccess());
    auto pipeline = std::move(pipelineResult.value());

    // Execute compute
    CommandPool cmdPool(context_.get(), context_->getComputeQueueFamily(),
                        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VkCommandBuffer cmd = cmdPool.allocate();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    pipeline->bind(cmd);
    VkDescriptorSet descSet = descriptorSet.get();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->getLayout(), 0, 1,
                            &descSet, 0, nullptr);

    PushConstants pushConstants{static_cast<uint32_t>(elementCount)};
    vkCmdPushConstants(cmd, pipeline->getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(pushConstants), &pushConstants);

    uint32_t groupCount = (static_cast<uint32_t>(elementCount) + 255) / 256;
    pipeline->dispatch(cmd, groupCount, 1, 1);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkQueue computeQueue = context_->getComputeQueue();
    vkQueueSubmit(computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(computeQueue);

    // Download and verify results
    std::vector<float> output;
    bufferOutput.download(output);

    ASSERT_EQ(output.size(), elementCount);
    for (size_t i = 0; i < elementCount; ++i) {
        float expected = inputA[i] + inputB[i];
        EXPECT_FLOAT_EQ(output[i], expected) << "Float precision mismatch at index " << i;
    }
}
