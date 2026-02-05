#pragma once

#include "axiom/core/result.hpp"

#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace axiom::gpu {

// Forward declaration
class VkContext;

/// Descriptor set layout wrapper
/// This class manages a VkDescriptorSetLayout which defines the structure and types
/// of descriptor bindings that shaders can access. A descriptor set layout is like
/// a schema that describes what resources (buffers, images, samplers) a shader expects.
///
/// Descriptor set layouts are immutable once created and should be shared across
/// multiple descriptor sets that have the same layout structure.
///
/// Example usage:
/// @code
/// DescriptorSetLayoutBuilder builder(context);
/// auto layout = builder
///     .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
///     .addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
///     .build();
///
/// if (layout.isSuccess()) {
///     // Use layout for descriptor pool and pipeline creation
/// }
/// @endcode
class DescriptorSetLayout {
public:
    /// Create descriptor set layout from bindings
    /// @param context Valid VkContext pointer (must outlive this object)
    /// @param bindings Vector of descriptor set layout bindings
    /// @return Result containing unique_ptr to DescriptorSetLayout or error code
    static core::Result<std::unique_ptr<DescriptorSetLayout>>
    create(VkContext* context, const std::vector<VkDescriptorSetLayoutBinding>& bindings);

    /// Destructor - cleans up VkDescriptorSetLayout
    ~DescriptorSetLayout();

    // Non-copyable, non-movable
    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout(DescriptorSetLayout&&) = delete;
    DescriptorSetLayout& operator=(DescriptorSetLayout&&) = delete;

    /// Get the Vulkan descriptor set layout handle
    /// @return VkDescriptorSetLayout handle
    VkDescriptorSetLayout get() const noexcept { return layout_; }

    /// Get the binding information
    /// @return Vector of descriptor set layout bindings
    const std::vector<VkDescriptorSetLayoutBinding>& getBindings() const noexcept {
        return bindings_;
    }

private:
    /// Private constructor - use create() or builder instead
    explicit DescriptorSetLayout(VkContext* context);

    /// Initialize with bindings
    /// @param bindings Vector of descriptor set layout bindings
    /// @return Result indicating success or failure
    core::Result<void> initialize(const std::vector<VkDescriptorSetLayoutBinding>& bindings);

    VkContext* context_;                                  ///< Vulkan context (not owned)
    VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;       ///< Vulkan layout handle
    std::vector<VkDescriptorSetLayoutBinding> bindings_;  ///< Binding information
};

/// Builder for creating descriptor set layouts
/// This class provides a fluent interface for building descriptor set layouts by
/// adding bindings one at a time. This is more convenient than manually constructing
/// VkDescriptorSetLayoutBinding structures.
///
/// Example usage:
/// @code
/// DescriptorSetLayoutBuilder builder(context);
/// auto layoutResult = builder
///     .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1)
///     .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1)
///     .addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1)
///     .build();
/// @endcode
class DescriptorSetLayoutBuilder {
public:
    /// Create a builder for descriptor set layouts
    /// @param context Valid VkContext pointer
    explicit DescriptorSetLayoutBuilder(VkContext* context);

    /// Add a descriptor binding to the layout
    /// @param binding Binding number (must be unique within the layout)
    /// @param type Type of descriptor (e.g., STORAGE_BUFFER, UNIFORM_BUFFER,
    /// COMBINED_IMAGE_SAMPLER)
    /// @param stages Shader stages that access this binding (e.g., COMPUTE_BIT, VERTEX_BIT |
    /// FRAGMENT_BIT)
    /// @param count Number of descriptors in this binding (default 1, use >1 for arrays)
    /// @return Reference to this builder for method chaining
    DescriptorSetLayoutBuilder& addBinding(uint32_t binding, VkDescriptorType type,
                                           VkShaderStageFlags stages, uint32_t count = 1);

