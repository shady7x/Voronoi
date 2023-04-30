#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <set>

#define ENABLE_VALIDATION_LAYERS true

class VulkanEngine
{
    public:
        VkExtent2D windowExtent{ 1700, 900 };
        SDL_Window* window{ nullptr };
        VkInstance instance; // Vulkan library handle
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE; // Vulkan debug output handle
        VkPhysicalDevice chosenGPU = VK_NULL_HANDLE; // GPU chosen as the default device
        VkDevice vulkanDevice = VK_NULL_HANDLE; // Vulkan device for commands
        VkSurfaceKHR surface; // Vulkan window surface
        VkFormat swapChainImageFormat;
        VkSwapchainKHR swapChain;
        std::vector< VkImage > swapChainImages;
        std::vector< VkImageView > swapChainImageViews;
        VkQueue graphicsQueue;
        VkQueue presentQueue;

        void init();
        void cleanup();
        void draw();
        void run();

    private:
        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;

            bool isComplete() {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }

            std::vector< VkDeviceQueueCreateInfo > getQueueCreateInfos() {
                std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
                std::set< uint32_t > uniqueQueueFamilies = { graphicsFamily.value(), presentFamily.value() };
                float queuePriority = 1.0f;
                for (auto queueFamily : uniqueQueueFamilies)
                {
                    VkDeviceQueueCreateInfo queueCreateInfo{};
                    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    queueCreateInfo.queueFamilyIndex = queueFamily;
                    queueCreateInfo.queueCount = 1;
                    queueCreateInfo.pQueuePriorities = &queuePriority;
                    queueCreateInfos.push_back(queueCreateInfo);
                }
                return queueCreateInfos;
            }
        };

        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector< VkSurfaceFormatKHR > formats;
            std::vector< VkPresentModeKHR > presentModes;

            bool isComplete() {
                return !formats.empty() && !presentModes.empty();
            }
        };

        const std::vector< const char* > validationLayers = { 
            "VK_LAYER_KHRONOS_validation"
        };
        const std::vector< const char* > deviceExtensions = { 
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        bool checkValidationLayersSupport();
        bool checkDeviceExtensionsSupport(const VkPhysicalDevice& device);
        QueueFamilyIndices getQueueFamilyIndices(const VkPhysicalDevice& device);
        SwapChainSupportDetails getSwapChainSupport(const VkPhysicalDevice& device);
        void createSwapChain(const SwapChainSupportDetails& swapChainDetails, const QueueFamilyIndices& queueFamilyIndices);
        std::vector< const char* > getExtensions();

        void initVulkan();
        void createInstance();
        void createSurface();
        void pickPhysicalAndCreateLogicalDevice();
        void createImageViews();

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackFunc(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
            return VK_FALSE;
        }
};
