/// Example demonstrating Vulkan command buffer management
/// This example shows how to use CommandPool, CommandBuffer, and OneTimeCommand
/// for recording and executing GPU commands.

#include <axiom/gpu/vk_command.hpp>
#include <axiom/gpu/vk_instance.hpp>
#include <cstdio>

using namespace axiom::gpu;
using namespace axiom::core;

int main() {
    printf("=== Axiom Command Buffer Example ===\n\n");

    // Step 1: Create Vulkan context
    printf("1. Creating Vulkan context...\n");
    auto contextResult = VkContext::create();
    if (contextResult.isFailure()) {
        fprintf(stderr, "Failed to create Vulkan context: %s\n", contextResult.errorMessage());
        return 1;
    }
    auto context = std::move(contextResult.value());
    printf("   Context created successfully!\n\n");

    // Step 2: Create command pool
    printf("2. Creating command pool for compute operations...\n");
    CommandPool pool(context.get(), context->getComputeQueueFamily());
    printf("   Command pool created!\n\n");

    // Step 3: Manual command buffer usage
    printf("3. Using manual command buffer:\n");
    {
        VkCommandBuffer cmdBuf = pool.allocate();
        CommandBuffer cmd(context.get(), cmdBuf, context->getComputeQueueFamily());

        // Begin recording
        auto beginResult = cmd.begin();
        if (beginResult.isSuccess()) {
            printf("   - Recording started\n");

            // Add a simple pipeline barrier (no-op for demonstration)
            VkMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr,
                                 0, nullptr);

            printf("   - Commands recorded\n");

            // End recording
            cmd.end();
            printf("   - Recording ended\n");

            // Submit and wait
            auto submitResult = cmd.submitAndWait(context->getComputeQueue());
            if (submitResult.isSuccess()) {
                printf("   - Command submitted and executed successfully!\n");
            }
        }

        pool.free(cmdBuf);
    }
    printf("\n");

    // Step 4: One-time command usage (RAII style)
    printf("4. Using one-time command (RAII):\n");
    {
        OneTimeCommand cmd(context.get(), context->getTransferQueue(),
                           context->getTransferQueueFamily());

        printf("   - One-time command created and recording started\n");

        // Record a transfer barrier
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             1, &barrier, 0, nullptr, 0, nullptr);

        printf("   - Commands recorded\n");
        printf("   - Command will auto-submit on scope exit...\n");
    }
    printf("   - Command submitted and executed successfully!\n\n");

    // Step 5: Batch allocation
    printf("5. Batch command buffer allocation:\n");
    {
        auto cmdBufs = pool.allocateMultiple(5);
        printf("   - Allocated %zu command buffers\n", cmdBufs.size());

        // Use them...
        for (size_t i = 0; i < cmdBufs.size(); i++) {
            CommandBuffer cmd(context.get(), cmdBufs[i], context->getComputeQueueFamily());
            cmd.begin();
            // Record commands...
            cmd.end();
        }
        printf("   - Recorded commands in all buffers\n");

        // Free all at once
        pool.free(cmdBufs);
        printf("   - Freed all buffers\n");
    }
    printf("\n");

    // Step 6: Pool reset
    printf("6. Resetting command pool:\n");
    {
        auto cmdBufs = pool.allocateMultiple(3);
        printf("   - Allocated 3 command buffers\n");

        // Reset pool (invalidates all allocated buffers)
        pool.reset();
        printf("   - Pool reset (all buffers invalidated)\n");

        // Allocate new buffers
        VkCommandBuffer newBuf = pool.allocate();
        printf("   - Allocated new buffer after reset\n");
        pool.free(newBuf);
    }
    printf("\n");

    printf("=== All command buffer operations completed successfully! ===\n");

    return 0;
}
