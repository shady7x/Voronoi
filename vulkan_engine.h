#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>

#define ENABLE_VALIDATION_LAYERS true

class VulkanEngine
{
    public:
        VkExtent2D windowExtent{ 1700, 900 };
        SDL_Window* window{ nullptr };
        VkInstance instance; // Vulkan library handle
        VkDebugUtilsMessengerEXT debugMessenger; // Vulkan debug output handle
        VkPhysicalDevice chosenGPU = VK_NULL_HANDLE; // GPU chosen as the default device
        VkDevice vulkanDevice; // Vulkan device for commands
        VkSurfaceKHR surface; // Vulkan window surface

        void init();
        void cleanup();
        void draw();
        void run();

    private:
        const std::vector< const char* > validationLayers = { 
            "VK_LAYER_KHRONOS_validation"
        };
        bool checkValidationLayersSupport();
        std::vector< const char* > getExtensions();
        void initVulkan();
        void createInstance();
        void createPhysicalAndLogicalDevice();
    
};
