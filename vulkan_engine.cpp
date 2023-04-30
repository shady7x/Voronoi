#include <iostream>
#include <string>
#include <algorithm>
#include "vulkan_engine.h"

void VulkanEngine::init()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) 
    {
        throw std::runtime_error(SDL_GetError());
	}

	const int SDL_SCREEN_WIDTH = 800;
	const int SDL_SCREEN_HEIGHT = 600;

	window = SDL_CreateWindow("Hello, SDL 2!", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        SDL_SCREEN_WIDTH, SDL_SCREEN_HEIGHT, 
        SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

	if (window == nullptr) 
    {
        throw std::runtime_error(SDL_GetError());
	}

    initVulkan();
}

void VulkanEngine::initVulkan()
{
    createInstance();
    createSurface();
    pickPhysicalAndCreateLogicalDevice();
    createImageViews();
    createRenderPass();
    createGraphicPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
}

void VulkanEngine::cleanup()
{
    vkDestroyCommandPool(vulkanDevice, commandPool, nullptr);
    for (auto framebuffer : swapChainFramebuffers)
    {
        vkDestroyFramebuffer(vulkanDevice, framebuffer, nullptr);
    }
    vkDestroyPipeline(vulkanDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(vulkanDevice, pipelineLayout, nullptr);
    vkDestroyRenderPass(vulkanDevice, renderPass, nullptr);
    for (auto imageView : swapChainImageViews)
    {
        vkDestroyImageView(vulkanDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(vulkanDevice, swapChain, nullptr);
    vkDestroyDevice(vulkanDevice, nullptr);
    if (debugMessenger != VK_NULL_HANDLE)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) 
        {
            func(instance, debugMessenger, nullptr);
        }
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    if (window != nullptr)
    {
        SDL_DestroyWindow(window);
    }
}

void VulkanEngine::draw()
{

}

void VulkanEngine::run()
{
    SDL_Event e;
    bool isRunning = true;
    while (isRunning)
    {
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                isRunning = false;
            }
        }

        draw();
    }
}

bool VulkanEngine::checkValidationLayersSupport()
{
    uint32_t layersCount = 0;
	if (vkEnumerateInstanceLayerProperties(&layersCount, nullptr) != VK_SUCCESS)
	{
		std::cout << "Unable to query Vulkan instance layers count\n";
		return false;
	}

	std::vector< VkLayerProperties > availableLayers(layersCount);
    if (vkEnumerateInstanceLayerProperties(&layersCount, availableLayers.data()) != VK_SUCCESS)
    {
        std::cout << "Unable to query Vulkan instance layers\n";
		return false;
    }

    for (const char* layer : validationLayers)
    {
        bool foundLayer = false;
        for (const auto& availableLayer : availableLayers)
        {
            if (strcmp(layer, availableLayer.layerName) == 0) 
            {
                foundLayer = true;
                break;
            }
        }
        if (foundLayer == false)
        {
            std::cout << "Layer not found " << layer << std::endl;
            return false;
        }
    }

	return true;
}

std::vector< const char* > VulkanEngine::getExtensions() {
    uint32_t extCount = 0;
	if (!SDL_Vulkan_GetInstanceExtensions(window, &extCount, nullptr))
	{
        throw std::runtime_error("Unable to query Vulkan instance extensions");
	}
    
	std::vector< const char* > extensions(extCount);
	if (!SDL_Vulkan_GetInstanceExtensions(window, &extCount, extensions.data()))
	{
        throw std::runtime_error("Unable to query Vulkan instance extensions names");
	}
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	//extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    for (auto extension : extensions)
    {
        std::cout << extension << std::endl;
    }

    return extensions;
}

void VulkanEngine::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Voronoi";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.pEngineName = "no engine";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getExtensions();
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (!validationLayers.empty() && checkValidationLayersSupport()) {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
        debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugMessengerCreateInfo.pfnUserCallback = debugCallbackFunc;
        createInfo.pNext = &debugMessengerCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create vulkan instance");
    }

    if (createInfo.pNext != nullptr)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"); 
        if (func == nullptr || func(instance, (VkDebugUtilsMessengerCreateInfoEXT*) createInfo.pNext, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create debug messenger, func is null: " + std::to_string(func == nullptr));
        }
    }
}

