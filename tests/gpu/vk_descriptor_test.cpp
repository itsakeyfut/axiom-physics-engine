#include "axiom/gpu/vk_descriptor.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_memory.hpp"

#include <gtest/gtest.h>

using namespace axiom::gpu;
using namespace axiom::core;

// Test fixture for descriptor set tests
class VkDescriptorTest : public ::testing::Test {
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
    }

    void TearDown() override {
        // Cleanup happens automatically through destructors
        memManager_.reset();
        context_.reset();
    }

    std::unique_ptr<VkContext> context_;
    std::unique_ptr<VkMemoryManager> memManager_;
};

// ========================================
// DescriptorSetLayout tests
// ========================================

TEST_F(VkDescriptorTest, CreateDescriptorSetLayoutWithBuilder) {
    ASSERT_NE(context_, nullptr);

    DescriptorSetLayoutBuilder builder(context_.get());
    auto result = builder
                      .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                      .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                      .addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                      .build();

    ASSERT_TRUE(result.isSuccess()) << "Failed: " << result.errorMessage();

    auto layout = std::move(result.value());
    EXPECT_NE(layout->get(), VK_NULL_HANDLE);
    EXPECT_EQ(layout->getBindings().size(), 3);
}

TEST_F(VkDescriptorTest, CreateDescriptorSetLayoutDirectly) {
    ASSERT_NE(context_, nullptr);

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.push_back(
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr});
    bindings.push_back(
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr});

    auto result = DescriptorSetLayout::create(context_.get(), bindings);

    ASSERT_TRUE(result.isSuccess()) << "Failed: " << result.errorMessage();

    auto layout = std::move(result.value());
    EXPECT_NE(layout->get(), VK_NULL_HANDLE);
    EXPECT_EQ(layout->getBindings().size(), 2);
}

TEST_F(VkDescriptorTest, CreateDescriptorSetLayoutWithNullContext) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.push_back(
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr});

    auto result = DescriptorSetLayout::create(nullptr, bindings);

    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

TEST_F(VkDescriptorTest, CreateDescriptorSetLayoutWithNoBindings) {
    ASSERT_NE(context_, nullptr);

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    auto result = DescriptorSetLayout::create(context_.get(), bindings);

    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

TEST_F(VkDescriptorTest, BuilderWithNoBindings) {
    ASSERT_NE(context_, nullptr);

    DescriptorSetLayoutBuilder builder(context_.get());
    auto result = builder.build();

    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

TEST_F(VkDescriptorTest, BuilderWithMultipleDescriptorTypes) {
    ASSERT_NE(context_, nullptr);

    DescriptorSetLayoutBuilder builder(context_.get());
    auto result = builder
                      .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                      .addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
                      .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  VK_SHADER_STAGE_FRAGMENT_BIT)
                      .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
                      .build();

    ASSERT_TRUE(result.isSuccess()) << "Failed: " << result.errorMessage();

    auto layout = std::move(result.value());
    EXPECT_NE(layout->get(), VK_NULL_HANDLE);
    EXPECT_EQ(layout->getBindings().size(), 4);
}

TEST_F(VkDescriptorTest, BuilderWithArrayBinding) {
    ASSERT_NE(context_, nullptr);

    DescriptorSetLayoutBuilder builder(context_.get());
    auto result = builder
                      .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT,
                                  10)  // Array of 10
                      .build();

    ASSERT_TRUE(result.isSuccess()) << "Failed: " << result.errorMessage();

    auto layout = std::move(result.value());
    EXPECT_NE(layout->get(), VK_NULL_HANDLE);
    EXPECT_EQ(layout->getBindings().size(), 1);
    EXPECT_EQ(layout->getBindings()[0].descriptorCount, 10);
}

// ========================================
// DescriptorPool tests
// ========================================

TEST_F(VkDescriptorTest, CreateDescriptorPool) {
    ASSERT_NE(context_, nullptr);

    std::vector<DescriptorPool::PoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
                                                       {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 50}};

    auto result = DescriptorPool::create(context_.get(), poolSizes, 100);

    ASSERT_TRUE(result.isSuccess()) << "Failed: " << result.errorMessage();

    auto pool = std::move(result.value());
    EXPECT_NE(pool->get(), VK_NULL_HANDLE);
    EXPECT_EQ(pool->getMaxSets(), 100);
}

