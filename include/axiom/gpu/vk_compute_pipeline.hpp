#pragma once

#include "axiom/core/result.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace axiom::gpu {

// Forward declarations
class VkContext;
class ShaderModule;
class DescriptorSetLayout;

/// Vulkan compute pipeline wrapper
/// This class manages a VkPipeline and VkPipelineLayout for compute operations.
/// Compute pipelines are used for general-purpose GPU computing (GPGPU) tasks
/// such as particle simulation, fluid dynamics, and parallel constraint solving.
///
/// Features:
/// - Compute shader binding
/// - Descriptor set layout management
/// - Push constant support
/// - Specialization constants
/// - Pipeline cache support for faster creation
///
/// Example usage:
/// @code
/// // Create compute pipeline
/// ComputePipelineBuilder builder(context);
/// auto pipelineResult = builder
///     .setShader(*computeShader)
///     .setDescriptorSetLayout(*descriptorLayout)
///     .setPushConstantRange({0, sizeof(PushConstants)})
///     .build();
///
/// if (pipelineResult.isSuccess()) {
///     auto pipeline = std::move(pipelineResult.value());
///
///     // Use in command buffer
///     pipeline->bind(cmdBuffer);
///     pipeline->dispatch(cmdBuffer, groupCountX, groupCountY, groupCountZ);
/// }
/// @endcode
class ComputePipeline {
public:
    /// Destructor - cleans up VkPipeline and VkPipelineLayout
    ~ComputePipeline();

    // Non-copyable, non-movable
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;
    ComputePipeline(ComputePipeline&&) = delete;
    ComputePipeline& operator=(ComputePipeline&&) = delete;

    /// Bind this pipeline to a command buffer
    /// Must be called before dispatch operations
    /// @param cmd Command buffer to bind to
    void bind(VkCommandBuffer cmd) const noexcept;

    /// Dispatch compute work with specified group counts
    /// Group counts define how many workgroups to execute in each dimension.
    /// The actual thread count depends on the local_size in the shader.
    /// @param cmd Command buffer to record into
    /// @param groupCountX Number of workgroups in X dimension
    /// @param groupCountY Number of workgroups in Y dimension (default 1)
    /// @param groupCountZ Number of workgroups in Z dimension (default 1)
    void dispatch(VkCommandBuffer cmd, uint32_t groupCountX, uint32_t groupCountY = 1,
                  uint32_t groupCountZ = 1) const noexcept;

    /// Dispatch compute work with indirect parameters from a buffer
    /// The buffer must contain a VkDispatchIndirectCommand structure at the specified offset.
    /// This allows GPU-driven compute dispatch where the workgroup counts are determined
    /// by previous GPU operations.
    /// @param cmd Command buffer to record into
    /// @param buffer Buffer containing dispatch parameters
    /// @param offset Offset in bytes to the dispatch parameters
    void dispatchIndirect(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset) const noexcept;

    /// Get the Vulkan pipeline handle
    /// @return VkPipeline handle
    VkPipeline get() const noexcept { return pipeline_; }

    /// Get the pipeline layout handle
    /// @return VkPipelineLayout handle
    VkPipelineLayout getLayout() const noexcept { return layout_; }

private:
    friend class ComputePipelineBuilder;

    /// Private constructor - use ComputePipelineBuilder instead
    /// @param context Valid VkContext pointer (must outlive this object)
    /// @param pipeline VkPipeline handle (ownership transferred)
    /// @param layout VkPipelineLayout handle (ownership transferred)
    ComputePipeline(VkContext* context, VkPipeline pipeline, VkPipelineLayout layout);

    VkContext* context_;                        ///< Vulkan context (not owned)
    VkPipeline pipeline_ = VK_NULL_HANDLE;      ///< Vulkan pipeline handle
    VkPipelineLayout layout_ = VK_NULL_HANDLE;  ///< Vulkan pipeline layout handle
};