bool VulkanEngine::checkDeviceExtensionsSupport(const VkPhysicalDevice& device)
{
    uint32_t deviceExtensionsCount;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionsCount, nullptr) != VK_SUCCESS || deviceExtensionsCount == 0)
    {
        std::cout << "Failed to get device extensions count" << std::endl;
        return false;
    }
    std::vector< VkExtensionProperties > availableDeviceExtensions(deviceExtensionsCount);
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionsCount, availableDeviceExtensions.data()) != VK_SUCCESS)
    {
        std::cout << "Failed to get available device extensions" << std::endl;
        return false;
    }
    for (const auto& deviceExtension : deviceExtensions)
    {
        bool extensionSupported = false;
        for (const auto& availableDeviceExtension : availableDeviceExtensions)
        {
            if (strcmp(deviceExtension, availableDeviceExtension.extensionName) == 0)
            {
                extensionSupported = true;
                break;
            }
        }
        if (!extensionSupported)
        {
            std::cout << "Unsupported device extension: " << deviceExtension << std::endl;
            return false;
        }
    }
    return true;
}

VulkanEngine::QueueFamilyIndices VulkanEngine::getQueueFamilyIndices(const VkPhysicalDevice& device)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector< VkQueueFamilyProperties > queueFamilies(queueFamilyCount); 
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    QueueFamilyIndices queueFamilyIndices;
    for (uint32_t i = 0; i < queueFamilies.size(); ++i)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queueFamilyIndices.graphicsFamily = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
        {
            queueFamilyIndices.presentFamily = i;
        }
        if (queueFamilyIndices.isComplete())
        {
            break;
        }
    }
    return queueFamilyIndices;
}

VulkanEngine::SwapChainSupportDetails VulkanEngine::getSwapChainSupport(const VkPhysicalDevice& device)
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    uint32_t formatCount = 0;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr) == VK_SUCCESS && formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount = 0;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr) == VK_SUCCESS && presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

void VulkanEngine::createSwapChain(const SwapChainSupportDetails& swapChainDetails, const QueueFamilyIndices& queueFamilyIndices)
{
    VkSurfaceFormatKHR surfaceFormat = swapChainDetails.formats[0];
    for (const auto& availableFormat : swapChainDetails.formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
        }
    }

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availablePresentMode : swapChainDetails.presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = availablePresentMode;
        }
    }
    
    if (swapChainDetails.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        swapChainExtent = swapChainDetails.capabilities.currentExtent;
    } else {
        int width, height;
        SDL_Vulkan_GetDrawableSize(window, &width, &height);
        swapChainExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        swapChainExtent.width = std::clamp(swapChainExtent.width, swapChainDetails.capabilities.minImageExtent.width, swapChainDetails.capabilities.maxImageExtent.width);
        swapChainExtent.height = std::clamp(swapChainExtent.height, swapChainDetails.capabilities.minImageExtent.height, swapChainDetails.capabilities.maxImageExtent.height);
    }
    
    uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;
    if (swapChainDetails.capabilities.maxImageCount > 0 && imageCount > swapChainDetails.capabilities.maxImageCount)
    {
        imageCount = swapChainDetails.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = surface;
    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = swapChainExtent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (queueFamilyIndices.graphicsFamily.value() != queueFamilyIndices.presentFamily.value()) {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = new uint32_t[2] { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };
    } else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    swapChainCreateInfo.preTransform = swapChainDetails.capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(vulkanDevice, &swapChainCreateInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain");
    }
    vkGetSwapchainImagesKHR(vulkanDevice, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vulkanDevice, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
}

void VulkanEngine::pickPhysicalAndCreateLogicalDevice()
{
    uint32_t deviceCount = 0;
    if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0)
    {
        throw std::runtime_error("Unable to get physical devices count");
    }
    std::vector< VkPhysicalDevice > devices(deviceCount);
    if (vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Unable to get physical devices");
    }

    for (const auto& device : devices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            if (!checkDeviceExtensionsSupport(device)) continue;
            QueueFamilyIndices queueFamilyIndices = getQueueFamilyIndices(device);
            if (!queueFamilyIndices.isComplete()) continue;
            SwapChainSupportDetails swapChainDetails = getSwapChainSupport(device);
            if (!swapChainDetails.isComplete()) continue;
            physicalDevice = device;
            auto queueCreateInfos = queueFamilyIndices.getQueueCreateInfos();
            VkPhysicalDeviceFeatures deviceFeatures{};
            VkDeviceCreateInfo deviceCreateInfo{};
            deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
            deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
            deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
            deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
            deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
            if (validationLayers.empty())
            {
                deviceCreateInfo.enabledLayerCount = 0;
            }
            else 
            {
                deviceCreateInfo.enabledLayerCount = validationLayers.size();
                deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
            }
            if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &vulkanDevice) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create logical device");
            }
            vkGetDeviceQueue(vulkanDevice, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
            vkGetDeviceQueue(vulkanDevice, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
            createSwapChain(swapChainDetails, queueFamilyIndices);
            return;
        }
    }
    throw std::runtime_error("No suitable GPU found");
}