TEST_F(VkDescriptorTest, CreateDescriptorPoolWithNullContext) {
    std::vector<DescriptorPool::PoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10}};

    auto result = DescriptorPool::create(nullptr, poolSizes, 10);

    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

TEST_F(VkDescriptorTest, CreateDescriptorPoolWithNoPoolSizes) {
    ASSERT_NE(context_, nullptr);

    std::vector<DescriptorPool::PoolSize> poolSizes;
    auto result = DescriptorPool::create(context_.get(), poolSizes, 10);

    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

TEST_F(VkDescriptorTest, CreateDescriptorPoolWithZeroMaxSets) {
    ASSERT_NE(context_, nullptr);

    std::vector<DescriptorPool::PoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10}};

    auto result = DescriptorPool::create(context_.get(), poolSizes, 0);

    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

TEST_F(VkDescriptorTest, AllocateDescriptorSet) {
    ASSERT_NE(context_, nullptr);

    // Create layout
    DescriptorSetLayoutBuilder builder(context_.get());
    auto layoutResult = builder
                            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        VK_SHADER_STAGE_COMPUTE_BIT)
                            .build();
    ASSERT_TRUE(layoutResult.isSuccess());
    auto layout = std::move(layoutResult.value());

    // Create pool
    std::vector<DescriptorPool::PoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10}};
    auto poolResult = DescriptorPool::create(context_.get(), poolSizes, 10);
    ASSERT_TRUE(poolResult.isSuccess());
    auto pool = std::move(poolResult.value());

    // Allocate descriptor set
    auto descSetResult = pool->allocate(*layout);
    ASSERT_TRUE(descSetResult.isSuccess()) << "Failed: " << descSetResult.errorMessage();

    VkDescriptorSet descSet = descSetResult.value();
    EXPECT_NE(descSet, VK_NULL_HANDLE);
}

TEST_F(VkDescriptorTest, AllocateMultipleDescriptorSets) {
    ASSERT_NE(context_, nullptr);

    // Create layout
    DescriptorSetLayoutBuilder builder(context_.get());
    auto layoutResult = builder
                            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        VK_SHADER_STAGE_COMPUTE_BIT)
                            .build();
    ASSERT_TRUE(layoutResult.isSuccess());
    auto layout = std::move(layoutResult.value());

    // Create pool
    std::vector<DescriptorPool::PoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 50}};
    auto poolResult = DescriptorPool::create(context_.get(), poolSizes, 10);
    ASSERT_TRUE(poolResult.isSuccess());
    auto pool = std::move(poolResult.value());

    // Allocate multiple descriptor sets
    auto descSetsResult = pool->allocateMultiple(*layout, 5);
    ASSERT_TRUE(descSetsResult.isSuccess()) << "Failed: " << descSetsResult.errorMessage();

    auto descSets = descSetsResult.value();
    EXPECT_EQ(descSets.size(), 5);
    for (const auto& descSet : descSets) {
        EXPECT_NE(descSet, VK_NULL_HANDLE);
    }
}

TEST_F(VkDescriptorTest, AllocateMultipleWithZeroCount) {
    ASSERT_NE(context_, nullptr);

    // Create layout
    DescriptorSetLayoutBuilder builder(context_.get());
    auto layoutResult = builder
                            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        VK_SHADER_STAGE_COMPUTE_BIT)
                            .build();
    ASSERT_TRUE(layoutResult.isSuccess());
    auto layout = std::move(layoutResult.value());

    // Create pool
    std::vector<DescriptorPool::PoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10}};
    auto poolResult = DescriptorPool::create(context_.get(), poolSizes, 10);
    ASSERT_TRUE(poolResult.isSuccess());
    auto pool = std::move(poolResult.value());

    // Try to allocate 0 sets
    auto descSetsResult = pool->allocateMultiple(*layout, 0);
    EXPECT_TRUE(descSetsResult.isFailure());
    EXPECT_EQ(descSetsResult.errorCode(), ErrorCode::InvalidParameter);
}

