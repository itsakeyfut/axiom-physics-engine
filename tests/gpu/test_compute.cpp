#include "axiom/gpu/gpu_buffer.hpp"
#include "axiom/gpu/vk_command.hpp"
#include "axiom/gpu/vk_compute_pipeline.hpp"
#include "axiom/gpu/vk_descriptor.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_memory.hpp"
#include "axiom/gpu/vk_shader.hpp"
#include "axiom/gpu/vk_sync.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <vector>

using namespace axiom::gpu;
using namespace axiom::core;

/// Test fixture for compute shader integration tests
/// This fixture sets up the complete Vulkan compute infrastructure
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
        auto memManagerResult = VkMemoryManager::create(context_.get());
        if (memManagerResult.isSuccess()) {
            memManager_ = std::move(memManagerResult.value());
        } else {
            GTEST_SKIP() << "Failed to create memory manager: " << memManagerResult.errorMessage();
        }

        // Load array addition shader
        if (!std::filesystem::exists(arrayAddShaderPath_)) {
            GTEST_SKIP() << "Array addition shader not found: " << arrayAddShaderPath_
                         << " (compile shaders/test/array_add.comp with glslc)";
        }

        auto shaderResult =
            ShaderModule::createFromFile(context_.get(), arrayAddShaderPath_, ShaderStage::Compute);
        if (shaderResult.isFailure()) {
            GTEST_SKIP() << "Failed to load array addition shader: " << shaderResult.errorMessage();
        }
        arrayAddShader_ = std::move(shaderResult.value());
    }

    void TearDown() override {
        arrayAddShader_.reset();
        memManager_.reset();
        context_.reset();
    }

    std::unique_ptr<VkContext> context_;
    std::unique_ptr<VkMemoryManager> memManager_;
    std::unique_ptr<ShaderModule> arrayAddShader_;
    const std::string arrayAddShaderPath_ = "shaders/test/array_add.comp.spv";
};

/// Test basic array addition on GPU
/// This test verifies that:
/// - GPU buffers can be created and uploaded
/// - Compute shaders can be loaded and executed
/// - Results can be downloaded and match CPU computation
TEST_F(ComputeShaderTest, ArrayAddition_BasicTest) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memManager_, nullptr);
    ASSERT_NE(arrayAddShader_, nullptr);

    // Test parameters
    constexpr size_t count = 1024;

    // Prepare test data
    std::vector<float> a(count);
    std::vector<float> b(count);
    std::vector<float> expected(count);

    for (size_t i = 0; i < count; ++i) {
        a[i] = static_cast<float>(i);
        b[i] = static_cast<float>(i * 2);
        expected[i] = a[i] + b[i];
    }

    // Create GPU buffers
    StorageBuffer<float> bufferA(memManager_.get(), count);
    StorageBuffer<float> bufferB(memManager_.get(), count);
    StorageBuffer<float> bufferResult(memManager_.get(), count);

    // Upload input data
    auto uploadA = bufferA.upload(a);
    ASSERT_TRUE(uploadA.isSuccess()) << "Failed to upload buffer A: " << uploadA.errorMessage();

    auto uploadB = bufferB.upload(b);
    ASSERT_TRUE(uploadB.isSuccess()) << "Failed to upload buffer B: " << uploadB.errorMessage();

    // Create descriptor set layout
    DescriptorSetLayoutBuilder layoutBuilder(context_.get());
    auto layoutResult =
        layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();

    ASSERT_TRUE(layoutResult.isSuccess()) << "Failed to create descriptor layout: "
                                           << layoutResult.errorMessage();
    auto layout = std::move(layoutResult.value());

    // Create descriptor pool
    auto poolResult = DescriptorPool::create(
        context_.get(), {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3}}, 1);

    ASSERT_TRUE(poolResult.isSuccess()) << "Failed to create descriptor pool: "
                                         << poolResult.errorMessage();
    auto pool = std::move(poolResult.value());

    // Allocate and bind descriptor set
    auto descriptorSetResult = pool->allocate(*layout);
    ASSERT_TRUE(descriptorSetResult.isSuccess())
        << "Failed to allocate descriptor set: " << descriptorSetResult.errorMessage();

    DescriptorSet desc(context_.get(), descriptorSetResult.value());
    desc.bindBuffer(0, bufferA.getBuffer(), 0, bufferA.getSize() * sizeof(float));
    desc.bindBuffer(1, bufferB.getBuffer(), 0, bufferB.getSize() * sizeof(float));
    desc.bindBuffer(2, bufferResult.getBuffer(), 0, bufferResult.getSize() * sizeof(float));
    desc.update();

    // Create compute pipeline
    ComputePipelineBuilder pipelineBuilder(context_.get());
    auto pipelineResult = pipelineBuilder.setShader(*arrayAddShader_)
                              .setDescriptorSetLayout(*layout)
                              .setPushConstantRange({0, sizeof(uint32_t)})
                              .build();

    ASSERT_TRUE(pipelineResult.isSuccess())
        << "Failed to create compute pipeline: " << pipelineResult.errorMessage();
    auto pipeline = std::move(pipelineResult.value());

    // Create command pool and buffer
    CommandPool cmdPool(context_.get(), context_->getComputeQueueFamily(),
                        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VkCommandBuffer cmdBuf = cmdPool.allocate();
    ASSERT_NE(cmdBuf, VK_NULL_HANDLE);

    // Record commands
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkResult vkResult = vkBeginCommandBuffer(cmdBuf, &beginInfo);
    ASSERT_EQ(vkResult, VK_SUCCESS) << "Failed to begin command buffer";

    // Bind pipeline and descriptor set
    pipeline->bind(cmdBuf);
    VkDescriptorSet descSet = desc.get();
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->getLayout(), 0, 1,
                            &descSet, 0, nullptr);

    // Push constants (element count)
    uint32_t pushCount = static_cast<uint32_t>(count);
    vkCmdPushConstants(cmdBuf, pipeline->getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(pushCount), &pushCount);

    // Dispatch compute work
    uint32_t groupCount = (static_cast<uint32_t>(count) + 255) / 256;
    pipeline->dispatch(cmdBuf, groupCount, 1, 1);

    vkResult = vkEndCommandBuffer(cmdBuf);
    ASSERT_EQ(vkResult, VK_SUCCESS) << "Failed to end command buffer";

    // Submit and wait for completion
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    Fence fence(context_.get());
    VkQueue computeQueue = context_->getComputeQueue();
    vkResult = vkQueueSubmit(computeQueue, 1, &submitInfo, fence.get());
    ASSERT_EQ(vkResult, VK_SUCCESS) << "Failed to submit command buffer";

    auto waitResult = fence.wait();
    ASSERT_TRUE(waitResult.isSuccess()) << "Failed to wait for fence: " << waitResult.errorMessage();

    // Download and verify results
    std::vector<float> result(count);
    auto downloadResult = bufferResult.download(result);
    ASSERT_TRUE(downloadResult.isSuccess())
        << "Failed to download results: " << downloadResult.errorMessage();

    // Verify all elements match expected values
    for (size_t i = 0; i < count; ++i) {
        EXPECT_FLOAT_EQ(result[i], expected[i]) << "Mismatch at index " << i;
    }
}