/// Builder for creating compute pipelines
/// This class provides a fluent interface for building compute pipelines with
/// shader modules, descriptor set layouts, push constants, and specialization constants.
///
/// Example usage:
/// @code
/// ComputePipelineBuilder builder(context);
///
/// // Setup specialization constants
/// struct SpecData {
///     uint32_t workGroupSize = 512;
/// } specData;
///
/// auto pipelineResult = builder
///     .setShader(*computeShader)
///     .addDescriptorSetLayout(*layout0)
///     .addDescriptorSetLayout(*layout1)
///     .setPushConstantRange({0, sizeof(PushConstants)})
///     .addSpecializationConstant({0, offsetof(SpecData, workGroupSize), sizeof(uint32_t)})
///     .setSpecializationData(&specData, sizeof(specData))
///     .setPipelineCache(cache.get())
///     .build();
/// @endcode
class ComputePipelineBuilder {
public:
    /// Push constant range specification
    /// Defines a range of push constants that can be updated quickly without descriptor sets
    struct PushConstantRange {
        uint32_t offset;  ///< Offset in bytes (typically 0)
        uint32_t size;    ///< Size in bytes (max 128 bytes recommended)
    };

    /// Specialization constant specification
    /// Allows compile-time constants in shaders to be set at pipeline creation time
    /// This enables shader optimization while maintaining runtime flexibility
    struct SpecializationConstant {
        uint32_t constantId;  ///< Constant ID in shader (layout(constant_id = N))
        uint32_t offset;      ///< Offset in specialization data buffer
        size_t size;          ///< Size of the constant (4 for uint32_t, etc.)
    };

    /// Create a builder for compute pipelines
    /// @param context Valid VkContext pointer
    explicit ComputePipelineBuilder(VkContext* context);

    /// Set the compute shader module
    /// This is required - pipeline creation will fail without a shader
    /// @param shader Compute shader module (must remain valid until build())
    /// @return Reference to this builder for method chaining
    ComputePipelineBuilder& setShader(const ShaderModule& shader);

    /// Set a single descriptor set layout (replaces previous layouts)
    /// Use this when you have only one descriptor set
    /// @param layout Descriptor set layout (must remain valid until build())
    /// @return Reference to this builder for method chaining
    ComputePipelineBuilder& setDescriptorSetLayout(const DescriptorSetLayout& layout);

    /// Add a descriptor set layout (appends to existing layouts)
    /// Use this to build pipelines with multiple descriptor sets
    /// Descriptor sets are numbered 0, 1, 2, ... in the order added
    /// @param layout Descriptor set layout to add (must remain valid until build())
    /// @return Reference to this builder for method chaining
    ComputePipelineBuilder& addDescriptorSetLayout(const DescriptorSetLayout& layout);

    /// Set push constant range
    /// Push constants provide a fast way to update small amounts of data (typically < 128 bytes)
    /// without using descriptor sets. Only one range is supported for compute shaders.
    /// @param range Push constant range specification
    /// @return Reference to this builder for method chaining
    ComputePipelineBuilder& setPushConstantRange(const PushConstantRange& range);

    /// Add a specialization constant entry
    /// Specialization constants allow shader optimization based on runtime-provided values.
    /// They are evaluated at pipeline creation time, allowing the compiler to optimize.
    /// @param constant Specialization constant specification
    /// @return Reference to this builder for method chaining
    ComputePipelineBuilder& addSpecializationConstant(const SpecializationConstant& constant);

    /// Set specialization constant data
    /// This buffer contains the actual values for all specialization constants.
    /// The buffer must remain valid until build() is called.
    /// @param data Pointer to specialization data buffer
    /// @param size Size of the data buffer in bytes
    /// @return Reference to this builder for method chaining
    ComputePipelineBuilder& setSpecializationData(const void* data, size_t size);

    /// Set pipeline cache for faster pipeline creation
    /// Pipeline caches store compiled pipeline data for reuse across runs.
    /// This can significantly reduce pipeline creation time (10-100ms savings).
    /// @param cache VkPipelineCache handle (use PipelineCache class to manage)
    /// @return Reference to this builder for method chaining
    ComputePipelineBuilder& setPipelineCache(VkPipelineCache cache);

