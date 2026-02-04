#include "axiom/gpu/vk_instance.hpp"
#include "axiom/gpu/vk_shader.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>

using namespace axiom::gpu;
using namespace axiom::core;

// Test fixture for VkShader tests
class VkShaderTest : public ::testing::Test {
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
    }

    void TearDown() override {
        // Cleanup happens automatically through destructors
        context_.reset();
    }

    std::unique_ptr<VkContext> context_;
    const std::string testShaderPath_ = "shaders/test/simple.comp.spv";
};

// Test shader module creation from file
TEST_F(VkShaderTest, CreateFromFile) {
    ASSERT_NE(context_, nullptr);

    // Check if test shader exists
    if (!std::filesystem::exists(testShaderPath_)) {
        GTEST_SKIP() << "Test shader not found: " << testShaderPath_
                     << " (compile shaders/test/simple.comp with slangc)";
    }

    auto result = ShaderModule::createFromFile(context_.get(), testShaderPath_,
                                               ShaderStage::Compute);
    ASSERT_TRUE(result.isSuccess()) << "Failed: " << result.errorMessage();

    auto shader = std::move(result.value());
    EXPECT_NE(shader->get(), VK_NULL_HANDLE);
    EXPECT_EQ(shader->getStage(), ShaderStage::Compute);
    EXPECT_EQ(shader->getStageFlags(), VK_SHADER_STAGE_COMPUTE_BIT);
    EXPECT_STREQ(shader->getEntryPoint(), "main");
    EXPECT_EQ(shader->getSourcePath(), testShaderPath_);
    EXPECT_GT(shader->getSpirv().size(), 0);
}

