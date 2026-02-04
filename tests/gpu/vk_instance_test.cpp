#include "axiom/gpu/vk_instance.hpp"

#include <gtest/gtest.h>

using namespace axiom::gpu;
using namespace axiom::core;

// Test fixture for VkContext tests
class VkContextTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create context once for all tests
        auto result = VkContext::create();
        if (result.isSuccess()) {
            context_ = std::move(result.value());
        }
    }

    void TearDown() override {
        // Context will be automatically destroyed
        context_.reset();
    }

    std::unique_ptr<VkContext> context_;
};

// Test that we can create a Vulkan context
TEST_F(VkContextTest, ContextCreation) {
    // Context creation happens in SetUp
    // If we got here, creation succeeded
    EXPECT_NE(context_, nullptr);
}

// Test that instance is valid
TEST_F(VkContextTest, InstanceValid) {
    ASSERT_NE(context_, nullptr);
    EXPECT_NE(context_->getInstance(), VK_NULL_HANDLE);
}

// Test that physical device is selected
TEST_F(VkContextTest, PhysicalDeviceSelected) {
    ASSERT_NE(context_, nullptr);
    EXPECT_NE(context_->getPhysicalDevice(), VK_NULL_HANDLE);
}

// Test that logical device is created
TEST_F(VkContextTest, LogicalDeviceCreated) {
    ASSERT_NE(context_, nullptr);
    EXPECT_NE(context_->getDevice(), VK_NULL_HANDLE);
}

// Test that all queues are retrieved
TEST_F(VkContextTest, QueuesRetrieved) {
    ASSERT_NE(context_, nullptr);
    EXPECT_NE(context_->getGraphicsQueue(), VK_NULL_HANDLE);
    EXPECT_NE(context_->getComputeQueue(), VK_NULL_HANDLE);
    EXPECT_NE(context_->getTransferQueue(), VK_NULL_HANDLE);
}

// Test that queue family indices are valid
TEST_F(VkContextTest, QueueFamilyIndicesValid) {
    ASSERT_NE(context_, nullptr);
    EXPECT_NE(context_->getGraphicsQueueFamily(), UINT32_MAX);
    EXPECT_NE(context_->getComputeQueueFamily(), UINT32_MAX);
    EXPECT_NE(context_->getTransferQueueFamily(), UINT32_MAX);
}

// Test that we can query device properties
TEST_F(VkContextTest, DeviceProperties) {
    ASSERT_NE(context_, nullptr);

    auto properties = context_->getDeviceProperties();
    EXPECT_NE(properties.deviceName[0], '\0');  // Device name should not be empty
    EXPECT_GT(properties.limits.maxComputeWorkGroupCount[0], 0u);

    // Print device info for debugging
    std::cout << "GPU: " << properties.deviceName << std::endl;
    std::cout << "API Version: " << VK_VERSION_MAJOR(properties.apiVersion) << "."
              << VK_VERSION_MINOR(properties.apiVersion) << "."
              << VK_VERSION_PATCH(properties.apiVersion) << std::endl;
}

// Test that we can query memory properties
TEST_F(VkContextTest, MemoryProperties) {
    ASSERT_NE(context_, nullptr);

    auto memProperties = context_->getMemoryProperties();
    EXPECT_GT(memProperties.memoryTypeCount, 0u);
    EXPECT_GT(memProperties.memoryHeapCount, 0u);

    // Print memory info for debugging
    std::cout << "Memory heaps: " << memProperties.memoryHeapCount << std::endl;
    for (uint32_t i = 0; i < memProperties.memoryHeapCount; i++) {
        const auto& heap = memProperties.memoryHeaps[i];
        std::cout << "  Heap " << i << ": " << (heap.size / 1024 / 1024) << " MB";
        if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            std::cout << " (device local)";
        }
        std::cout << std::endl;
    }
}

// Test validation layers status
TEST_F(VkContextTest, ValidationLayersStatus) {
    ASSERT_NE(context_, nullptr);

    // Note: Validation layers are currently disabled even in debug builds
    // due to driver compatibility issues on some systems
    // TODO: Make validation layers configurable via environment variable
    EXPECT_FALSE(context_->hasValidationLayers());
}

// Test that context is move-only (not copyable)
TEST(VkContextMoveTest, NotCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<VkContext>);
    EXPECT_FALSE(std::is_copy_assignable_v<VkContext>);
}

// Test that context is not movable (due to Vulkan handle management)
TEST(VkContextMoveTest, NotMovable) {
    EXPECT_FALSE(std::is_move_constructible_v<VkContext>);
    EXPECT_FALSE(std::is_move_assignable_v<VkContext>);
}

// Test multiple context destruction (should be safe)
TEST(VkContextLifetimeTest, MultipleContexts) {
    // Create and destroy multiple contexts sequentially
    for (int i = 0; i < 3; i++) {
        auto result = VkContext::create();
        EXPECT_TRUE(result.isSuccess());
        if (result.isSuccess()) {
            auto context = std::move(result.value());
            EXPECT_NE(context, nullptr);
            // Context will be destroyed when it goes out of scope
        }
    }
}

// Test error handling when Vulkan is not available (this may not fail on all systems)
// This test is more of a documentation of expected behavior
TEST(VkContextErrorTest, DISABLED_VulkanNotAvailable) {
    // This test would only fail if Vulkan is not installed
    // Disabled by default since most development machines have Vulkan
    auto result = VkContext::create();
    if (result.isFailure()) {
        EXPECT_EQ(result.errorCode(), ErrorCode::VulkanInitializationFailed);
        EXPECT_NE(result.errorMessage(), nullptr);
    }
}