/// Test array addition with larger dataset
/// This verifies that the compute shader scales correctly with multiple workgroups
TEST_F(ComputeShaderTest, ArrayAddition_LargeDataset) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memManager_, nullptr);
    ASSERT_NE(arrayAddShader_, nullptr);

    // Test with 100K elements (requires ~391 workgroups)
    constexpr size_t count = 100000;

    // Prepare test data
    std::vector<float> a(count);
    std::vector<float> b(count);

    for (size_t i = 0; i < count; ++i) {
        a[i] = static_cast<float>(i) * 0.5f;
        b[i] = static_cast<float>(i) * 1.5f;
    }

    // Create and upload buffers
    StorageBuffer<float> bufferA(memManager_.get(), count);
    StorageBuffer<float> bufferB(memManager_.get(), count);
    StorageBuffer<float> bufferResult(memManager_.get(), count);

    ASSERT_TRUE(bufferA.upload(a).isSuccess());
    ASSERT_TRUE(bufferB.upload(b).isSuccess());

    // Setup descriptors
    DescriptorSetLayoutBuilder layoutBuilder(context_.get());
    auto layout = std::move(layoutBuilder
                                .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            VK_SHADER_STAGE_COMPUTE_BIT)
                                .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            VK_SHADER_STAGE_COMPUTE_BIT)
                                .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            VK_SHADER_STAGE_COMPUTE_BIT)
                                .build()
                                .value());

    auto pool = std::move(
        DescriptorPool::create(context_.get(), {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3}}, 1)
            .value());

    DescriptorSet desc(context_.get(), pool->allocate(*layout).value());
    desc.bindBuffer(0, bufferA.getBuffer(), 0, bufferA.getSize() * sizeof(float));
    desc.bindBuffer(1, bufferB.getBuffer(), 0, bufferB.getSize() * sizeof(float));
    desc.bindBuffer(2, bufferResult.getBuffer(), 0, bufferResult.getSize() * sizeof(float));
    desc.update();

    // Create pipeline
    ComputePipelineBuilder pipelineBuilder(context_.get());
    auto pipeline = std::move(pipelineBuilder.setShader(*arrayAddShader_)
                                  .setDescriptorSetLayout(*layout)
                                  .setPushConstantRange({0, sizeof(uint32_t)})
                                  .build()
                                  .value());

    // Execute compute shader
    CommandPool cmdPool(context_.get(), context_->getComputeQueueFamily(),
                        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VkCommandBuffer cmdBuf = cmdPool.allocate();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuf, &beginInfo);

    pipeline->bind(cmdBuf);
    VkDescriptorSet descSet2 = desc.get();
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->getLayout(), 0, 1,
                            &descSet2, 0, nullptr);

    uint32_t pushCount = static_cast<uint32_t>(count);
    vkCmdPushConstants(cmdBuf, pipeline->getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(pushCount), &pushCount);

    uint32_t groupCount = (pushCount + 255) / 256;
    pipeline->dispatch(cmdBuf, groupCount, 1, 1);

    vkEndCommandBuffer(cmdBuf);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    Fence fence(context_.get());
    vkQueueSubmit(context_->getComputeQueue(), 1, &submitInfo, fence.get());
    ASSERT_TRUE(fence.wait().isSuccess());

    // Verify results (spot check for performance)
    std::vector<float> result(count);
    ASSERT_TRUE(bufferResult.download(result).isSuccess());

    // Check first, middle, and last elements
    EXPECT_FLOAT_EQ(result[0], a[0] + b[0]);
    EXPECT_FLOAT_EQ(result[count / 2], a[count / 2] + b[count / 2]);
    EXPECT_FLOAT_EQ(result[count - 1], a[count - 1] + b[count - 1]);
}

