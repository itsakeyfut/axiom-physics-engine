#include "axiom/gpu/vk_instance.hpp"

#include <iostream>

int main() {
    std::cout << "Creating Vulkan context..." << std::endl;

    auto result = axiom::gpu::VkContext::create();

    if (result.isSuccess()) {
        std::cout << "Success! Vulkan context created." << std::endl;
        auto& context = result.value();

        auto props = context->getDeviceProperties();
        std::cout << "GPU: " << props.deviceName << std::endl;
        std::cout << "API Version: " << VK_VERSION_MAJOR(props.apiVersion) << "."
                  << VK_VERSION_MINOR(props.apiVersion) << "." << VK_VERSION_PATCH(props.apiVersion)
                  << std::endl;

        return 0;
    } else {
        std::cerr << "Failed to create Vulkan context: " << result.errorMessage() << std::endl;
        return 1;
    }
}
