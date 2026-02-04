#include "axiom/gpu/vk_command.hpp"
#include "axiom/gpu/vk_instance.hpp"

#include <gtest/gtest.h>

using namespace axiom::gpu;
using namespace axiom::core;

// Test fixture for command buffer tests
class VkCommandTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create Vulkan context
        auto result = VkContext::create();
        if (result.isSuccess()) {
            context_ = std::move(result.value());
        } else {
            // Vulkan not available (common in CI without GPU)
            GTEST_SKIP() << "Vulkan not available: " << result.errorMessage();
        }
    }

    void TearDown() override { context_.reset(); }

    std::unique_ptr<VkContext> context_;
};

//==============================================================================
// CommandPool Tests
//==============================================================================

TEST_F(VkCommandTest, CommandPoolCreation) {
    ASSERT_NE(context_, nullptr);

    // Create command pool for graphics queue family
    CommandPool pool(context_.get(), context_->getGraphicsQueueFamily());

    EXPECT_NE(pool.get(), VK_NULL_HANDLE);
    EXPECT_EQ(pool.getQueueFamily(), context_->getGraphicsQueueFamily());
}

TEST_F(VkCommandTest, CommandPoolCreationWithFlags) {
    ASSERT_NE(context_, nullptr);

    // Create transient command pool
    CommandPool pool(context_.get(), context_->getComputeQueueFamily(),
                     VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

    EXPECT_NE(pool.get(), VK_NULL_HANDLE);
}

TEST_F(VkCommandTest, CommandPoolAllocateSingle) {
    ASSERT_NE(context_, nullptr);

    CommandPool pool(context_.get(), context_->getGraphicsQueueFamily());
    VkCommandBuffer cmdBuf = pool.allocate();

    EXPECT_NE(cmdBuf, VK_NULL_HANDLE);

    pool.free(cmdBuf);
}

TEST_F(VkCommandTest, CommandPoolAllocateMultiple) {
    ASSERT_NE(context_, nullptr);

    CommandPool pool(context_.get(), context_->getGraphicsQueueFamily());
    auto cmdBufs = pool.allocateMultiple(5);

    EXPECT_EQ(cmdBufs.size(), 5);
    for (auto buf : cmdBufs) {
        EXPECT_NE(buf, VK_NULL_HANDLE);
    }

    pool.free(cmdBufs);
}

TEST_F(VkCommandTest, CommandPoolAllocateSecondary) {
    ASSERT_NE(context_, nullptr);

    CommandPool pool(context_.get(), context_->getGraphicsQueueFamily());
    VkCommandBuffer cmdBuf = pool.allocate(VK_COMMAND_BUFFER_LEVEL_SECONDARY);

    EXPECT_NE(cmdBuf, VK_NULL_HANDLE);

    pool.free(cmdBuf);
}

TEST_F(VkCommandTest, CommandPoolReset) {
    ASSERT_NE(context_, nullptr);

    CommandPool pool(context_.get(), context_->getGraphicsQueueFamily());

    // Allocate some command buffers
    auto cmdBufs = pool.allocateMultiple(3);
    EXPECT_EQ(cmdBufs.size(), 3);

    // Reset pool (invalidates all allocated buffers)
    pool.reset();

    // Should be able to allocate again
    VkCommandBuffer newBuf = pool.allocate();
    EXPECT_NE(newBuf, VK_NULL_HANDLE);

    pool.free(newBuf);
}

TEST_F(VkCommandTest, CommandPoolResetWithReleaseResources) {
    ASSERT_NE(context_, nullptr);

    CommandPool pool(context_.get(), context_->getGraphicsQueueFamily());

    // Allocate buffers
    auto cmdBufs = pool.allocateMultiple(3);

    // Reset and release resources
    pool.reset(true);

    // Should still work
    VkCommandBuffer newBuf = pool.allocate();
    EXPECT_NE(newBuf, VK_NULL_HANDLE);

    pool.free(newBuf);
}

//==============================================================================
// CommandBuffer Tests
//==============================================================================

TEST_F(VkCommandTest, CommandBufferBeginEnd) {
    ASSERT_NE(context_, nullptr);

    CommandPool pool(context_.get(), context_->getGraphicsQueueFamily());
    VkCommandBuffer cmdBuf = pool.allocate();
    ASSERT_NE(cmdBuf, VK_NULL_HANDLE);

    CommandBuffer cmd(context_.get(), cmdBuf, context_->getGraphicsQueueFamily());

    auto beginResult = cmd.begin();
    EXPECT_TRUE(beginResult.isSuccess());

    auto endResult = cmd.end();
    EXPECT_TRUE(endResult.isSuccess());

    pool.free(cmdBuf);
}

TEST_F(VkCommandTest, CommandBufferBeginWithFlags) {
    ASSERT_NE(context_, nullptr);

    CommandPool pool(context_.get(), context_->getGraphicsQueueFamily());
    VkCommandBuffer cmdBuf = pool.allocate();
    ASSERT_NE(cmdBuf, VK_NULL_HANDLE);

    CommandBuffer cmd(context_.get(), cmdBuf, context_->getGraphicsQueueFamily());

    auto result = cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    EXPECT_TRUE(result.isSuccess());

    cmd.end();
    pool.free(cmdBuf);
}

TEST_F(VkCommandTest, CommandBufferSubmitAndWait) {
    ASSERT_NE(context_, nullptr);

    CommandPool pool(context_.get(), context_->getGraphicsQueueFamily());
    VkCommandBuffer cmdBuf = pool.allocate();
    ASSERT_NE(cmdBuf, VK_NULL_HANDLE);

    CommandBuffer cmd(context_.get(), cmdBuf, context_->getGraphicsQueueFamily());

    // Record an empty command buffer (just for testing submission)
    cmd.begin();
    cmd.end();

    // Submit and wait
    auto submitResult = cmd.submitAndWait(context_->getGraphicsQueue());
    EXPECT_TRUE(submitResult.isSuccess());

    pool.free(cmdBuf);
}

TEST_F(VkCommandTest, CommandBufferSubmitWithEmptyInfo) {
    ASSERT_NE(context_, nullptr);

    CommandPool pool(context_.get(), context_->getGraphicsQueueFamily());
    VkCommandBuffer cmdBuf = pool.allocate();
    ASSERT_NE(cmdBuf, VK_NULL_HANDLE);

    CommandBuffer cmd(context_.get(), cmdBuf, context_->getGraphicsQueueFamily());

    // Record empty command buffer
    cmd.begin();
    cmd.end();

    // Submit with default (empty) info
    CommandBuffer::SubmitInfo info;
    auto submitResult = cmd.submit(context_->getGraphicsQueue(), info);
    EXPECT_TRUE(submitResult.isSuccess());

    // Wait for queue to finish
    vkQueueWaitIdle(context_->getGraphicsQueue());

    pool.free(cmdBuf);
}

TEST_F(VkCommandTest, CommandBufferReset) {
    ASSERT_NE(context_, nullptr);

    // Create pool with RESET_COMMAND_BUFFER_BIT flag
    CommandPool pool(context_.get(), context_->getGraphicsQueueFamily(),
                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VkCommandBuffer cmdBuf = pool.allocate();
    ASSERT_NE(cmdBuf, VK_NULL_HANDLE);

    CommandBuffer cmd(context_.get(), cmdBuf, context_->getGraphicsQueueFamily());

    // Record and submit
    cmd.begin();
    cmd.end();
    cmd.submitAndWait(context_->getGraphicsQueue());

    // Reset command buffer
    auto resetResult = cmd.reset();
    EXPECT_TRUE(resetResult.isSuccess());

    // Should be able to record again
    auto beginResult = cmd.begin();
    EXPECT_TRUE(beginResult.isSuccess());
    cmd.end();

    pool.free(cmdBuf);
}

TEST_F(VkCommandTest, CommandBufferMove) {
    ASSERT_NE(context_, nullptr);

    CommandPool pool(context_.get(), context_->getGraphicsQueueFamily());
    VkCommandBuffer cmdBuf = pool.allocate();
    ASSERT_NE(cmdBuf, VK_NULL_HANDLE);

    CommandBuffer cmd1(context_.get(), cmdBuf, context_->getGraphicsQueueFamily());
    VkCommandBuffer handle1 = cmd1.get();

    // Move construction
    CommandBuffer cmd2(std::move(cmd1));
    EXPECT_EQ(cmd2.get(), handle1);
    EXPECT_EQ(cmd1.get(), VK_NULL_HANDLE);  // Moved-from state

    // Move assignment
    CommandBuffer cmd3(context_.get(), cmdBuf, context_->getGraphicsQueueFamily());
    cmd3 = std::move(cmd2);
    EXPECT_EQ(cmd3.get(), handle1);
    EXPECT_EQ(cmd2.get(), VK_NULL_HANDLE);  // Moved-from state

    pool.free(cmdBuf);
}

//==============================================================================
// OneTimeCommand Tests
//==============================================================================

TEST_F(VkCommandTest, OneTimeCommandBasic) {
    ASSERT_NE(context_, nullptr);

    // Create a one-time command
    {
        OneTimeCommand cmd(context_.get(), context_->getTransferQueue(),
                           context_->getTransferQueueFamily());

        EXPECT_NE(cmd.get(), VK_NULL_HANDLE);

        // Can record commands here (though we don't have buffers to transfer yet)
        // The command will be automatically submitted and waited on destruction
    }

    // Command should be complete after scope exit
    // Verify queue is idle
    VkResult result = vkQueueWaitIdle(context_->getTransferQueue());
    EXPECT_EQ(result, VK_SUCCESS);
}

TEST_F(VkCommandTest, OneTimeCommandWithGraphicsQueue) {
    ASSERT_NE(context_, nullptr);

    {
        OneTimeCommand cmd(context_.get(), context_->getGraphicsQueue(),
                           context_->getGraphicsQueueFamily());

        EXPECT_NE(cmd.get(), VK_NULL_HANDLE);

        // Record some barrier or pipeline barrier (safe no-op for testing)
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 1, &barrier, 0, nullptr, 0,
                             nullptr);
    }

    // Verify completion
    VkResult result = vkQueueWaitIdle(context_->getGraphicsQueue());
    EXPECT_EQ(result, VK_SUCCESS);
}

TEST_F(VkCommandTest, OneTimeCommandWithComputeQueue) {
    ASSERT_NE(context_, nullptr);

    {
        OneTimeCommand cmd(context_.get(), context_->getComputeQueue(),
                           context_->getComputeQueueFamily());

        EXPECT_NE(cmd.get(), VK_NULL_HANDLE);

        // Add a pipeline barrier (safe for compute queue)
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0,
                             nullptr);
    }

    // Verify completion
    VkResult result = vkQueueWaitIdle(context_->getComputeQueue());
    EXPECT_EQ(result, VK_SUCCESS);
}