/// Test array addition with non-multiple of workgroup size
/// This verifies that the bounds check in the shader works correctly
TEST_F(ComputeShaderTest, ArrayAddition_NonAlignedSize) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memManager_, nullptr);
    ASSERT_NE(arrayAddShader_, nullptr);

    // Test with 1000 elements (not a multiple of 256)
    constexpr size_t count = 1000;

    std::vector<float> a(count);
    std::vector<float> b(count);
    std::vector<float> expected(count);

    for (size_t i = 0; i < count; ++i) {
        a[i] = static_cast<float>(i) + 0.5f;
        b[i] = static_cast<float>(i) - 0.5f;
        expected[i] = a[i] + b[i];
    }

    // Create and upload buffers
    StorageBuffer<float> bufferA(memManager_.get(), count);
    StorageBuffer<float> bufferB(memManager_.get(), count);
    StorageBuffer<float> bufferResult(memManager_.get(), count);

    ASSERT_TRUE(bufferA.upload(a).isSuccess());
    ASSERT_TRUE(bufferB.upload(b).isSuccess());

    // Setup descriptors
    DescriptorSetLayoutBuilder layoutBuilder(context_.get());
    auto layout = std::move(layoutBuilder
                                .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            VK_SHADER_STAGE_COMPUTE_BIT)
                                .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            VK_SHADER_STAGE_COMPUTE_BIT)
                                .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            VK_SHADER_STAGE_COMPUTE_BIT)
                                .build()
                                .value());

    auto pool = std::move(
        DescriptorPool::create(context_.get(), {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3}}, 1)
            .value());

    DescriptorSet desc(context_.get(), pool->allocate(*layout).value());
    desc.bindBuffer(0, bufferA.getBuffer(), 0, bufferA.getSize() * sizeof(float));
    desc.bindBuffer(1, bufferB.getBuffer(), 0, bufferB.getSize() * sizeof(float));
    desc.bindBuffer(2, bufferResult.getBuffer(), 0, bufferResult.getSize() * sizeof(float));
    desc.update();

    // Create pipeline
    ComputePipelineBuilder pipelineBuilder(context_.get());
    auto pipeline = std::move(pipelineBuilder.setShader(*arrayAddShader_)
                                  .setDescriptorSetLayout(*layout)
                                  .setPushConstantRange({0, sizeof(uint32_t)})
                                  .build()
                                  .value());

    // Execute
    CommandPool cmdPool(context_.get(), context_->getComputeQueueFamily(),
                        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VkCommandBuffer cmdBuf = cmdPool.allocate();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuf, &beginInfo);

    pipeline->bind(cmdBuf);
    VkDescriptorSet descSet3 = desc.get();
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->getLayout(), 0, 1,
                            &descSet3, 0, nullptr);

    uint32_t pushCount = static_cast<uint32_t>(count);
    vkCmdPushConstants(cmdBuf, pipeline->getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(pushCount), &pushCount);

    uint32_t groupCount = (pushCount + 255) / 256;  // This will be 4 workgroups
    pipeline->dispatch(cmdBuf, groupCount, 1, 1);

    vkEndCommandBuffer(cmdBuf);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    Fence fence(context_.get());
    vkQueueSubmit(context_->getComputeQueue(), 1, &submitInfo, fence.get());
    ASSERT_TRUE(fence.wait().isSuccess());

    // Verify all results
    std::vector<float> result(count);
    ASSERT_TRUE(bufferResult.download(result).isSuccess());

    for (size_t i = 0; i < count; ++i) {
        EXPECT_FLOAT_EQ(result[i], expected[i]) << "Mismatch at index " << i;
    }
}
