#include "axiom/gpu/vk_instance.hpp"

#include "axiom/core/assert.hpp"
#include "axiom/core/logger.hpp"

#include <algorithm>
#include <cstring>
#include <set>

namespace axiom::gpu {

namespace {

// Validation layers to enable in debug builds
constexpr const char* kValidationLayerName = "VK_LAYER_KHRONOS_validation";

// Required device extensions
// Note: Currently no required device extensions
// VK_KHR_synchronization2 is core in Vulkan 1.3, so we don't need it as an extension
constexpr size_t kDeviceExtensionCount = 0;
constexpr const char* const* kDeviceExtensions = nullptr;

// Debug messenger callback for validation layer messages
VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    (void)messageType;
    (void)pUserData;

    // Filter out info messages in non-debug builds to reduce noise
    if (messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
#ifndef AXIOM_DEBUG
        return VK_FALSE;
#endif
    }

    // Log validation layer messages using the logger
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        AXIOM_LOG_ERROR("Vulkan", "%s", pCallbackData->pMessage);
    } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        AXIOM_LOG_WARN("Vulkan", "%s", pCallbackData->pMessage);
    } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        AXIOM_LOG_INFO("Vulkan", "%s", pCallbackData->pMessage);
    } else {
        AXIOM_LOG_TRACE("Vulkan", "%s", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

// Helper to populate debug messenger create info
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

// Helper to create debug messenger
VkResult createDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// Helper to destroy debug messenger
void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

}  // namespace

VkContext::VkContext() {
    // Note: Validation layers can cause crashes on some systems with buggy drivers
    // For now, disable them even in debug mode for compatibility
    // TODO: Make this configurable via environment variable
#ifdef AXIOM_DEBUG
    // enableValidationLayers_ = true;
    enableValidationLayers_ = false;  // Temporarily disabled due to driver issues
#else
    enableValidationLayers_ = false;
#endif
}

VkContext::~VkContext() {
    // Destroy in reverse order of creation
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
    }

    if (enableValidationLayers_ && debugMessenger_ != VK_NULL_HANDLE) {
        destroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
    }

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
    }
}

core::Result<std::unique_ptr<VkContext>> VkContext::create() {
    auto context = std::unique_ptr<VkContext>(new VkContext());

    // Step 1: Create Vulkan instance
    auto result = context->createInstance();
    if (result.isFailure()) {
        return core::Result<std::unique_ptr<VkContext>>::failure(result.errorCode(),
                                                                 result.errorMessage());
    }

    // Step 2: Setup debug messenger (if validation layers are enabled)
    if (context->enableValidationLayers_) {
        result = context->setupDebugMessenger();
        if (result.isFailure()) {
            return core::Result<std::unique_ptr<VkContext>>::failure(result.errorCode(),
                                                                     result.errorMessage());
        }
    }

    // Step 3: Select physical device
    result = context->selectPhysicalDevice();
    if (result.isFailure()) {
        return core::Result<std::unique_ptr<VkContext>>::failure(result.errorCode(),
                                                                 result.errorMessage());
    }

    // Step 4: Create logical device
    result = context->createLogicalDevice();
    if (result.isFailure()) {
        return core::Result<std::unique_ptr<VkContext>>::failure(result.errorCode(),
                                                                 result.errorMessage());
    }

    return core::Result<std::unique_ptr<VkContext>>::success(std::move(context));
}

core::Result<void> VkContext::createInstance() {
    // Check validation layer support if enabled
    if (enableValidationLayers_) {
        auto requiredLayers = getRequiredValidationLayers();
        if (!checkValidationLayerSupport(requiredLayers)) {
            return core::Result<void>::failure(
                core::ErrorCode::VulkanInitializationFailed,
                "Vulkan validation layers requested but not available");
        }
    }

    // Application info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Axiom Physics Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "Axiom";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;  // Vulkan 1.1 for broader compatibility

    // Instance create info
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Get required extensions
    auto extensions = getRequiredInstanceExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Enable validation layers if requested
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers_) {
        auto layers = getRequiredValidationLayers();
        createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames = layers.data();

        // Chain debug messenger create info for instance creation/destruction messages
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // Create instance
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance_);
    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to create Vulkan instance");
    }

    return core::Result<void>::success();
}

core::Result<void> VkContext::selectPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);

    if (deviceCount == 0) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    // Rate all devices and pick the best one
    uint32_t bestScore = 0;
    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;

    for (const auto& device : devices) {
        uint32_t score = rateDeviceSuitability(device);
        if (score > bestScore) {
            bestScore = score;
            bestDevice = device;
        }
    }

    if (bestDevice == VK_NULL_HANDLE || bestScore == 0) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to find a suitable GPU");
    }

    physicalDevice_ = bestDevice;

    // Log selected device info
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
    AXIOM_LOG_INFO("GPU", "Selected GPU: %s", properties.deviceName);

    return core::Result<void>::success();
}

