#include <iostream>
#include <string>
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
    createPhysicalAndLogicalDevice();
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

void VulkanEngine::createPhysicalAndLogicalDevice()
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
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader)
        {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            std::vector< VkQueueFamilyProperties > queueFamilies(queueFamilyCount); 
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            for (size_t i = 0; i < queueFamilies.size(); ++i)
            {
                if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    chosenGPU = device;
                    VkDeviceQueueCreateInfo queueCreateInfo{};
                    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    queueCreateInfo.queueFamilyIndex = i;
                    queueCreateInfo.queueCount = 1;
                    float queuePriority = 1.0f;
                    queueCreateInfo.pQueuePriorities = &queuePriority;

                    VkPhysicalDeviceFeatures df{};
                    VkDeviceCreateInfo deviceCreateInfo{};
                    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
                    deviceCreateInfo.queueCreateInfoCount = 1;
                    deviceCreateInfo.pEnabledFeatures = &df;

                    if (vkCreateDevice(chosenGPU, &deviceCreateInfo, nullptr, &vulkanDevice) != VK_SUCCESS)
                    {
                        throw std::runtime_error("Failed to create logical device");
                    }

                    return;
                }
            }
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