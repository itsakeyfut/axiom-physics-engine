#include "axiom/gpu/vk_command.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_memory.hpp"
#include "axiom/gpu/vk_sync.hpp"

#include <gtest/gtest.h>

using namespace axiom::gpu;
using namespace axiom::core;

// Test fixture that creates a Vulkan context for all tests
class VkSyncTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto result = VkContext::create();
        if (result.isSuccess()) {
            context_ = std::move(result.value());
        } else {
            GTEST_SKIP() << "Vulkan not available, skipping GPU tests";
        }
    }

    void TearDown() override {
        // Wait for device to be idle before cleanup
        if (context_) {
            vkDeviceWaitIdle(context_->getDevice());
        }
        context_.reset();
    }

    std::unique_ptr<VkContext> context_;
};

// ============================================================================
// Fence Tests
// ============================================================================

TEST_F(VkSyncTest, FenceCreateAndDestroy) {
    Fence fence(context_.get(), false);
    EXPECT_NE(fence.get(), VK_NULL_HANDLE);
}

TEST_F(VkSyncTest, FenceCreateSignaled) {
    Fence fence(context_.get(), true);
    EXPECT_TRUE(fence.isSignaled());
}

TEST_F(VkSyncTest, FenceCreateUnsignaled) {
    Fence fence(context_.get(), false);
    EXPECT_FALSE(fence.isSignaled());
}

TEST_F(VkSyncTest, FenceWaitImmediate) {
    Fence fence(context_.get(), true);
    auto result = fence.wait(0);
    EXPECT_TRUE(result.isSuccess());
}

TEST_F(VkSyncTest, FenceWaitTimeout) {
    Fence fence(context_.get(), false);
    // Should timeout immediately since fence is not signaled
    auto result = fence.wait(0);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.errorCode(), ErrorCode::GPU_TIMEOUT);
}

TEST_F(VkSyncTest, FenceReset) {
    Fence fence(context_.get(), true);
    EXPECT_TRUE(fence.isSignaled());

    auto result = fence.reset();
    EXPECT_TRUE(result.isSuccess());
    EXPECT_FALSE(fence.isSignaled());
}

TEST_F(VkSyncTest, FenceMove) {
    Fence fence1(context_.get(), false);
    VkFence handle = fence1.get();

    Fence fence2(std::move(fence1));
    EXPECT_EQ(fence2.get(), handle);
    EXPECT_EQ(fence1.get(), VK_NULL_HANDLE);
}

TEST_F(VkSyncTest, FenceMoveAssignment) {
    Fence fence1(context_.get(), false);
    Fence fence2(context_.get(), true);

    VkFence handle1 = fence1.get();
    fence2 = std::move(fence1);

    EXPECT_EQ(fence2.get(), handle1);
    EXPECT_EQ(fence1.get(), VK_NULL_HANDLE);
}

TEST_F(VkSyncTest, FenceWithCommandBuffer) {
    // Create command pool and buffer
    CommandPool pool(context_.get(), context_->getComputeQueueFamily());
    VkCommandBuffer cmdBuf = pool.allocate();
    ASSERT_NE(cmdBuf, VK_NULL_HANDLE);

    // Begin and end command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuf, &beginInfo);
    vkEndCommandBuffer(cmdBuf);

    // Create fence and submit
    Fence fence(context_.get(), false);
    EXPECT_FALSE(fence.isSignaled());

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    VkResult result = vkQueueSubmit(context_->getComputeQueue(), 1, &submitInfo, fence.get());
    ASSERT_EQ(result, VK_SUCCESS);

    // Wait for fence
    auto waitResult = fence.wait();
    EXPECT_TRUE(waitResult.isSuccess());
    EXPECT_TRUE(fence.isSignaled());
}

// ============================================================================
// FencePool Tests
// ============================================================================

TEST_F(VkSyncTest, FencePoolCreateAndDestroy) {
    FencePool pool(context_.get());
    EXPECT_EQ(pool.getTotalCount(), 0);
    EXPECT_EQ(pool.getAvailableCount(), 0);
}

TEST_F(VkSyncTest, FencePoolAcquire) {
    FencePool pool(context_.get());

    Fence* fence = pool.acquire();
    ASSERT_NE(fence, nullptr);
    EXPECT_NE(fence->get(), VK_NULL_HANDLE);
    EXPECT_EQ(pool.getTotalCount(), 1);
    EXPECT_EQ(pool.getAvailableCount(), 0);
}

TEST_F(VkSyncTest, FencePoolRelease) {
    FencePool pool(context_.get());

    Fence* fence = pool.acquire();
    ASSERT_NE(fence, nullptr);

    pool.release(fence);
    EXPECT_EQ(pool.getTotalCount(), 1);
    EXPECT_EQ(pool.getAvailableCount(), 1);
}

