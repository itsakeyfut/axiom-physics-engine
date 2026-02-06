#include "axiom/gui/imgui_impl.hpp"

#include "axiom/core/error_code.hpp"
#include "axiom/core/logger.hpp"
#include "axiom/frontend/window.hpp"
#include "axiom/gpu/vk_instance.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace axiom::gui {

namespace {

// Check Vulkan result for ImGui backend
void checkVkResult(VkResult err) {
    if (err == VK_SUCCESS) {
        return;
    }
    AXIOM_LOG_ERROR("ImGui", "Vulkan error: %d", static_cast<int>(err));
}

}  // namespace

// Static factory method
core::Result<std::unique_ptr<ImGuiRenderer>> ImGuiRenderer::create(VkContext* context,
                                                                   Window* window) {
    if (context == nullptr) {
        return core::Result<std::unique_ptr<ImGuiRenderer>>::failure(
            core::ErrorCode::InvalidParameter, "VkContext is null");
    }
    if (window == nullptr) {
        return core::Result<std::unique_ptr<ImGuiRenderer>>::failure(
            core::ErrorCode::InvalidParameter, "Window is null");
    }

    // Create renderer with private constructor
    auto renderer = std::unique_ptr<ImGuiRenderer>(new ImGuiRenderer(context, window));

    // Initialize ImGui and backends
    auto initResult = renderer->initialize();
    if (initResult.isFailure()) {
        return core::Result<std::unique_ptr<ImGuiRenderer>>::failure(initResult.errorCode(),
                                                                     initResult.errorMessage());
    }

    AXIOM_LOG_INFO("ImGui", "Renderer initialized successfully");
    return core::Result<std::unique_ptr<ImGuiRenderer>>::success(std::move(renderer));
}

// Private constructor
ImGuiRenderer::ImGuiRenderer(VkContext* context, Window* window)
    : context_(context), window_(window) {}

// Destructor
ImGuiRenderer::~ImGuiRenderer() {
    // Wait for device to be idle before cleanup
    if (context_ != nullptr && context_->getDevice() != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(context_->getDevice());
    }

    // Shutdown ImGui backends
    if (vulkanBackendInitialized_) {
        ImGui_ImplVulkan_Shutdown();
    }
    if (glfwBackendInitialized_) {
        ImGui_ImplGlfw_Shutdown();
    }

    // Destroy descriptor pool
    if (descriptorPool_ != VK_NULL_HANDLE && context_ != nullptr) {
        vkDestroyDescriptorPool(context_->getDevice(), descriptorPool_, nullptr);
        descriptorPool_ = VK_NULL_HANDLE;
    }

    // Destroy render pass
    if (renderPass_ != VK_NULL_HANDLE && context_ != nullptr) {
        vkDestroyRenderPass(context_->getDevice(), renderPass_, nullptr);
        renderPass_ = VK_NULL_HANDLE;
    }

    // Destroy ImGui context
    if (imguiContext_ != nullptr) {
        ImGui::DestroyContext(imguiContext_);
        imguiContext_ = nullptr;
    }

    AXIOM_LOG_DEBUG("ImGui", "Renderer destroyed");
}

// Initialize ImGui and backends
core::Result<void> ImGuiRenderer::initialize() {
    // Create ImGui context
    imguiContext_ = ImGui::CreateContext();
    if (imguiContext_ == nullptr) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to create ImGui context");
    }
    ImGui::SetCurrentContext(imguiContext_);

    // Configure ImGui IO
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable keyboard navigation
    // Note: Docking is an optional feature in ImGui, enable if available
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;  // Disable imgui.ini file (optional)

    // Configure style (dark theme)
    configureStyle();

    // Initialize GLFW backend
    auto glfwResult = initializeGLFWBackend();
    if (glfwResult.isFailure()) {
        return glfwResult;
    }

    // Create descriptor pool for ImGui
    auto poolResult = createDescriptorPool();
    if (poolResult.isFailure()) {
        return poolResult;
    }

    // Initialize Vulkan backend
    auto vulkanResult = initializeVulkanBackend();
    if (vulkanResult.isFailure()) {
        return vulkanResult;
    }

    // Upload fonts to GPU
    auto fontsResult = uploadFonts();
    if (fontsResult.isFailure()) {
        return fontsResult;
    }

    return core::Result<void>::success();
}

// Initialize Vulkan backend
core::Result<void> ImGuiRenderer::initializeVulkanBackend() {
    // Setup ImGui Vulkan initialization info
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = context_->getInstance();
    initInfo.PhysicalDevice = context_->getPhysicalDevice();
    initInfo.Device = context_->getDevice();
    initInfo.QueueFamily = context_->getGraphicsQueueFamily();
    initInfo.Queue = context_->getGraphicsQueue();
    initInfo.DescriptorPool = descriptorPool_;
    initInfo.Subpass = 0;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = 2;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.CheckVkResultFn = checkVkResult;

    // Create a minimal render pass for ImGui
    // ImGui currently requires a render pass for initialization
    // even when using dynamic rendering at draw time
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkResult result = vkCreateRenderPass(context_->getDevice(), &renderPassInfo, nullptr,
                                         &renderPass);
    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("ImGui", "Failed to create render pass (VkResult: %d)",
                        static_cast<int>(result));
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to create render pass for ImGui");
    }

    initInfo.RenderPass = renderPass;

    // Initialize ImGui Vulkan backend
    if (!ImGui_ImplVulkan_Init(&initInfo)) {
        vkDestroyRenderPass(context_->getDevice(), renderPass, nullptr);
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to initialize ImGui Vulkan backend");
    }

    // Store render pass for cleanup
    renderPass_ = renderPass;

    vulkanBackendInitialized_ = true;
    AXIOM_LOG_DEBUG("ImGui", "Vulkan backend initialized");
    return core::Result<void>::success();
}

