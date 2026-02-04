#pragma once

#include "axiom/core/result.hpp"

#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace axiom::gpu {

/// Vulkan context that manages instance, physical device, and logical device
/// This class provides a high-level interface to initialize Vulkan for GPU compute
/// operations. It handles instance creation with validation layers (in debug builds),
/// physical device selection with discrete GPU preference, logical device creation
/// with graphics/compute/transfer queues, and debug messenger setup.
///
/// Example usage:
/// @code
/// auto contextResult = VkContext::create();
/// if (contextResult.isSuccess()) {
///     VkContext& context = contextResult.value();
///     VkDevice device = context.getDevice();
///     VkQueue computeQueue = context.getComputeQueue();
///     // Use device and queues for GPU operations...
/// }
/// @endcode
class VkContext {
public:
    /// Create a new Vulkan context
    /// This performs full Vulkan initialization including instance creation,
    /// physical device selection, logical device creation, and debug setup.
    /// @return Result containing the VkContext on success, or error code on failure
    static core::Result<std::unique_ptr<VkContext>> create();

    /// Destructor - cleans up all Vulkan resources
    ~VkContext();

    // Disable copy and move operations (Vulkan handles are not copyable)
    VkContext(const VkContext&) = delete;
    VkContext& operator=(const VkContext&) = delete;
    VkContext(VkContext&&) = delete;
    VkContext& operator=(VkContext&&) = delete;

    /// Get the Vulkan instance
    /// @return The VkInstance handle
    VkInstance getInstance() const noexcept { return instance_; }

    /// Get the selected physical device
    /// @return The VkPhysicalDevice handle
    VkPhysicalDevice getPhysicalDevice() const noexcept { return physicalDevice_; }

    /// Get the logical device
    /// @return The VkDevice handle
    VkDevice getDevice() const noexcept { return device_; }

    /// Get the graphics queue
    /// @return The graphics queue handle
    VkQueue getGraphicsQueue() const noexcept { return graphicsQueue_; }

    /// Get the compute queue
    /// @return The compute queue handle
    VkQueue getComputeQueue() const noexcept { return computeQueue_; }

    /// Get the transfer queue
    /// @return The transfer queue handle
    VkQueue getTransferQueue() const noexcept { return transferQueue_; }

    /// Get the graphics queue family index
    /// @return The queue family index for graphics operations
    uint32_t getGraphicsQueueFamily() const noexcept { return graphicsFamily_; }

    /// Get the compute queue family index
    /// @return The queue family index for compute operations
    uint32_t getComputeQueueFamily() const noexcept { return computeFamily_; }

    /// Get the transfer queue family index
    /// @return The queue family index for transfer operations
    uint32_t getTransferQueueFamily() const noexcept { return transferFamily_; }

    /// Get physical device properties
    /// @return The physical device properties structure
    VkPhysicalDeviceProperties getDeviceProperties() const;

    /// Get physical device memory properties
    /// @return The memory properties structure
    VkPhysicalDeviceMemoryProperties getMemoryProperties() const;

    /// Check if validation layers are enabled
    /// @return true if validation layers are active (debug builds)
    bool hasValidationLayers() const noexcept { return enableValidationLayers_; }

private:
    /// Private constructor - use create() instead
    VkContext();

    /// Create the Vulkan instance with validation layers
    /// @return Result indicating success or failure with error details
    core::Result<void> createInstance();

    /// Select the best physical device (GPU)
    /// Prefers discrete GPUs over integrated ones
    /// @return Result indicating success or failure with error details
    core::Result<void> selectPhysicalDevice();

    /// Create the logical device with required queues
    /// @return Result indicating success or failure with error details
    core::Result<void> createLogicalDevice();

    /// Setup debug messenger for validation layer output
    /// Only active when validation layers are enabled
    /// @return Result indicating success or failure with error details
    core::Result<void> setupDebugMessenger();

    /// Find queue families that support required operations
    /// @param device The physical device to query
    /// @param outGraphicsFamily Output parameter for graphics queue family index
    /// @param outComputeFamily Output parameter for compute queue family index
    /// @param outTransferFamily Output parameter for transfer queue family index
    /// @return true if all required queue families were found
    bool findQueueFamilies(VkPhysicalDevice device, uint32_t& outGraphicsFamily,
                           uint32_t& outComputeFamily, uint32_t& outTransferFamily) const;

    /// Rate a physical device's suitability (higher is better)
    /// Discrete GPUs receive higher scores than integrated GPUs
    /// @param device The physical device to rate
    /// @return Score value (0 means unsuitable)
    uint32_t rateDeviceSuitability(VkPhysicalDevice device) const;

    /// Check if required extensions are supported by a physical device
    /// @param device The physical device to check
    /// @return true if all required extensions are available
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

    /// Get the list of required validation layers
    /// @return Vector of validation layer names
    std::vector<const char*> getRequiredValidationLayers() const;

    /// Get the list of required instance extensions
    /// @return Vector of instance extension names
    std::vector<const char*> getRequiredInstanceExtensions() const;

    /// Get the list of required device extensions
    /// @return Vector of device extension names
    std::vector<const char*> getRequiredDeviceExtensions() const;

    /// Check if validation layers are available
    /// @param layers The list of layer names to check
    /// @return true if all layers are available
    bool checkValidationLayerSupport(const std::vector<const char*>& layers) const;

    // Vulkan handles
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;

    // Queue handles
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue computeQueue_ = VK_NULL_HANDLE;
    VkQueue transferQueue_ = VK_NULL_HANDLE;

    // Queue family indices
    uint32_t graphicsFamily_ = UINT32_MAX;
    uint32_t computeFamily_ = UINT32_MAX;
    uint32_t transferFamily_ = UINT32_MAX;

    // Configuration
    bool enableValidationLayers_ = false;
};

}  // namespace axiom::gpu
