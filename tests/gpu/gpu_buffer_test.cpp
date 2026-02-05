#include "axiom/gpu/gpu_buffer.hpp"
#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_memory.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

using namespace axiom::gpu;
using namespace axiom::core;

// Test fixture for GpuBuffer tests
class GpuBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create Vulkan context
        auto contextResult = VkContext::create();
        if (contextResult.isSuccess()) {
            context_ = std::move(contextResult.value());

            // Create memory manager
            auto managerResult = VkMemoryManager::create(context_.get());
            if (managerResult.isSuccess()) {
                memManager_ = std::move(managerResult.value());
            } else {
                GTEST_SKIP() << "Failed to create memory manager: " << managerResult.errorMessage();
            }
        } else {
            GTEST_SKIP() << "Vulkan not available: " << contextResult.errorMessage()
                         << " (this is expected in CI environments without GPU)";
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

// Test basic GpuBuffer creation
TEST_F(GpuBufferTest, BasicCreation) {
    ASSERT_NE(memManager_, nullptr);

    GpuBuffer buffer(memManager_.get(), 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     MemoryUsage::GpuOnly);

    EXPECT_NE(buffer.getBuffer(), VK_NULL_HANDLE);
    EXPECT_EQ(buffer.getSize(), 1024);
    EXPECT_FALSE(buffer.isMapped());
}

// Test GpuBuffer move constructor
TEST_F(GpuBufferTest, MoveConstructor) {
    ASSERT_NE(memManager_, nullptr);

    GpuBuffer buffer1(memManager_.get(), 2048, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      MemoryUsage::GpuOnly);
    VkBuffer handle = buffer1.getBuffer();
    EXPECT_NE(handle, VK_NULL_HANDLE);

    GpuBuffer buffer2(std::move(buffer1));
    EXPECT_EQ(buffer2.getBuffer(), handle);
    EXPECT_EQ(buffer2.getSize(), 2048);
    EXPECT_EQ(buffer1.getBuffer(), VK_NULL_HANDLE);  // Moved-from buffer is empty
}

// Test GpuBuffer move assignment
TEST_F(GpuBufferTest, MoveAssignment) {
    ASSERT_NE(memManager_, nullptr);

    GpuBuffer buffer1(memManager_.get(), 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      MemoryUsage::GpuOnly);
    GpuBuffer buffer2(memManager_.get(), 2048, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      MemoryUsage::GpuOnly);

    VkBuffer handle = buffer2.getBuffer();
    buffer1 = std::move(buffer2);

    EXPECT_EQ(buffer1.getBuffer(), handle);
    EXPECT_EQ(buffer1.getSize(), 2048);
}

// Test upload to CPU-accessible buffer
TEST_F(GpuBufferTest, UploadToCpuAccessible) {
    ASSERT_NE(memManager_, nullptr);

    GpuBuffer buffer(memManager_.get(), 256, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     MemoryUsage::CpuToGpu);

    // Upload test data
    std::vector<uint32_t> testData = {1, 2, 3, 4, 5, 6, 7, 8};
    auto result = buffer.upload(testData.data(), testData.size() * sizeof(uint32_t));
    EXPECT_TRUE(result.isSuccess());
}

// Test upload to GPU-only buffer (uses staging)
TEST_F(GpuBufferTest, UploadToGpuOnly) {
    ASSERT_NE(memManager_, nullptr);

    GpuBuffer buffer(memManager_.get(), 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     MemoryUsage::GpuOnly);

    // Upload test data
    std::vector<float> testData(256, 42.0f);
    auto result = buffer.upload(testData.data(), testData.size() * sizeof(float));
    EXPECT_TRUE(result.isSuccess());
}

// Test download from CPU-accessible buffer
TEST_F(GpuBufferTest, DownloadFromCpuAccessible) {
    ASSERT_NE(memManager_, nullptr);

    GpuBuffer buffer(memManager_.get(), 256, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     MemoryUsage::CpuToGpu);

    // Upload test data
    std::vector<uint32_t> testData = {10, 20, 30, 40, 50};
    buffer.upload(testData.data(), testData.size() * sizeof(uint32_t));

    // Download and verify
    std::vector<uint32_t> readData(5);
    auto result = buffer.download(readData.data(), readData.size() * sizeof(uint32_t));
    ASSERT_TRUE(result.isSuccess());

    EXPECT_EQ(readData, testData);
}

// Test download from GPU-only buffer (uses staging)
TEST_F(GpuBufferTest, DownloadFromGpuOnly) {
    ASSERT_NE(memManager_, nullptr);

    GpuBuffer buffer(memManager_.get(), 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     MemoryUsage::GpuOnly);

    // Upload test data
    std::vector<float> testData(64);
    for (size_t i = 0; i < testData.size(); i++) {
        testData[i] = static_cast<float>(i) * 2.5f;
    }
    buffer.upload(testData.data(), testData.size() * sizeof(float));

    // Download and verify
    std::vector<float> readData(64);
    auto result = buffer.download(readData.data(), readData.size() * sizeof(float));
    ASSERT_TRUE(result.isSuccess());

    for (size_t i = 0; i < testData.size(); i++) {
        EXPECT_FLOAT_EQ(readData[i], testData[i]);
    }
}

// Test mapping and unmapping
TEST_F(GpuBufferTest, MapUnmap) {
    ASSERT_NE(memManager_, nullptr);

    GpuBuffer buffer(memManager_.get(), 512, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     MemoryUsage::CpuToGpu);

    // Map buffer
    auto mapResult = buffer.map();
    ASSERT_TRUE(mapResult.isSuccess());
    void* ptr = mapResult.value();
    EXPECT_NE(ptr, nullptr);
    EXPECT_TRUE(buffer.isMapped());

    // Write data
    uint32_t testValue = 0xDEADBEEF;
    std::memcpy(ptr, &testValue, sizeof(testValue));

    // Unmap
    buffer.unmap();
    EXPECT_FALSE(buffer.isMapped());
}

// Test mapping GPU-only buffer fails
TEST_F(GpuBufferTest, MapGpuOnlyFails) {
    ASSERT_NE(memManager_, nullptr);

    GpuBuffer buffer(memManager_.get(), 256, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     MemoryUsage::GpuOnly);

    auto result = buffer.map();
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test buffer resize
TEST_F(GpuBufferTest, Resize) {
    ASSERT_NE(memManager_, nullptr);

    GpuBuffer buffer(memManager_.get(), 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     MemoryUsage::GpuOnly);

    VkBuffer oldHandle = buffer.getBuffer();
    EXPECT_EQ(buffer.getSize(), 1024);

    auto result = buffer.resize(2048);
    ASSERT_TRUE(result.isSuccess());

    EXPECT_EQ(buffer.getSize(), 2048);
    EXPECT_NE(buffer.getBuffer(), oldHandle);  // New buffer created
}

// Test TypedBuffer creation
TEST_F(GpuBufferTest, TypedBufferCreation) {
    ASSERT_NE(memManager_, nullptr);

    struct Vertex {
        float x, y, z;
    };

    TypedBuffer<Vertex> buffer(memManager_.get(), 100, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                               MemoryUsage::GpuOnly);

    EXPECT_NE(buffer.getBuffer(), VK_NULL_HANDLE);
    EXPECT_EQ(buffer.getCount(), 100);
    EXPECT_EQ(buffer.getSize(), 100 * sizeof(Vertex));
}

// Test TypedBuffer upload with vector
TEST_F(GpuBufferTest, TypedBufferUploadVector) {
    ASSERT_NE(memManager_, nullptr);

    TypedBuffer<uint32_t> buffer(memManager_.get(), 10, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 MemoryUsage::GpuOnly);

    std::vector<uint32_t> testData = {1, 2, 3, 4, 5};
    auto result = buffer.upload(testData);
    EXPECT_TRUE(result.isSuccess());
}

// Test TypedBuffer upload with array
TEST_F(GpuBufferTest, TypedBufferUploadArray) {
    ASSERT_NE(memManager_, nullptr);

    TypedBuffer<float> buffer(memManager_.get(), 20, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                              MemoryUsage::GpuOnly);

    float testData[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    auto result = buffer.upload(testData, 5);
    EXPECT_TRUE(result.isSuccess());
}

// Test TypedBuffer download to vector
TEST_F(GpuBufferTest, TypedBufferDownloadVector) {
    ASSERT_NE(memManager_, nullptr);

    TypedBuffer<uint32_t> buffer(memManager_.get(), 10, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 MemoryUsage::GpuOnly);

    std::vector<uint32_t> testData = {10, 20, 30, 40, 50};
    buffer.upload(testData);

    std::vector<uint32_t> readData;
    auto result = buffer.download(readData);
    ASSERT_TRUE(result.isSuccess());

    EXPECT_EQ(readData.size(), 10);
    for (size_t i = 0; i < testData.size(); i++) {
        EXPECT_EQ(readData[i], testData[i]);
    }
}

// Test TypedBuffer mapTyped
TEST_F(GpuBufferTest, TypedBufferMapTyped) {
    ASSERT_NE(memManager_, nullptr);

    TypedBuffer<float> buffer(memManager_.get(), 8, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                              MemoryUsage::CpuToGpu);

    auto result = buffer.mapTyped();
    ASSERT_TRUE(result.isSuccess());

    float* ptr = result.value();
    EXPECT_NE(ptr, nullptr);

    // Write some data
    for (int i = 0; i < 8; i++) {
        ptr[i] = static_cast<float>(i) * 1.5f;
    }

    buffer.unmap();
}

// Test VertexBuffer alias
TEST_F(GpuBufferTest, VertexBufferCreation) {
    ASSERT_NE(memManager_, nullptr);

    struct MyVertex {
        float pos[3];
        float normal[3];
        float uv[2];
    };

    VertexBuffer<MyVertex> vbo(memManager_.get(), 1000, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                               MemoryUsage::GpuOnly);

    EXPECT_NE(vbo.getBuffer(), VK_NULL_HANDLE);
    EXPECT_EQ(vbo.getCount(), 1000);
}

// Test IndexBuffer
TEST_F(GpuBufferTest, IndexBufferCreation) {
    ASSERT_NE(memManager_, nullptr);

    IndexBuffer ibo(memManager_.get(), 3000, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    MemoryUsage::GpuOnly);

    EXPECT_NE(ibo.getBuffer(), VK_NULL_HANDLE);
    EXPECT_EQ(ibo.getCount(), 3000);
    EXPECT_EQ(ibo.getSize(), 3000 * sizeof(uint32_t));
}

// Test IndexBuffer16
TEST_F(GpuBufferTest, IndexBuffer16Creation) {
    ASSERT_NE(memManager_, nullptr);

    IndexBuffer16 ibo(memManager_.get(), 2000, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      MemoryUsage::GpuOnly);

    EXPECT_EQ(ibo.getCount(), 2000);
    EXPECT_EQ(ibo.getSize(), 2000 * sizeof(uint16_t));
}

// Test UniformBuffer creation and persistent mapping
TEST_F(GpuBufferTest, UniformBufferCreation) {
    ASSERT_NE(memManager_, nullptr);

    struct CameraUBO {
        float viewMatrix[16];
        float projMatrix[16];
    };

    UniformBuffer<CameraUBO> ubo(memManager_.get());

    EXPECT_NE(ubo.getBuffer(), VK_NULL_HANDLE);
    EXPECT_EQ(ubo.getCount(), 1);
    EXPECT_TRUE(ubo.isMapped());  // Should be persistently mapped
}

// Test UniformBuffer update
TEST_F(GpuBufferTest, UniformBufferUpdate) {
    ASSERT_NE(memManager_, nullptr);

    struct TestUBO {
        uint32_t frame;
        float time;
    };

    UniformBuffer<TestUBO> ubo(memManager_.get());

    TestUBO data1 = {1, 0.016f};
    auto result1 = ubo.update(data1);
    EXPECT_TRUE(result1.isSuccess());

    TestUBO data2 = {2, 0.032f};
    auto result2 = ubo.update(data2);
    EXPECT_TRUE(result2.isSuccess());
}

// Test StorageBuffer creation
TEST_F(GpuBufferTest, StorageBufferCreation) {
    ASSERT_NE(memManager_, nullptr);

    struct Particle {
        float position[3];
        float velocity[3];
    };

    StorageBuffer<Particle> ssbo(memManager_.get(), 10000);

    EXPECT_NE(ssbo.getBuffer(), VK_NULL_HANDLE);
    EXPECT_EQ(ssbo.getCount(), 10000);
    EXPECT_FALSE(ssbo.isMapped());  // GPU-only by default
}

// Test StorageBuffer upload and download
TEST_F(GpuBufferTest, StorageBufferUploadDownload) {
    ASSERT_NE(memManager_, nullptr);

    struct Data {
        float value;
        uint32_t index;
    };

    StorageBuffer<Data> ssbo(memManager_.get(), 100);

    // Upload test data
    std::vector<Data> testData(100);
    for (size_t i = 0; i < testData.size(); i++) {
        testData[i].value = static_cast<float>(i) * 0.5f;
        testData[i].index = static_cast<uint32_t>(i);
    }

    auto uploadResult = ssbo.upload(testData);
    ASSERT_TRUE(uploadResult.isSuccess());

    // Download and verify
    std::vector<Data> readData;
    auto downloadResult = ssbo.download(readData);
    ASSERT_TRUE(downloadResult.isSuccess());

    EXPECT_EQ(readData.size(), 100);
    for (size_t i = 0; i < testData.size(); i++) {
        EXPECT_FLOAT_EQ(readData[i].value, testData[i].value);
        EXPECT_EQ(readData[i].index, testData[i].index);
    }
}

// Test IndirectBuffer creation
TEST_F(GpuBufferTest, IndirectBufferCreation) {
    ASSERT_NE(memManager_, nullptr);

    IndirectBuffer indirectBuf(memManager_.get(), 256);

    EXPECT_NE(indirectBuf.getBuffer(), VK_NULL_HANDLE);
    EXPECT_EQ(indirectBuf.getSize(), 256);
}

// Test upload with invalid parameters
TEST_F(GpuBufferTest, UploadInvalidParameters) {
    ASSERT_NE(memManager_, nullptr);

    GpuBuffer buffer(memManager_.get(), 128, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     MemoryUsage::GpuOnly);

    // Null data pointer
    auto result1 = buffer.upload(nullptr, 64);
    EXPECT_TRUE(result1.isFailure());
    EXPECT_EQ(result1.errorCode(), ErrorCode::InvalidParameter);

    // Size exceeds buffer
    uint32_t data = 42;
    auto result2 = buffer.upload(&data, 256);
    EXPECT_TRUE(result2.isFailure());
    EXPECT_EQ(result2.errorCode(), ErrorCode::InvalidParameter);
}

// Test download with invalid parameters
TEST_F(GpuBufferTest, DownloadInvalidParameters) {
    ASSERT_NE(memManager_, nullptr);

    GpuBuffer buffer(memManager_.get(), 128, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     MemoryUsage::GpuOnly);

    // Null data pointer
    auto result1 = buffer.download(nullptr, 64);
    EXPECT_TRUE(result1.isFailure());
    EXPECT_EQ(result1.errorCode(), ErrorCode::InvalidParameter);

    // Size exceeds buffer
    uint32_t data;
    auto result2 = buffer.download(&data, 256);
    EXPECT_TRUE(result2.isFailure());
    EXPECT_EQ(result2.errorCode(), ErrorCode::InvalidParameter);
}

// Test TypedBuffer upload exceeding capacity
TEST_F(GpuBufferTest, TypedBufferUploadExceedsCapacity) {
    ASSERT_NE(memManager_, nullptr);

    TypedBuffer<uint32_t> buffer(memManager_.get(), 5, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 MemoryUsage::GpuOnly);

    std::vector<uint32_t> testData(10, 42);  // 10 elements, buffer has capacity for 5
    auto result = buffer.upload(testData);
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test buffer resize to zero fails
TEST_F(GpuBufferTest, ResizeToZeroFails) {
    ASSERT_NE(memManager_, nullptr);

    GpuBuffer buffer(memManager_.get(), 128, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     MemoryUsage::GpuOnly);

    auto result = buffer.resize(0);
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Integration test: Complete workflow
TEST_F(GpuBufferTest, CompleteWorkflow) {
    ASSERT_NE(memManager_, nullptr);

    struct Particle {
        float pos[3];
        float vel[3];
        uint32_t id;
    };

    // Create storage buffer for particles
    StorageBuffer<Particle> particles(memManager_.get(), 1000);

    // Initialize particle data
    std::vector<Particle> initialData(1000);
    for (size_t i = 0; i < initialData.size(); i++) {
        initialData[i].pos[0] = static_cast<float>(i) * 0.1f;
        initialData[i].pos[1] = static_cast<float>(i) * 0.2f;
        initialData[i].pos[2] = 0.0f;
        initialData[i].vel[0] = 1.0f;
        initialData[i].vel[1] = 0.0f;
        initialData[i].vel[2] = 0.0f;
        initialData[i].id = static_cast<uint32_t>(i);
    }

    // Upload to GPU
    auto uploadResult = particles.upload(initialData);
    ASSERT_TRUE(uploadResult.isSuccess());

    // Download and verify
    std::vector<Particle> readData;
    auto downloadResult = particles.download(readData);
    ASSERT_TRUE(downloadResult.isSuccess());

    ASSERT_EQ(readData.size(), 1000);
    for (size_t i = 0; i < 1000; i++) {
        EXPECT_FLOAT_EQ(readData[i].pos[0], initialData[i].pos[0]);
        EXPECT_FLOAT_EQ(readData[i].pos[1], initialData[i].pos[1]);
        EXPECT_FLOAT_EQ(readData[i].pos[2], initialData[i].pos[2]);
        EXPECT_EQ(readData[i].id, initialData[i].id);
    }
}