// Initialize GLFW backend
core::Result<void> ImGuiRenderer::initializeGLFWBackend() {
    GLFWwindow* glfwWindow = window_->getNativeHandle();
    if (glfwWindow == nullptr) {
        return core::Result<void>::failure(core::ErrorCode::InvalidParameter,
                                           "GLFW window handle is null");
    }

    // Initialize ImGui GLFW backend
    // installCallbacks = true: ImGui will install GLFW callbacks for input
    if (!ImGui_ImplGlfw_InitForVulkan(glfwWindow, true)) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to initialize ImGui GLFW backend");
    }

    glfwBackendInitialized_ = true;
    AXIOM_LOG_DEBUG("ImGui", "GLFW backend initialized");
    return core::Result<void>::success();
}

// Create descriptor pool for ImGui
core::Result<void> ImGuiRenderer::createDescriptorPool() {
    // Descriptor pool sizes for ImGui
    // ImGui needs uniform buffers and combined image samplers for rendering
    VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    poolInfo.pPoolSizes = poolSizes;

    VkResult result = vkCreateDescriptorPool(context_->getDevice(), &poolInfo, nullptr,
                                             &descriptorPool_);
    if (result != VK_SUCCESS) {
        AXIOM_LOG_ERROR("ImGui", "Failed to create descriptor pool (VkResult: %d)",
                        static_cast<int>(result));
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to create descriptor pool for ImGui");
    }

    AXIOM_LOG_DEBUG("ImGui", "Descriptor pool created");
    return core::Result<void>::success();
}

// Upload fonts to GPU
core::Result<void> ImGuiRenderer::uploadFonts() {
    // Create command pool for one-time commands
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = context_->getGraphicsQueueFamily();

    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkResult result = vkCreateCommandPool(context_->getDevice(), &poolInfo, nullptr, &commandPool);
    if (result != VK_SUCCESS) {
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to create command pool for font upload");
    }

    // Allocate command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    result = vkAllocateCommandBuffers(context_->getDevice(), &allocInfo, &commandBuffer);
    if (result != VK_SUCCESS) {
        vkDestroyCommandPool(context_->getDevice(), commandPool, nullptr);
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to allocate command buffer for font upload");
    }

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        vkDestroyCommandPool(context_->getDevice(), commandPool, nullptr);
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to begin command buffer for font upload");
    }

    // Upload fonts using ImGui Vulkan backend
    if (!ImGui_ImplVulkan_CreateFontsTexture()) {
        vkEndCommandBuffer(commandBuffer);
        vkDestroyCommandPool(context_->getDevice(), commandPool, nullptr);
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to create ImGui fonts texture");
    }

    // End command buffer
    result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) {
        vkDestroyCommandPool(context_->getDevice(), commandPool, nullptr);
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to end command buffer for font upload");
    }

    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    result = vkQueueSubmit(context_->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        vkDestroyCommandPool(context_->getDevice(), commandPool, nullptr);
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to submit font upload command buffer");
    }

    // Wait for upload to complete
    result = vkQueueWaitIdle(context_->getGraphicsQueue());
    if (result != VK_SUCCESS) {
        vkDestroyCommandPool(context_->getDevice(), commandPool, nullptr);
        return core::Result<void>::failure(core::ErrorCode::VulkanInitializationFailed,
                                           "Failed to wait for font upload completion");
    }

    // Clean up command pool
    vkDestroyCommandPool(context_->getDevice(), commandPool, nullptr);

    // Destroy font upload objects (they're no longer needed after upload)
    ImGui_ImplVulkan_DestroyFontsTexture();

    AXIOM_LOG_DEBUG("ImGui", "Fonts uploaded to GPU");
    return core::Result<void>::success();
}

// Configure ImGui style (dark theme)
void ImGuiRenderer::configureStyle() {
    ImGui::StyleColorsDark();

    // Optional: Customize colors for a more modern look
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    // Adjust colors for better contrast
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.40f, 0.40f, 0.40f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.26f, 0.26f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.26f, 0.26f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);

    AXIOM_LOG_DEBUG("ImGui", "Style configured (dark theme)");
}

// Start a new ImGui frame
void ImGuiRenderer::newFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

// Render ImGui draw data to command buffer
void ImGuiRenderer::render(VkCommandBuffer commandBuffer) {
    // End the current ImGui frame and generate draw data
    ImGui::Render();

    // Get draw data
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData == nullptr || drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f) {
        return;  // Nothing to render
    }

    // Render ImGui draw data to the command buffer
    ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
}

}  // namespace axiom::gui