core::Result<void> VkContext::createLogicalDevice() {
    // Find queue families
    if (!findQueueFamilies(physicalDevice_, graphicsFamily_, computeFamily_, transferFamily_)) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to find required queue families");
    }

    // Create queue create infos for unique queue families
    std::set<uint32_t> uniqueQueueFamilies = {graphicsFamily_, computeFamily_, transferFamily_};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Enable required device features
    VkPhysicalDeviceFeatures deviceFeatures{};
    // Add required features here as needed

    // Get required device extensions
    auto extensions = getRequiredDeviceExtensions();

    // Create logical device
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Note: Device-specific validation layers are deprecated, but set them anyway for compatibility
    if (enableValidationLayers_) {
        auto layers = getRequiredValidationLayers();
        createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames = layers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_);
    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to create logical device");
    }

    // Retrieve queue handles
    vkGetDeviceQueue(device_, graphicsFamily_, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, computeFamily_, 0, &computeQueue_);
    vkGetDeviceQueue(device_, transferFamily_, 0, &transferQueue_);

    return core::Result<void>::success();
}

core::Result<void> VkContext::setupDebugMessenger() {
    if (!enableValidationLayers_) {
        return core::Result<void>::success();
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    VkResult result = createDebugUtilsMessengerEXT(instance_, &createInfo, nullptr,
                                                   &debugMessenger_);
    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to setup debug messenger");
    }

    return core::Result<void>::success();
}

bool VkContext::findQueueFamilies(VkPhysicalDevice device, uint32_t& outGraphicsFamily,
                                  uint32_t& outComputeFamily, uint32_t& outTransferFamily) const {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // Find queue families that support required operations
    bool foundGraphics = false;
    bool foundCompute = false;
    bool foundTransfer = false;

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        const auto& queueFamily = queueFamilies[i];

        // Graphics queue (usually also supports compute and transfer)
        if (!foundGraphics && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            outGraphicsFamily = i;
            foundGraphics = true;
        }

        // Dedicated compute queue (prefer queue that doesn't support graphics)
        if (!foundCompute && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            outComputeFamily = i;
            foundCompute = true;
            // If this is a dedicated compute queue, prefer it
            if (!(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                // Keep this as the compute queue
            }
        }

        // Dedicated transfer queue (prefer queue that only supports transfer)
        if (!foundTransfer && (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)) {
            outTransferFamily = i;
            foundTransfer = true;
            // If this is a dedicated transfer queue, prefer it
            if (!(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                // Keep this as the transfer queue
            }
        }

        if (foundGraphics && foundCompute && foundTransfer) {
            break;
        }
    }

    return foundGraphics && foundCompute && foundTransfer;
}

uint32_t VkContext::rateDeviceSuitability(VkPhysicalDevice device) const {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    uint32_t score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    // Check if device supports required queue families
    uint32_t graphicsFamily, computeFamily, transferFamily;
    if (!findQueueFamilies(device, graphicsFamily, computeFamily, transferFamily)) {
        return 0;  // Device is not suitable
    }

    // Check if device supports required extensions
    if (!checkDeviceExtensionSupport(device)) {
        return 0;  // Device is not suitable
    }

    // Application can't function without these features
    // Add required feature checks here as needed

    return score;
}

bool VkContext::checkDeviceExtensionSupport(VkPhysicalDevice device) const {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         availableExtensions.data());

    auto requiredExtensions = getRequiredDeviceExtensions();

    for (const char* requiredExt : requiredExtensions) {
        bool found = false;
        for (const auto& availableExt : availableExtensions) {
            if (strcmp(requiredExt, availableExt.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> VkContext::getRequiredValidationLayers() const {
    return {kValidationLayerName};
}

std::vector<const char*> VkContext::getRequiredInstanceExtensions() const {
    std::vector<const char*> extensions;

    // Add debug utils extension if validation layers are enabled
    if (enableValidationLayers_) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Add platform-specific surface extensions if needed in the future
    // For now, we only need compute capabilities, not presentation

    return extensions;
}

std::vector<const char*> VkContext::getRequiredDeviceExtensions() const {
    std::vector<const char*> extensions;

    // Add required device extensions
    // Currently no required extensions
    if constexpr (kDeviceExtensions != nullptr) {
        for (size_t i = 0; i < kDeviceExtensionCount; i++) {
            extensions.push_back(kDeviceExtensions[i]);
        }
    }

    return extensions;
}

bool VkContext::checkValidationLayerSupport(const std::vector<const char*>& layers) const {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : layers) {
        bool found = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    return true;
}

VkPhysicalDeviceProperties VkContext::getDeviceProperties() const {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
    return properties;
}

VkPhysicalDeviceMemoryProperties VkContext::getMemoryProperties() const {
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &properties);
    return properties;
}

}  // namespace axiom::gpu