TEST_F(VkSyncTest, FencePoolReuse) {
    FencePool pool(context_.get());

    // Acquire first fence
    Fence* fence1 = pool.acquire();
    ASSERT_NE(fence1, nullptr);
    VkFence handle1 = fence1->get();

    // Release it
    pool.release(fence1);

    // Acquire again - should reuse the same fence
    Fence* fence2 = pool.acquire();
    ASSERT_NE(fence2, nullptr);
    EXPECT_EQ(fence2->get(), handle1);
    EXPECT_EQ(pool.getTotalCount(), 1);
}

TEST_F(VkSyncTest, FencePoolMultipleAcquire) {
    FencePool pool(context_.get());

    Fence* fence1 = pool.acquire();
    Fence* fence2 = pool.acquire();
    Fence* fence3 = pool.acquire();

    EXPECT_NE(fence1, nullptr);
    EXPECT_NE(fence2, nullptr);
    EXPECT_NE(fence3, nullptr);
    EXPECT_EQ(pool.getTotalCount(), 3);
    EXPECT_EQ(pool.getAvailableCount(), 0);

    pool.release(fence1);
    pool.release(fence2);
    pool.release(fence3);

    EXPECT_EQ(pool.getTotalCount(), 3);
    EXPECT_EQ(pool.getAvailableCount(), 3);
}

// ============================================================================
// Semaphore Tests
// ============================================================================

TEST_F(VkSyncTest, SemaphoreCreateAndDestroy) {
    Semaphore semaphore(context_.get());
    EXPECT_NE(semaphore.get(), VK_NULL_HANDLE);
}

TEST_F(VkSyncTest, SemaphoreMove) {
    Semaphore sem1(context_.get());
    VkSemaphore handle = sem1.get();

    Semaphore sem2(std::move(sem1));
    EXPECT_EQ(sem2.get(), handle);
    EXPECT_EQ(sem1.get(), VK_NULL_HANDLE);
}

TEST_F(VkSyncTest, SemaphoreMoveAssignment) {
    Semaphore sem1(context_.get());
    Semaphore sem2(context_.get());

    VkSemaphore handle1 = sem1.get();
    sem2 = std::move(sem1);

    EXPECT_EQ(sem2.get(), handle1);
    EXPECT_EQ(sem1.get(), VK_NULL_HANDLE);
}

TEST_F(VkSyncTest, SemaphoreWithCommandBuffers) {
    // Create two command pools and buffers
    CommandPool pool(context_.get(), context_->getComputeQueueFamily());
    VkCommandBuffer cmdBuf1 = pool.allocate();
    VkCommandBuffer cmdBuf2 = pool.allocate();

    // Record empty command buffers
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmdBuf1, &beginInfo);
    vkEndCommandBuffer(cmdBuf1);

    vkBeginCommandBuffer(cmdBuf2, &beginInfo);
    vkEndCommandBuffer(cmdBuf2);

    // Create semaphore for synchronization
    Semaphore semaphore(context_.get());
    Fence fence(context_.get(), false);

    // First submit signals semaphore
    VkSemaphore semaphoreHandle = semaphore.get();
    VkSubmitInfo submit1{};
    submit1.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit1.commandBufferCount = 1;
    submit1.pCommandBuffers = &cmdBuf1;
    submit1.signalSemaphoreCount = 1;
    submit1.pSignalSemaphores = &semaphoreHandle;

    VkResult result = vkQueueSubmit(context_->getComputeQueue(), 1, &submit1, VK_NULL_HANDLE);
    ASSERT_EQ(result, VK_SUCCESS);

    // Second submit waits on semaphore
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    VkSubmitInfo submit2{};
    submit2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit2.commandBufferCount = 1;
    submit2.pCommandBuffers = &cmdBuf2;
    submit2.waitSemaphoreCount = 1;
    submit2.pWaitSemaphores = &semaphoreHandle;
    submit2.pWaitDstStageMask = &waitStage;

    result = vkQueueSubmit(context_->getComputeQueue(), 1, &submit2, fence.get());
    ASSERT_EQ(result, VK_SUCCESS);

    // Wait for completion
    auto waitResult = fence.wait();
    EXPECT_TRUE(waitResult.isSuccess());
}

// ============================================================================
// TimelineSemaphore Tests
// ============================================================================

TEST_F(VkSyncTest, TimelineSemaphoreCreateAndDestroy) {
    TimelineSemaphore semaphore(context_.get(), 0);
    EXPECT_NE(semaphore.get(), VK_NULL_HANDLE);
}

TEST_F(VkSyncTest, TimelineSemaphoreInitialValue) {
    TimelineSemaphore semaphore(context_.get(), 42);
    EXPECT_EQ(semaphore.getValue(), 42);
}

TEST_F(VkSyncTest, TimelineSemaphoreSignal) {
    TimelineSemaphore semaphore(context_.get(), 0);
    EXPECT_EQ(semaphore.getValue(), 0);

    auto result = semaphore.signal(10);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(semaphore.getValue(), 10);
}

TEST_F(VkSyncTest, TimelineSemaphoreWaitImmediate) {
    TimelineSemaphore semaphore(context_.get(), 0);
    semaphore.signal(5);

    // Wait for value that's already reached
    auto result = semaphore.wait(5, 0);
    EXPECT_TRUE(result.isSuccess());
}

