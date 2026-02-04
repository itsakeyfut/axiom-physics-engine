#include "axiom/gpu/vk_memory.hpp"

#include "axiom/gpu/vk_instance.hpp"

#include <gtest/gtest.h>

using namespace axiom::gpu;
using namespace axiom::core;

// Test fixture for VkMemoryManager tests
class VkMemoryManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create Vulkan context
        auto contextResult = VkContext::create();
        if (contextResult.isSuccess()) {
            context_ = std::move(contextResult.value());

            // Create memory manager
            auto managerResult = VkMemoryManager::create(context_.get());
            if (managerResult.isSuccess()) {
                manager_ = std::move(managerResult.value());
            } else {
                GTEST_SKIP() << "Failed to create memory manager: "
                             << managerResult.errorMessage();
            }
        } else {
            GTEST_SKIP() << "Vulkan not available: " << contextResult.errorMessage()
                         << " (this is expected in CI environments without GPU)";
        }
    }

    void TearDown() override {
        // Cleanup happens automatically through destructors
        manager_.reset();
        context_.reset();
    }

    std::unique_ptr<VkContext> context_;
    std::unique_ptr<VkMemoryManager> manager_;
};

// Test memory manager creation
TEST_F(VkMemoryManagerTest, ManagerCreation) {
    ASSERT_NE(manager_, nullptr);
    EXPECT_NE(manager_->getAllocator(), nullptr);
}