TEST_F(VkCommandTest, OneTimeCommandMultipleInSequence) {
    ASSERT_NE(context_, nullptr);

    // Execute multiple one-time commands in sequence
    for (int i = 0; i < 3; i++) {
        OneTimeCommand cmd(context_.get(), context_->getTransferQueue(),
                           context_->getTransferQueueFamily());

        EXPECT_NE(cmd.get(), VK_NULL_HANDLE);

        // Record a simple barrier
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             1, &barrier, 0, nullptr, 0, nullptr);
    }

    // All commands should be complete
    VkResult result = vkQueueWaitIdle(context_->getTransferQueue());
    EXPECT_EQ(result, VK_SUCCESS);
}

//==============================================================================
// Integration Tests
//==============================================================================

TEST_F(VkCommandTest, MultiplePoolsPerThread) {
    ASSERT_NE(context_, nullptr);

    // Create multiple pools for different queue families
    CommandPool graphicsPool(context_.get(), context_->getGraphicsQueueFamily());
    CommandPool computePool(context_.get(), context_->getComputeQueueFamily());
    CommandPool transferPool(context_.get(), context_->getTransferQueueFamily());

    EXPECT_NE(graphicsPool.get(), VK_NULL_HANDLE);
    EXPECT_NE(computePool.get(), VK_NULL_HANDLE);
    EXPECT_NE(transferPool.get(), VK_NULL_HANDLE);

    // Allocate from each
    VkCommandBuffer gfxBuf = graphicsPool.allocate();
    VkCommandBuffer compBuf = computePool.allocate();
    VkCommandBuffer xferBuf = transferPool.allocate();

    EXPECT_NE(gfxBuf, VK_NULL_HANDLE);
    EXPECT_NE(compBuf, VK_NULL_HANDLE);
    EXPECT_NE(xferBuf, VK_NULL_HANDLE);

    // Free buffers
    graphicsPool.free(gfxBuf);
    computePool.free(compBuf);
    transferPool.free(xferBuf);
}

TEST_F(VkCommandTest, CommandBufferReuse) {
    ASSERT_NE(context_, nullptr);

    CommandPool pool(context_.get(), context_->getGraphicsQueueFamily(),
                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VkCommandBuffer cmdBuf = pool.allocate();
    ASSERT_NE(cmdBuf, VK_NULL_HANDLE);

    CommandBuffer cmd(context_.get(), cmdBuf, context_->getGraphicsQueueFamily());

    // Use the command buffer multiple times
    for (int i = 0; i < 3; i++) {
        cmd.begin();
        // Record commands...
        cmd.end();
        cmd.submitAndWait(context_->getGraphicsQueue());

        // Reset for next iteration
        if (i < 2) {  // Don't reset on last iteration
            auto resetResult = cmd.reset();
            EXPECT_TRUE(resetResult.isSuccess());
        }
    }

    pool.free(cmdBuf);
}
