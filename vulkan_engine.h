#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <array>
#include <fstream>
#include <optional>

#ifdef NDEBUG
    #define ENABLE_VALIDATION_LAYERS false
#else
    #define ENABLE_VALIDATION_LAYERS true
#endif

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

struct Matrices {
    glm::mat4 mvp;
    glm::mat4 mv;
    glm::mat4 normal;
};

struct LightInfo {
    glm::vec4 position;
    glm::vec4 color;
};

struct UniformBufferObject {
    VkDeviceSize size;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec3 outline;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array< VkVertexInputAttributeDescription, 4 > getAttributeDescriptions() {
        std::array< VkVertexInputAttributeDescription, 4 > attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, normal);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, outline);

        return attributeDescriptions;
    }
};

class VulkanEngine {
    public:
        std::vector< Vertex > vertices;
        std::vector< uint32_t > indices;
        void run();
    private:
        const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
        std::atomic< uint32_t > WIDTH = 1600;
        std::atomic< uint32_t > HEIGHT = 900;

        std::atomic< float > moveX = 0, moveY = 0, moveZ = 0;
        
        MVP mvp {
            glm::mat4(1.0f),
            glm::lookAt(glm::vec3(0.0f, -0.9f, -0.9f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::perspective(glm::radians(45.0f), WIDTH / (float) HEIGHT, 0.1f, 10.0f)
        };

        LightInfo lightInfo { 
            glm::vec4(1.0, 1.0, -0.3, 1.0), 
            glm::vec4(1, 1, 1, 0.2) 
        };

        uint32_t currentFrame = 0;

        SDL_Window* window;
        std::mutex windowMutex;
        std::condition_variable windowCv;

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;

        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice vulkanDevice;

        VkQueue graphicsQueue;
        VkQueue presentQueue;

        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> swapChainFramebuffers;

        VkImage depthImage;
        VkImageView depthImageView;
        VkDeviceMemory depthImageMemory;

        VkRenderPass renderPass;

        std::array<VkDescriptorSetLayout, 2> descriptorSetLayout;
        VkDescriptorPool descriptorPool;
        std::array<std::vector<VkDescriptorSet>, 2> descriptorSets;

        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;

        VkCommandPool commandPool;
        std::vector< VkCommandBuffer > commandBuffers;

        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;

        std::vector<UniformBufferObject> ubo { 
            { sizeof(Matrices), std::vector<VkBuffer>(MAX_FRAMES_IN_FLIGHT), std::vector<VkDeviceMemory>(MAX_FRAMES_IN_FLIGHT), std::vector<void*>(MAX_FRAMES_IN_FLIGHT) },
            { sizeof(LightInfo), std::vector<VkBuffer>(MAX_FRAMES_IN_FLIGHT), std::vector<VkDeviceMemory>(MAX_FRAMES_IN_FLIGHT), std::vector<void*>(MAX_FRAMES_IN_FLIGHT) }
        };

        std::vector< VkSemaphore > imageAvailableSemaphores;
        std::vector< VkSemaphore > renderFinishedSemaphores;
        std::vector< VkFence > inFlightFences;

        std::atomic< bool > running = true;
        std::atomic< bool > windowVisible = true;

        void initWindow();
        void initVulkan();
        void inputLoop();
        void drawLoop();
        void cleanupSwapChain();
        void cleanup();

        void createInstance();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createDescriptorSetLayout();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandPool();
        void createDepthResources();
        void createVertexBuffer();
        void createIndexBuffer();
        void createUniformBuffers();
        void createDescriptorPool();
        void createDescriptorSets();
        void createCommandBuffer();
        void createSyncObjects();

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void recreateSwapChain();
        void updateUniformBuffer(uint32_t currentImage);
        void drawFrame();

        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);
        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        
        VkFormat findSupportedFormat(const std::vector<VkFormat>&candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkFormat findDepthFormat();
        bool hasStencilComponent(VkFormat format);

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        VkShaderModule createShaderModule(const std::vector<char> &code);
        QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice &device);
        SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice &device);
        std::vector<const char *> getRequiredExtensions();

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
            switch (messageSeverity) {
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                    //std::cout << "[VERBOSE] ";
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                    std::cout << "[INFO] ";
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                    std::cout << "[WARN] ";
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                    std::cout << "[ERROR] ";
                    break;
                default:
                    std::cout << "[UNKNOWN] ";
            }
            if (messageSeverity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
                std::cout << pCallbackData->pMessage << std::endl;
            return VK_FALSE;
        }

        static std::vector<char> readFile(const std::string& filename) {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open file!");
            }
            size_t fileSize = (size_t) file.tellg();
            std::vector<char> buffer(fileSize);
            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();
            return buffer;
        }
};