TEST_F(VkDescriptorTest, ResetDescriptorPool) {
    ASSERT_NE(context_, nullptr);

    // Create layout
    DescriptorSetLayoutBuilder builder(context_.get());
    auto layoutResult = builder
                            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        VK_SHADER_STAGE_COMPUTE_BIT)
                            .build();
    ASSERT_TRUE(layoutResult.isSuccess());
    auto layout = std::move(layoutResult.value());

    // Create pool
    std::vector<DescriptorPool::PoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10}};
    auto poolResult = DescriptorPool::create(context_.get(), poolSizes, 10);
    ASSERT_TRUE(poolResult.isSuccess());
    auto pool = std::move(poolResult.value());

    // Allocate some descriptor sets
    auto descSet1Result = pool->allocate(*layout);
    ASSERT_TRUE(descSet1Result.isSuccess());
    auto descSet2Result = pool->allocate(*layout);
    ASSERT_TRUE(descSet2Result.isSuccess());

    // Reset pool (should not crash)
    pool->reset();

    // After reset, we should be able to allocate again
    auto descSet3Result = pool->allocate(*layout);
    EXPECT_TRUE(descSet3Result.isSuccess());
}

// ========================================
// DescriptorSet tests
// ========================================

TEST_F(VkDescriptorTest, BindBufferToDescriptorSet) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memManager_, nullptr);

    // Create layout
    DescriptorSetLayoutBuilder builder(context_.get());
    auto layoutResult = builder
                            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        VK_SHADER_STAGE_COMPUTE_BIT)
                            .build();
    ASSERT_TRUE(layoutResult.isSuccess());
    auto layout = std::move(layoutResult.value());

    // Create pool
    std::vector<DescriptorPool::PoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10}};
    auto poolResult = DescriptorPool::create(context_.get(), poolSizes, 10);
    ASSERT_TRUE(poolResult.isSuccess());
    auto pool = std::move(poolResult.value());

    // Allocate descriptor set
    auto descSetResult = pool->allocate(*layout);
    ASSERT_TRUE(descSetResult.isSuccess());

    // Create buffer
    VkMemoryManager::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 1024;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferInfo.memoryUsage = MemoryUsage::GpuOnly;

    auto bufferResult = memManager_->createBuffer(bufferInfo);
    ASSERT_TRUE(bufferResult.isSuccess()) << "Failed: " << bufferResult.errorMessage();
    auto buffer = bufferResult.value();

    // Bind buffer to descriptor set
    DescriptorSet desc(context_.get(), descSetResult.value());
    desc.bindBuffer(0, buffer.buffer, 0, 1024);

    // Update (should not crash)
    desc.update();

    // Cleanup
    memManager_->destroyBuffer(buffer);
}

TEST_F(VkDescriptorTest, BindMultipleBuffersToDescriptorSet) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memManager_, nullptr);

    // Create layout with multiple bindings
    DescriptorSetLayoutBuilder builder(context_.get());
    auto layoutResult =
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .build();
    ASSERT_TRUE(layoutResult.isSuccess());
    auto layout = std::move(layoutResult.value());

    // Create pool
    std::vector<DescriptorPool::PoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 20},
                                                       {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10}};
    auto poolResult = DescriptorPool::create(context_.get(), poolSizes, 10);
    ASSERT_TRUE(poolResult.isSuccess());
    auto pool = std::move(poolResult.value());

    // Allocate descriptor set
    auto descSetResult = pool->allocate(*layout);
    ASSERT_TRUE(descSetResult.isSuccess());

    // Create buffers
    VkMemoryManager::BufferCreateInfo storageBufferInfo{};
    storageBufferInfo.size = 2048;
    storageBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    storageBufferInfo.memoryUsage = MemoryUsage::GpuOnly;

    auto buffer1Result = memManager_->createBuffer(storageBufferInfo);
    ASSERT_TRUE(buffer1Result.isSuccess());
    auto buffer1 = buffer1Result.value();

    auto buffer2Result = memManager_->createBuffer(storageBufferInfo);
    ASSERT_TRUE(buffer2Result.isSuccess());
    auto buffer2 = buffer2Result.value();

    VkMemoryManager::BufferCreateInfo uniformBufferInfo{};
    uniformBufferInfo.size = 256;
    uniformBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    uniformBufferInfo.memoryUsage = MemoryUsage::CpuToGpu;

    auto buffer3Result = memManager_->createBuffer(uniformBufferInfo);
    ASSERT_TRUE(buffer3Result.isSuccess());
    auto buffer3 = buffer3Result.value();

    // Bind buffers to descriptor set
    DescriptorSet desc(context_.get(), descSetResult.value());
    desc.bindBuffer(0, buffer1.buffer, 0, 2048, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    desc.bindBuffer(1, buffer2.buffer, 0, 2048, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    desc.bindBuffer(2, buffer3.buffer, 0, 256, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // Update (should not crash)
    desc.update();

    // Cleanup
    memManager_->destroyBuffer(buffer1);
    memManager_->destroyBuffer(buffer2);
    memManager_->destroyBuffer(buffer3);
}

TEST_F(VkDescriptorTest, UpdateDescriptorSetWithNoBindings) {
    ASSERT_NE(context_, nullptr);

    // Create layout
    DescriptorSetLayoutBuilder builder(context_.get());
    auto layoutResult = builder
                            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        VK_SHADER_STAGE_COMPUTE_BIT)
                            .build();
    ASSERT_TRUE(layoutResult.isSuccess());
    auto layout = std::move(layoutResult.value());

    // Create pool
    std::vector<DescriptorPool::PoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10}};
    auto poolResult = DescriptorPool::create(context_.get(), poolSizes, 10);
    ASSERT_TRUE(poolResult.isSuccess());
    auto pool = std::move(poolResult.value());

    // Allocate descriptor set
    auto descSetResult = pool->allocate(*layout);
    ASSERT_TRUE(descSetResult.isSuccess());

    // Update without binding anything (should log warning but not crash)
    DescriptorSet desc(context_.get(), descSetResult.value());
    desc.update();
}