    /// Build the compute pipeline
    /// Creates VkPipelineLayout and VkPipeline based on the configured settings.
    /// @return Result containing unique_ptr to ComputePipeline or error code
    core::Result<std::unique_ptr<ComputePipeline>> build();

private:
    VkContext* context_;                                           ///< Vulkan context (not owned)
    const ShaderModule* shader_ = nullptr;                         ///< Compute shader (not owned)
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts_;      ///< Descriptor set layouts
    std::optional<PushConstantRange> pushConstantRange_;           ///< Push constant range
    std::vector<VkSpecializationMapEntry> specializationEntries_;  ///< Specialization entries
    std::vector<uint8_t> specializationData_;                      ///< Specialization data buffer
    VkPipelineCache pipelineCache_ = VK_NULL_HANDLE;               ///< Pipeline cache (optional)
};

/// Pipeline cache for storing and loading compiled pipelines
/// Pipeline caches reduce pipeline creation time by storing compiled shader code
/// and pipeline state. Caches can be saved to disk and loaded on subsequent runs,
/// providing significant performance improvements (10-100ms per pipeline).
///
/// Recommended usage:
/// - Create one cache per application session
/// - Load from disk at startup (if available)
/// - Use the cache for all pipeline creation
/// - Save to disk at shutdown
///
/// Example usage:
/// @code
/// // At startup
/// auto cacheResult = PipelineCache::create(context);
/// if (cacheResult.isSuccess()) {
///     auto cache = std::move(cacheResult.value());
///     cache->load("pipeline_cache.bin");  // Ignore errors if file doesn't exist
///
///     // Use cache for pipeline creation
///     builder.setPipelineCache(cache->get());
///
///     // At shutdown
///     cache->save("pipeline_cache.bin");
/// }
/// @endcode
class PipelineCache {
public:
    /// Create a new pipeline cache
    /// Creates an empty VkPipelineCache that can be used for pipeline creation
    /// and optionally loaded from disk
    /// @param context Valid VkContext pointer (must outlive this object)
    /// @return Result containing unique_ptr to PipelineCache or error code
    static core::Result<std::unique_ptr<PipelineCache>> create(VkContext* context);

    /// Destructor - cleans up VkPipelineCache
    ~PipelineCache();

    // Non-copyable, non-movable
    PipelineCache(const PipelineCache&) = delete;
    PipelineCache& operator=(const PipelineCache&) = delete;
    PipelineCache(PipelineCache&&) = delete;
    PipelineCache& operator=(PipelineCache&&) = delete;

    /// Save pipeline cache data to file
    /// Retrieves the compiled pipeline data from the Vulkan driver and writes it to disk.
    /// This data is driver-specific and may not be portable across different GPU vendors.
    /// @param path File path to save cache data (will be created or overwritten)
    /// @return Result indicating success or failure
    core::Result<void> save(const std::string& path);

    /// Load pipeline cache data from file
    /// Reads previously saved pipeline data and initializes the cache.
    /// If the file doesn't exist or is invalid, the cache remains empty (not an error).
    /// @param path File path to load cache data from
    /// @return Result indicating success or failure
    core::Result<void> load(const std::string& path);

    /// Get the Vulkan pipeline cache handle
    /// @return VkPipelineCache handle
    VkPipelineCache get() const noexcept { return cache_; }

private:
    /// Private constructor - use create() instead
    explicit PipelineCache(VkContext* context);

    /// Initialize the cache (creates empty VkPipelineCache)
    /// @return Result indicating success or failure
    core::Result<void> initialize();

    VkContext* context_;                      ///< Vulkan context (not owned)
    VkPipelineCache cache_ = VK_NULL_HANDLE;  ///< Vulkan pipeline cache handle
};

}  // namespace axiom::gpu