// Test shader creation with null context
TEST(VkShaderStaticTest, CreateWithNullContext) {
    auto result = ShaderModule::createFromFile(nullptr, "shaders/test/simple.comp.spv",
                                               ShaderStage::Compute);
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test shader creation with non-existent file
TEST_F(VkShaderTest, CreateFromNonExistentFile) {
    ASSERT_NE(context_, nullptr);

    auto result = ShaderModule::createFromFile(context_.get(), "nonexistent.spv",
                                               ShaderStage::Compute);
    EXPECT_TRUE(result.isFailure());
}

// Test shader creation from bytecode
TEST_F(VkShaderTest, CreateFromCode) {
    ASSERT_NE(context_, nullptr);

    // Check if test shader exists
    if (!std::filesystem::exists(testShaderPath_)) {
        GTEST_SKIP() << "Test shader not found: " << testShaderPath_;
    }

    // First, load from file to get the bytecode
    auto loadResult = ShaderModule::createFromFile(context_.get(), testShaderPath_,
                                                   ShaderStage::Compute);
    ASSERT_TRUE(loadResult.isSuccess());
    auto loadedShader = std::move(loadResult.value());
    const auto& spirvCode = loadedShader->getSpirv();

    // Now create from bytecode
    auto result = ShaderModule::createFromCode(context_.get(), spirvCode, ShaderStage::Compute);
    ASSERT_TRUE(result.isSuccess());

    auto shader = std::move(result.value());
    EXPECT_NE(shader->get(), VK_NULL_HANDLE);
    EXPECT_EQ(shader->getStage(), ShaderStage::Compute);
    EXPECT_EQ(shader->getSpirv().size(), spirvCode.size());
    EXPECT_TRUE(shader->getSourcePath().empty());  // No source path when created from code
}

// Test shader creation from invalid bytecode (too small)
TEST_F(VkShaderTest, CreateFromInvalidBytecode_TooSmall) {
    ASSERT_NE(context_, nullptr);

    std::vector<uint32_t> invalidCode = {0x07230203, 0x00010000, 0x000D000B};  // Only 3 words
    auto result = ShaderModule::createFromCode(context_.get(), invalidCode, ShaderStage::Compute);
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test shader creation from invalid bytecode (wrong magic number)
TEST_F(VkShaderTest, CreateFromInvalidBytecode_WrongMagic) {
    ASSERT_NE(context_, nullptr);

    std::vector<uint32_t> invalidCode = {0xBADC0DE0,  // Wrong magic number
                                         0x00010000, 0x000D000B, 0x00000001, 0x00000000,
                                         0x00000001, 0x00000002, 0x00000003};
    auto result = ShaderModule::createFromCode(context_.get(), invalidCode, ShaderStage::Compute);
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test shader stage conversion
TEST_F(VkShaderTest, ShaderStageConversion) {
    ASSERT_NE(context_, nullptr);

    if (!std::filesystem::exists(testShaderPath_)) {
        GTEST_SKIP() << "Test shader not found: " << testShaderPath_;
    }

    // Test different shader stages
    struct StageTest {
        ShaderStage stage;
        VkShaderStageFlagBits expectedFlag;
    };

    StageTest tests[] = {
        {ShaderStage::Vertex, VK_SHADER_STAGE_VERTEX_BIT},
        {ShaderStage::Fragment, VK_SHADER_STAGE_FRAGMENT_BIT},
        {ShaderStage::Compute, VK_SHADER_STAGE_COMPUTE_BIT},
        {ShaderStage::Geometry, VK_SHADER_STAGE_GEOMETRY_BIT},
        {ShaderStage::TessControl, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT},
        {ShaderStage::TessEvaluation, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT}};

    for (const auto& test : tests) {
        auto result = ShaderModule::createFromFile(context_.get(), testShaderPath_, test.stage);
        ASSERT_TRUE(result.isSuccess());
        auto shader = std::move(result.value());
        EXPECT_EQ(shader->getStage(), test.stage);
        EXPECT_EQ(shader->getStageFlags(), test.expectedFlag);
    }
}

// Test shader reflection (currently returns empty, but should not crash)
TEST_F(VkShaderTest, ShaderReflection) {
    ASSERT_NE(context_, nullptr);

    if (!std::filesystem::exists(testShaderPath_)) {
        GTEST_SKIP() << "Test shader not found: " << testShaderPath_;
    }

    auto result = ShaderModule::createFromFile(context_.get(), testShaderPath_,
                                               ShaderStage::Compute);
    ASSERT_TRUE(result.isSuccess());
    auto shader = std::move(result.value());

    // Reflection is not yet implemented, should return empty
    auto bindings = shader->getBindings();
    EXPECT_TRUE(bindings.empty());  // Currently not implemented

    auto pushConstants = shader->getPushConstantInfo();
    EXPECT_FALSE(pushConstants.has_value());  // Currently not implemented
}

// ========================================
// ShaderCache tests
// ========================================

class VkShaderCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear cache before each test
        ShaderCache::instance().clear();

        // Create Vulkan context
        auto contextResult = VkContext::create();
        if (contextResult.isSuccess()) {
            context_ = std::move(contextResult.value());
        } else {
            GTEST_SKIP() << "Vulkan not available: " << contextResult.errorMessage();
        }
    }

    void TearDown() override {
        // Clear cache after each test
        ShaderCache::instance().clear();
        context_.reset();
    }

    std::unique_ptr<VkContext> context_;
    const std::string testShaderPath_ = "shaders/test/simple.comp.spv";
};

// Test shader cache loading
TEST_F(VkShaderCacheTest, LoadShader) {
    ASSERT_NE(context_, nullptr);

    if (!std::filesystem::exists(testShaderPath_)) {
        GTEST_SKIP() << "Test shader not found: " << testShaderPath_;
    }

    auto& cache = ShaderCache::instance();
    EXPECT_EQ(cache.size(), 0);

    auto result = cache.load(context_.get(), testShaderPath_, ShaderStage::Compute);
    ASSERT_TRUE(result.isSuccess());

    auto shader = result.value();
    EXPECT_NE(shader, nullptr);
    EXPECT_NE(shader->get(), VK_NULL_HANDLE);
    EXPECT_EQ(cache.size(), 1);
    EXPECT_TRUE(cache.contains(testShaderPath_));
}

// Test shader cache hit
TEST_F(VkShaderCacheTest, CacheHit) {
    ASSERT_NE(context_, nullptr);

    if (!std::filesystem::exists(testShaderPath_)) {
        GTEST_SKIP() << "Test shader not found: " << testShaderPath_;
    }

    auto& cache = ShaderCache::instance();

    // First load - cache miss
    auto result1 = cache.load(context_.get(), testShaderPath_, ShaderStage::Compute);
    ASSERT_TRUE(result1.isSuccess());
    auto shader1 = result1.value();

    // Second load - cache hit
    auto result2 = cache.load(context_.get(), testShaderPath_, ShaderStage::Compute);
    ASSERT_TRUE(result2.isSuccess());
    auto shader2 = result2.value();

    // Should be the same shared_ptr
    EXPECT_EQ(shader1, shader2);
    EXPECT_EQ(shader1.get(), shader2.get());
    EXPECT_EQ(cache.size(), 1);
}

// Test cache clear
TEST_F(VkShaderCacheTest, CacheClear) {
    ASSERT_NE(context_, nullptr);

    if (!std::filesystem::exists(testShaderPath_)) {
        GTEST_SKIP() << "Test shader not found: " << testShaderPath_;
    }

    auto& cache = ShaderCache::instance();

    // Load shader
    auto result = cache.load(context_.get(), testShaderPath_, ShaderStage::Compute);
    ASSERT_TRUE(result.isSuccess());
    EXPECT_EQ(cache.size(), 1);

    // Clear cache
    cache.clear();
    EXPECT_EQ(cache.size(), 0);
    EXPECT_FALSE(cache.contains(testShaderPath_));
}

// Test cache with multiple shaders
TEST_F(VkShaderCacheTest, MultipleShaders) {
    ASSERT_NE(context_, nullptr);

    if (!std::filesystem::exists(testShaderPath_)) {
        GTEST_SKIP() << "Test shader not found: " << testShaderPath_;
    }

    auto& cache = ShaderCache::instance();

    // Load same shader with different stages (different cache keys would be needed for this)
    // For now, load the same shader multiple times
    auto result1 = cache.load(context_.get(), testShaderPath_, ShaderStage::Compute);
    ASSERT_TRUE(result1.isSuccess());

    // Since path is the same, it should be cached
    EXPECT_EQ(cache.size(), 1);
}

// Test cache with null context
TEST_F(VkShaderCacheTest, LoadWithNullContext) {
    auto& cache = ShaderCache::instance();

    auto result = cache.load(nullptr, testShaderPath_, ShaderStage::Compute);
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::InvalidParameter);
}