    /// Build the descriptor set layout
    /// @return Result containing unique_ptr to DescriptorSetLayout or error code
    core::Result<std::unique_ptr<DescriptorSetLayout>> build();

private:
    VkContext* context_;                                  ///< Vulkan context (not owned)
    std::vector<VkDescriptorSetLayoutBinding> bindings_;  ///< Accumulated bindings
};

/// Descriptor pool for allocating descriptor sets
/// This class manages a VkDescriptorPool which serves as a pool for allocating
/// descriptor sets. The pool has a fixed capacity of descriptor sets and descriptor
/// types that it can allocate.
///
/// Descriptor pools are designed to be lightweight and can be created per-frame
/// or per-thread to avoid synchronization overhead. Pools can be reset to recycle
/// all allocated descriptor sets at once, which is more efficient than individual
/// descriptor set deallocation.
///
/// Example usage:
/// @code
/// // Create pool with capacity for 100 storage buffers and 50 uniform buffers
/// std::vector<DescriptorPool::PoolSize> poolSizes = {
///     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
///     {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 50}
/// };
///
/// auto poolResult = DescriptorPool::create(context, poolSizes, 100);
/// if (poolResult.isSuccess()) {
///     auto pool = std::move(poolResult.value());
///
///     // Allocate descriptor sets
///     auto descSet = pool->allocate(*layout);
///
///     // Later, reset the pool to recycle all allocations
///     pool->reset();
/// }
/// @endcode
class DescriptorPool {
public:
    /// Descriptor pool size specification
    struct PoolSize {
        VkDescriptorType type;  ///< Type of descriptor
        uint32_t count;         ///< Number of descriptors of this type
    };

    /// Create a descriptor pool
    /// @param context Valid VkContext pointer (must outlive this object)
    /// @param sizes Vector of pool sizes specifying capacity for each descriptor type
    /// @param maxSets Maximum number of descriptor sets that can be allocated from this pool
    /// @return Result containing unique_ptr to DescriptorPool or error code
    static core::Result<std::unique_ptr<DescriptorPool>>
    create(VkContext* context, const std::vector<PoolSize>& sizes, uint32_t maxSets);

    /// Destructor - cleans up VkDescriptorPool
    ~DescriptorPool();

    // Non-copyable, non-movable
    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;
    DescriptorPool(DescriptorPool&&) = delete;
    DescriptorPool& operator=(DescriptorPool&&) = delete;

    /// Allocate a single descriptor set from the pool
    /// @param layout Descriptor set layout defining the structure
    /// @return Result containing VkDescriptorSet handle or error code
    core::Result<VkDescriptorSet> allocate(const DescriptorSetLayout& layout);

    /// Allocate multiple descriptor sets with the same layout
    /// @param layout Descriptor set layout defining the structure
    /// @param count Number of descriptor sets to allocate
    /// @return Result containing vector of VkDescriptorSet handles or error code
    core::Result<std::vector<VkDescriptorSet>> allocateMultiple(const DescriptorSetLayout& layout,
                                                                uint32_t count);

    /// Reset the pool, recycling all allocated descriptor sets
    /// This is more efficient than freeing individual descriptor sets.
    /// After reset, all previously allocated descriptor sets become invalid.
    void reset();

    /// Get the Vulkan descriptor pool handle
    /// @return VkDescriptorPool handle
    VkDescriptorPool get() const noexcept { return pool_; }

    /// Get the maximum number of descriptor sets this pool can allocate
    /// @return Maximum number of descriptor sets
    uint32_t getMaxSets() const noexcept { return maxSets_; }

private:
    /// Private constructor - use create() instead
    explicit DescriptorPool(VkContext* context);

    /// Initialize the pool
    /// @param sizes Vector of pool sizes
    /// @param maxSets Maximum number of descriptor sets
    /// @return Result indicating success or failure
    core::Result<void> initialize(const std::vector<PoolSize>& sizes, uint32_t maxSets);

    VkContext* context_;                      ///< Vulkan context (not owned)
    VkDescriptorPool pool_ = VK_NULL_HANDLE;  ///< Vulkan descriptor pool handle
    uint32_t maxSets_ = 0;                    ///< Maximum number of descriptor sets
};