// ========================================
// Integration test
// ========================================

TEST_F(VkDescriptorTest, CompleteWorkflow) {
    ASSERT_NE(context_, nullptr);
    ASSERT_NE(memManager_, nullptr);

    // Step 1: Create descriptor set layout
    DescriptorSetLayoutBuilder layoutBuilder(context_.get());
    auto layoutResult = layoutBuilder
                            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        VK_SHADER_STAGE_COMPUTE_BIT)  // Input
                            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        VK_SHADER_STAGE_COMPUTE_BIT)  // Output
                            .addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                        VK_SHADER_STAGE_COMPUTE_BIT)  // Params
                            .build();
    ASSERT_TRUE(layoutResult.isSuccess()) << "Failed: " << layoutResult.errorMessage();
    auto layout = std::move(layoutResult.value());

    // Step 2: Create descriptor pool
    std::vector<DescriptorPool::PoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
                                                       {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 50}};
    auto poolResult = DescriptorPool::create(context_.get(), poolSizes, 100);
    ASSERT_TRUE(poolResult.isSuccess()) << "Failed: " << poolResult.errorMessage();
    auto pool = std::move(poolResult.value());

    // Step 3: Allocate descriptor set
    auto descSetResult = pool->allocate(*layout);
    ASSERT_TRUE(descSetResult.isSuccess()) << "Failed: " << descSetResult.errorMessage();

    // Step 4: Create buffers
    VkMemoryManager::BufferCreateInfo storageInfo{};
    storageInfo.size = 4096;
    storageInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    storageInfo.memoryUsage = MemoryUsage::GpuOnly;

    auto inputBufferResult = memManager_->createBuffer(storageInfo);
    ASSERT_TRUE(inputBufferResult.isSuccess());
    auto inputBuffer = inputBufferResult.value();

    auto outputBufferResult = memManager_->createBuffer(storageInfo);
    ASSERT_TRUE(outputBufferResult.isSuccess());
    auto outputBuffer = outputBufferResult.value();

    VkMemoryManager::BufferCreateInfo uniformInfo{};
    uniformInfo.size = 256;
    uniformInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    uniformInfo.memoryUsage = MemoryUsage::CpuToGpu;

    auto uniformBufferResult = memManager_->createBuffer(uniformInfo);
    ASSERT_TRUE(uniformBufferResult.isSuccess());
    auto uniformBuffer = uniformBufferResult.value();

    // Step 5: Bind resources to descriptor set
    DescriptorSet desc(context_.get(), descSetResult.value());
    desc.bindBuffer(0, inputBuffer.buffer, 0, 4096, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    desc.bindBuffer(1, outputBuffer.buffer, 0, 4096, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    desc.bindBuffer(2, uniformBuffer.buffer, 0, 256, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // Step 6: Update descriptor set
    desc.update();

    // Step 7: Verify descriptor set handle is valid
    EXPECT_NE(desc.get(), VK_NULL_HANDLE);

    // Cleanup
    memManager_->destroyBuffer(inputBuffer);
    memManager_->destroyBuffer(outputBuffer);
    memManager_->destroyBuffer(uniformBuffer);
}