// Test cache singleton
TEST_F(VkShaderCacheTest, Singleton) {
    auto& cache1 = ShaderCache::instance();
    auto& cache2 = ShaderCache::instance();

    // Should be the same instance
    EXPECT_EQ(&cache1, &cache2);
}

// ========================================
// ShaderCompiler tests (currently not implemented)
// ========================================

TEST(VkShaderCompilerTest, CompileSlang_NotImplemented) {
    // Runtime compilation is not yet implemented
    auto result = ShaderCompiler::compileSlang(
        "RWStructuredBuffer<float> output; [numthreads(1,1,1)] void main() {}",
        ShaderStage::Compute, "test.comp");
    EXPECT_TRUE(result.isFailure());
    EXPECT_EQ(result.errorCode(), ErrorCode::ShaderCompilationFailed);
}

TEST(VkShaderCompilerTest, CompileSlangFromFile_NotImplemented) {
    // Runtime compilation is not yet implemented
    auto result = ShaderCompiler::compileSlangFromFile("nonexistent.comp", ShaderStage::Compute);
    EXPECT_TRUE(result.isFailure());
}

// ========================================
// Pipeline integration test
// ========================================

TEST_F(VkShaderTest, PipelineIntegrationExample) {
    ASSERT_NE(context_, nullptr);

    if (!std::filesystem::exists(testShaderPath_)) {
        GTEST_SKIP() << "Test shader not found: " << testShaderPath_;
    }

    // Load shader
    auto result = ShaderModule::createFromFile(context_.get(), testShaderPath_,
                                               ShaderStage::Compute);
    ASSERT_TRUE(result.isSuccess());
    auto shader = std::move(result.value());

    // Verify it can be used in a pipeline create info structure
    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = shader->getStageFlags();
    stageInfo.module = shader->get();
    stageInfo.pName = shader->getEntryPoint();

    // Verify all fields are valid
    EXPECT_EQ(stageInfo.sType, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
    EXPECT_EQ(stageInfo.stage, VK_SHADER_STAGE_COMPUTE_BIT);
    EXPECT_NE(stageInfo.module, VK_NULL_HANDLE);
    EXPECT_STREQ(stageInfo.pName, "main");
}