void VulkanEngine::createSurface()
{
    if (SDL_Vulkan_CreateSurface(window, instance, &surface) == SDL_FALSE)
    {
        throw std::runtime_error("Failed to create Vulkan surface");
    }
}

void VulkanEngine::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); ++i)
    {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapChainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = swapChainImageFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(vulkanDevice, &imageViewCreateInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image view #" + std::to_string(i));
        }
    }
}

void VulkanEngine::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;

    if (vkCreateRenderPass(vulkanDevice, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass");
    }
}

void VulkanEngine::createGraphicPipeline()
{
    auto vertexShader = readFile("C:/Users/Home/source/repos/Voronoi/shaders/vertex.spv");
    auto fragmentShader = readFile("C:/Users/Home/source/repos/Voronoi/shaders/fragment.spv");
    auto vertexShaderModule = createShaderModule(vertexShader);
    auto fragmentShaderModule = createShaderModule(fragmentShader);

    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = vertexShaderModule;
    vertexShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
    fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStageInfo.module = fragmentShaderModule;
    fragmentShaderStageInfo.pName = "main";

    std::vector< VkPipelineShaderStageCreateInfo > shaderStages = { vertexShaderStageInfo, fragmentShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo inputStateCreateInfo{};
    inputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inputStateCreateInfo.vertexBindingDescriptionCount = 0;
    inputStateCreateInfo.pVertexBindingDescriptions = nullptr;
    inputStateCreateInfo.vertexAttributeDescriptionCount = 0;
    inputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = swapChainExtent.width;
    viewport.height = swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissors{};
    scissors.offset = { 0, 0 };
    scissors.extent = swapChainExtent;
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissors;

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentState.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

    std::vector< VkDynamicState > dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(vulkanDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.stageCount = shaderStages.size();
    graphicsPipelineCreateInfo.pStages = shaderStages.data();
    graphicsPipelineCreateInfo.pVertexInputState = &inputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    graphicsPipelineCreateInfo.layout = pipelineLayout;
    graphicsPipelineCreateInfo.renderPass = renderPass;
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    
    if (vkCreateGraphicsPipelines(vulkanDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    vkDestroyShaderModule(vulkanDevice, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(vulkanDevice, vertexShaderModule, nullptr);
}

VkShaderModule VulkanEngine::createShaderModule(const std::vector< char >& shader)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = shader.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast< const uint32_t* >(shader.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(vulkanDevice, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module");
    }
    return shaderModule;
}

void VulkanEngine::createFramebuffers()
{
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); ++i)
    {
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = attachments; // TODO &
        framebufferCreateInfo.width = swapChainExtent.width;
        framebufferCreateInfo.height = swapChainExtent.height;
        framebufferCreateInfo.layers = 1;

        if (vkCreateFramebuffer(vulkanDevice, &framebufferCreateInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
}

void VulkanEngine::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = getQueueFamilyIndices(physicalDevice);

    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    commandPoolCreateInfo.flags = 0;

    if (vkCreateCommandPool(vulkanDevice, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool");
    }
}

void VulkanEngine::createCommandBuffer()
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;
}