// Test creating a null manager
TEST(VkMemoryManagerStaticTest, CreateWithNullContext) {
    auto result = VkMemoryManager::create(nullptr);
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test buffer creation with GPU-only memory
TEST_F(VkMemoryManagerTest, CreateGpuOnlyBuffer) {
    ASSERT_NE(manager_, nullptr);

    VkMemoryManager::BufferCreateInfo info{.size = 1024,
                                           .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                           .memoryUsage = MemoryUsage::GpuOnly};

    auto result = manager_->createBuffer(info);
    ASSERT_TRUE(result.isSuccess());

    auto buffer = result.value();
    EXPECT_NE(buffer.buffer, VK_NULL_HANDLE);
    EXPECT_NE(buffer.allocation, nullptr);
    EXPECT_EQ(buffer.mappedPtr, nullptr);  // GPU-only buffers are not mapped

    manager_->destroyBuffer(buffer);
    EXPECT_EQ(buffer.buffer, VK_NULL_HANDLE);
    EXPECT_EQ(buffer.allocation, nullptr);
}

// Test buffer creation with CPU-to-GPU memory (staging)
TEST_F(VkMemoryManagerTest, CreateStagingBuffer) {
    ASSERT_NE(manager_, nullptr);

    VkMemoryManager::BufferCreateInfo info{.size = 4096,
                                           .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           .memoryUsage = MemoryUsage::CpuToGpu};

    auto result = manager_->createBuffer(info);
    ASSERT_TRUE(result.isSuccess());

    auto buffer = result.value();
    EXPECT_NE(buffer.buffer, VK_NULL_HANDLE);
    EXPECT_NE(buffer.allocation, nullptr);

    manager_->destroyBuffer(buffer);
}

// Test buffer creation with persistent mapping
TEST_F(VkMemoryManagerTest, CreatePersistentlyMappedBuffer) {
    ASSERT_NE(manager_, nullptr);

    VkMemoryManager::BufferCreateInfo info{.size = 2048,
                                           .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                           .memoryUsage = MemoryUsage::CpuToGpu,
                                           .persistentMapping = true};

    auto result = manager_->createBuffer(info);
    ASSERT_TRUE(result.isSuccess());

    auto buffer = result.value();
    EXPECT_NE(buffer.buffer, VK_NULL_HANDLE);
    EXPECT_NE(buffer.allocation, nullptr);
    EXPECT_NE(buffer.mappedPtr, nullptr);  // Should be persistently mapped

    manager_->destroyBuffer(buffer);
}

// Test memory mapping and unmapping
TEST_F(VkMemoryManagerTest, MapAndUnmapMemory) {
    ASSERT_NE(manager_, nullptr);

    VkMemoryManager::BufferCreateInfo info{.size = 1024,
                                           .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           .memoryUsage = MemoryUsage::CpuToGpu};

    auto bufferResult = manager_->createBuffer(info);
    ASSERT_TRUE(bufferResult.isSuccess());
    auto buffer = bufferResult.value();

    // Map memory
    auto mapResult = manager_->mapMemory(buffer);
    ASSERT_TRUE(mapResult.isSuccess());
    void* mappedPtr = mapResult.value();
    EXPECT_NE(mappedPtr, nullptr);

    // Write some data
    uint32_t testData[] = {1, 2, 3, 4};
    std::memcpy(mappedPtr, testData, sizeof(testData));

    // Unmap memory
    manager_->unmapMemory(buffer);

    manager_->destroyBuffer(buffer);
}

// Test image creation
TEST_F(VkMemoryManagerTest, CreateImage) {
    ASSERT_NE(manager_, nullptr);

    VkMemoryManager::ImageCreateInfo info{
        .extent = {256, 256, 1},
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT};

    auto result = manager_->createImage(info);
    ASSERT_TRUE(result.isSuccess());

    auto image = result.value();
    EXPECT_NE(image.image, VK_NULL_HANDLE);
    EXPECT_NE(image.allocation, nullptr);

    manager_->destroyImage(image);
    EXPECT_EQ(image.image, VK_NULL_HANDLE);
    EXPECT_EQ(image.allocation, nullptr);
}

// Test image creation with multiple mip levels
TEST_F(VkMemoryManagerTest, CreateImageWithMipLevels) {
    ASSERT_NE(manager_, nullptr);

    VkMemoryManager::ImageCreateInfo info{.extent = {512, 512, 1},
                                          .format = VK_FORMAT_R8G8B8A8_UNORM,
                                          .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
                                          .mipLevels = 4};

    auto result = manager_->createImage(info);
    ASSERT_TRUE(result.isSuccess());

    auto image = result.value();
    EXPECT_NE(image.image, VK_NULL_HANDLE);
    EXPECT_NE(image.allocation, nullptr);

    manager_->destroyImage(image);
}

// Test memory statistics
TEST_F(VkMemoryManagerTest, GetMemoryStats) {
    ASSERT_NE(manager_, nullptr);

    // Get initial stats
    auto initialStats = manager_->getStats();

    // Create some buffers
    VkMemoryManager::BufferCreateInfo info{.size = 1024 * 1024,  // 1 MB
                                           .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                           .memoryUsage = MemoryUsage::GpuOnly};

    std::vector<VkMemoryManager::Buffer> buffers;
    for (int i = 0; i < 5; i++) {
        auto result = manager_->createBuffer(info);
        ASSERT_TRUE(result.isSuccess());
        buffers.push_back(result.value());
    }

    // Get stats after allocation
    auto statsAfterAlloc = manager_->getStats();
    EXPECT_GT(statsAfterAlloc.usedBytes, initialStats.usedBytes);
    EXPECT_GT(statsAfterAlloc.allocationCount, initialStats.allocationCount);

    // Cleanup
    for (auto& buffer : buffers) {
        manager_->destroyBuffer(buffer);
    }

    // Get stats after cleanup
    auto statsAfterCleanup = manager_->getStats();
    EXPECT_LE(statsAfterCleanup.usedBytes, statsAfterAlloc.usedBytes);
}

// Test print stats (just verify it doesn't crash)
TEST_F(VkMemoryManagerTest, PrintStats) {
    ASSERT_NE(manager_, nullptr);
    manager_->printStats();  // Should not crash
}

// Test multiple buffer types
TEST_F(VkMemoryManagerTest, MultipleBufferTypes) {
    ASSERT_NE(manager_, nullptr);

    // Storage buffer
    VkMemoryManager::BufferCreateInfo storageInfo{.size = 4096,
                                                  .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                  .memoryUsage = MemoryUsage::GpuOnly};
    auto storageResult = manager_->createBuffer(storageInfo);
    ASSERT_TRUE(storageResult.isSuccess());

    // Uniform buffer
    VkMemoryManager::BufferCreateInfo uniformInfo{.size = 256,
                                                  .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                  .memoryUsage = MemoryUsage::CpuToGpu};
    auto uniformResult = manager_->createBuffer(uniformInfo);
    ASSERT_TRUE(uniformResult.isSuccess());

    // Transfer buffer
    VkMemoryManager::BufferCreateInfo transferInfo{.size = 8192,
                                                   .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                   .memoryUsage = MemoryUsage::CpuToGpu};
    auto transferResult = manager_->createBuffer(transferInfo);
    ASSERT_TRUE(transferResult.isSuccess());

    // Cleanup
    auto storageBuffer = storageResult.value();
    auto uniformBuffer = uniformResult.value();
    auto transferBuffer = transferResult.value();

    manager_->destroyBuffer(storageBuffer);
    manager_->destroyBuffer(uniformBuffer);
    manager_->destroyBuffer(transferBuffer);
}

// Test large allocation
TEST_F(VkMemoryManagerTest, LargeAllocation) {
    ASSERT_NE(manager_, nullptr);

    VkMemoryManager::BufferCreateInfo info{.size = 64 * 1024 * 1024,  // 64 MB
                                           .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                           .memoryUsage = MemoryUsage::GpuOnly};

    auto result = manager_->createBuffer(info);
    ASSERT_TRUE(result.isSuccess());

    auto buffer = result.value();
    EXPECT_NE(buffer.buffer, VK_NULL_HANDLE);

    manager_->destroyBuffer(buffer);
}

// Test destroying null buffer (should be safe)
TEST_F(VkMemoryManagerTest, DestroyNullBuffer) {
    ASSERT_NE(manager_, nullptr);

    VkMemoryManager::Buffer buffer;  // Default initialized to VK_NULL_HANDLE
    manager_->destroyBuffer(buffer);  // Should not crash
}

// Test destroying null image (should be safe)
TEST_F(VkMemoryManagerTest, DestroyNullImage) {
    ASSERT_NE(manager_, nullptr);

    VkMemoryManager::Image image;  // Default initialized to VK_NULL_HANDLE
    manager_->destroyImage(image);  // Should not crash
}

// Test readback buffer
TEST_F(VkMemoryManagerTest, CreateReadbackBuffer) {
    ASSERT_NE(manager_, nullptr);

    VkMemoryManager::BufferCreateInfo info{.size = 2048,
                                           .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                           .memoryUsage = MemoryUsage::GpuToCpu};

    auto result = manager_->createBuffer(info);
    ASSERT_TRUE(result.isSuccess());

    auto buffer = result.value();
    EXPECT_NE(buffer.buffer, VK_NULL_HANDLE);
    EXPECT_NE(buffer.allocation, nullptr);

    manager_->destroyBuffer(buffer);
}

// Test memory usage for all types
TEST_F(VkMemoryManagerTest, AllMemoryUsageTypes) {
    ASSERT_NE(manager_, nullptr);

    MemoryUsage usageTypes[] = {MemoryUsage::GpuOnly, MemoryUsage::CpuToGpu,
                                MemoryUsage::GpuToCpu, MemoryUsage::CpuOnly};

    for (auto usage : usageTypes) {
        VkMemoryManager::BufferCreateInfo info{.size = 1024,
                                               .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                               .memoryUsage = usage};

        auto result = manager_->createBuffer(info);
        ASSERT_TRUE(result.isSuccess()) << "Failed to create buffer with memory usage type";

        auto buffer = result.value();
        manager_->destroyBuffer(buffer);
    }
}