/// Descriptor set with resource binding utilities
/// This class wraps a VkDescriptorSet and provides a convenient interface for binding
/// GPU resources (buffers, images, samplers) to specific binding points. It batches
/// descriptor writes and applies them all at once with update().
///
/// Descriptor sets are allocated from descriptor pools and their lifetime is tied
/// to the pool. When the pool is reset or destroyed, all descriptor sets become invalid.
///
/// Example usage:
/// @code
/// // Allocate descriptor set
/// auto descSetResult = pool->allocate(*layout);
/// if (descSetResult.isSuccess()) {
///     DescriptorSet desc(context, descSetResult.value());
///
///     // Bind resources
///     desc.bindBuffer(0, storageBuffer, 0, bufferSize);
///     desc.bindBuffer(1, uniformBuffer, 0, sizeof(UniformData));
///
///     // Apply all bindings
///     desc.update();
///
///     // Use in command buffer
///     vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
///                            pipelineLayout, 0, 1, &desc.get(), 0, nullptr);
/// }
/// @endcode
class DescriptorSet {
public:
    /// Create a descriptor set wrapper
    /// @param context Valid VkContext pointer
    /// @param set VkDescriptorSet handle (allocated from a pool)
    DescriptorSet(VkContext* context, VkDescriptorSet set);

    /// Destructor (descriptor set is not destroyed, only the pool can destroy sets)
    ~DescriptorSet() = default;

    // Non-copyable but movable
    DescriptorSet(const DescriptorSet&) = delete;
    DescriptorSet& operator=(const DescriptorSet&) = delete;
    DescriptorSet(DescriptorSet&&) = default;
    DescriptorSet& operator=(DescriptorSet&&) = default;

    /// Bind a buffer to a descriptor binding
    /// The binding is queued and will be applied when update() is called.
    /// @param binding Binding number (must match descriptor set layout)
    /// @param buffer VkBuffer handle
    /// @param offset Offset in bytes from the start of the buffer
    /// @param range Size in bytes of the buffer region (use VK_WHOLE_SIZE for entire buffer)
    /// @param descriptorType Type of buffer descriptor (STORAGE_BUFFER, UNIFORM_BUFFER, etc.)
    void bindBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range,
                    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    /// Bind an image with sampler to a descriptor binding (for combined image samplers)
    /// The binding is queued and will be applied when update() is called.
    /// @param binding Binding number (must match descriptor set layout)
    /// @param imageView VkImageView handle
    /// @param sampler VkSampler handle
    /// @param layout Current image layout (e.g., SHADER_READ_ONLY_OPTIMAL)
    void bindImage(uint32_t binding, VkImageView imageView, VkSampler sampler,
                   VkImageLayout layout);

    /// Bind a storage image to a descriptor binding (for storage images without sampler)
    /// The binding is queued and will be applied when update() is called.
    /// @param binding Binding number (must match descriptor set layout)
    /// @param imageView VkImageView handle
    /// @param layout Current image layout (typically GENERAL for storage images)
    void bindStorageImage(uint32_t binding, VkImageView imageView, VkImageLayout layout);

    /// Apply all pending descriptor writes
    /// This must be called after binding resources and before using the descriptor set.
    /// After update(), the pending write queue is cleared and resources can be bound again.
    void update();

    /// Get the Vulkan descriptor set handle
    /// @return VkDescriptorSet handle
    VkDescriptorSet get() const noexcept { return set_; }

private:
    VkContext* context_;                               ///< Vulkan context (not owned)
    VkDescriptorSet set_;                              ///< Vulkan descriptor set handle
    std::vector<VkWriteDescriptorSet> pendingWrites_;  ///< Queued descriptor writes
    std::vector<VkDescriptorBufferInfo> bufferInfos_;  ///< Buffer info storage
    std::vector<VkDescriptorImageInfo> imageInfos_;    ///< Image info storage
};

}  // namespace axiom::gpu