TEST_F(VkSyncTest, TimelineSemaphoreWaitTimeout) {
    TimelineSemaphore semaphore(context_.get(), 0);

    // Wait for value that hasn't been reached yet
    auto result = semaphore.wait(10, 0);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.errorCode(), ErrorCode::GPU_TIMEOUT);
}

TEST_F(VkSyncTest, TimelineSemaphoreIncremental) {
    TimelineSemaphore semaphore(context_.get(), 0);

    semaphore.signal(1);
    EXPECT_EQ(semaphore.getValue(), 1);

    semaphore.signal(2);
    EXPECT_EQ(semaphore.getValue(), 2);

    semaphore.signal(10);
    EXPECT_EQ(semaphore.getValue(), 10);
}

TEST_F(VkSyncTest, TimelineSemaphoreMove) {
    TimelineSemaphore sem1(context_.get(), 5);
    VkSemaphore handle = sem1.get();

    TimelineSemaphore sem2(std::move(sem1));
    EXPECT_EQ(sem2.get(), handle);
    EXPECT_EQ(sem1.get(), VK_NULL_HANDLE);
}

// ============================================================================
// Pipeline Barrier Tests
// ============================================================================

TEST_F(VkSyncTest, MemoryBarrier) {
    CommandPool pool(context_.get(), context_->getComputeQueueFamily());
    VkCommandBuffer cmdBuf = pool.allocate();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmdBuf, &beginInfo);

    // Insert memory barrier - should not crash
    memoryBarrier(cmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                  VK_ACCESS_SHADER_READ_BIT);

    vkEndCommandBuffer(cmdBuf);

    // Submit and wait
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    Fence fence(context_.get(), false);
    VkResult result = vkQueueSubmit(context_->getComputeQueue(), 1, &submitInfo, fence.get());
    ASSERT_EQ(result, VK_SUCCESS);

    auto waitResult = fence.wait();
    EXPECT_TRUE(waitResult.isSuccess());
}

TEST_F(VkSyncTest, BufferBarrier) {
    // Create a buffer for testing using VkMemoryManager
    auto memManagerResult = VkMemoryManager::create(context_.get());
    if (!memManagerResult.isSuccess()) {
        GTEST_SKIP() << "Failed to create memory manager";
    }

    auto& memManager = memManagerResult.value();

    VkMemoryManager::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 1024;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.memoryUsage = MemoryUsage::GpuOnly;

    auto bufferResult = memManager->createBuffer(bufferInfo);
    if (!bufferResult.isSuccess()) {
        GTEST_SKIP() << "Failed to create buffer";
    }

    VkMemoryManager::Buffer buffer = bufferResult.value();

    CommandPool pool(context_.get(), context_->getComputeQueueFamily());
    VkCommandBuffer cmdBuf = pool.allocate();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmdBuf, &beginInfo);

    // Insert buffer barrier
    bufferBarrier(cmdBuf, buffer.buffer, 0, VK_WHOLE_SIZE, VK_PIPELINE_STAGE_TRANSFER_BIT,
                  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                  VK_ACCESS_SHADER_READ_BIT);

    vkEndCommandBuffer(cmdBuf);

    // Submit and wait
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    Fence fence(context_.get(), false);
    VkResult result = vkQueueSubmit(context_->getComputeQueue(), 1, &submitInfo, fence.get());
    ASSERT_EQ(result, VK_SUCCESS);

    auto waitResult = fence.wait();
    EXPECT_TRUE(waitResult.isSuccess());

    // Cleanup
    memManager->destroyBuffer(buffer);
}

TEST_F(VkSyncTest, ImageBarrier) {
    // Create an image for testing
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent = {256, 256, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage image;
    VkResult result = vkCreateImage(context_->getDevice(), &imageInfo, nullptr, &image);
    if (result != VK_SUCCESS) {
        GTEST_SKIP() << "Failed to create image";
    }

    // Allocate memory for the image (simplified)
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(context_->getDevice(), image, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = 0;  // Simplified

    VkDeviceMemory memory;
    vkAllocateMemory(context_->getDevice(), &allocInfo, nullptr, &memory);
    vkBindImageMemory(context_->getDevice(), image, memory, 0);

    CommandPool pool(context_.get(), context_->getComputeQueueFamily());
    VkCommandBuffer cmdBuf = pool.allocate();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmdBuf, &beginInfo);

    // Insert image barrier - transition layout
    imageBarrier(cmdBuf, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
                 VK_ACCESS_SHADER_WRITE_BIT);

    vkEndCommandBuffer(cmdBuf);

    // Submit and wait
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    Fence fence(context_.get(), false);
    result = vkQueueSubmit(context_->getComputeQueue(), 1, &submitInfo, fence.get());
    ASSERT_EQ(result, VK_SUCCESS);

    auto waitResult = fence.wait();
    EXPECT_TRUE(waitResult.isSuccess());

    // Cleanup
    vkDestroyImage(context_->getDevice(), image, nullptr);
    vkFreeMemory(context_->getDevice(), memory, nullptr);
}
