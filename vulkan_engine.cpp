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
}

void VulkanEngine::cleanup()
{
    vkDestroyDevice(vulkanDevice, nullptr);
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
        windowExtent = swapChainDetails.capabilities.currentExtent;
    } else {
        int width, height;
        SDL_Vulkan_GetDrawableSize(window, &width, &height);
        windowExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        windowExtent.width = std::clamp(windowExtent.width, swapChainDetails.capabilities.minImageExtent.width, swapChainDetails.capabilities.maxImageExtent.width);
        windowExtent.height = std::clamp(windowExtent.height, swapChainDetails.capabilities.minImageExtent.height, swapChainDetails.capabilities.maxImageExtent.height);
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
    swapChainCreateInfo.imageExtent = windowExtent;
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
            chosenGPU = device;
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
            if (vkCreateDevice(chosenGPU, &deviceCreateInfo, nullptr, &vulkanDevice) != VK_SUCCESS)
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