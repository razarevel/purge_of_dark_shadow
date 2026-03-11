#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>
#include "Engine/vk_mem_alloc.h"


const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME,
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
};

#ifdef _DEBUG
constexpr bool enableDebugger = true;
#else
constexpr bool enableDebugger = false;
#endif

bool checkValidation();

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphcisFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() const {
        return graphcisFamily.has_value() && presentFamily.has_value();
    }
};

std::vector<const char*> getRequiredExtensions();

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallback, void*);

VkResult
CreateDebugUtilsMessengerEXT(VkInstance instance,
    VkDebugUtilsMessengerCreateInfoEXT* createInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
    VkDebugUtilsMessengerEXT messenger,
    const VkAllocationCallbacks* pAllocator);

void populateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

bool isDeviceSuitable(VkPhysicalDevice device);

VkSurfaceFormatKHR chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

VkPresentModeKHR chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

VkExtent2D chooseSwapChainExtent(VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

SwapChainSupportDetails querrySwapChainSupport(VkPhysicalDevice device,
    VkSurfaceKHR surface);

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
    VkSurfaceKHR surface);

struct BufferCreateInfo {
    VkDeviceSize size;
    VkBufferUsageFlags usages;
    VmaAllocatorCreateFlags allocflags;
    VmaMemoryUsage memoryUsage; 
    VkMemoryPropertyFlags propertyFlags;
};

VkCommandBuffer beginSingleCommandBuffer(VkCommandPool cmdPool, VkDevice device);

void endCmdBuffs(uint32_t size, const VkCommandBuffer* cmdBuffs, VkCommandPool cmdPool, VkDevice device, VkQueue graphicsQueue);

void createBuffer(const BufferCreateInfo& info, VmaAllocator& alloc, VkBuffer& buffer, VmaAllocation& allocation, VmaAllocationInfo &allocInfo);
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
void updateBuffer(VmaAllocator allocator, VkBufferUsageFlags usage, VkBuffer& buffer, VmaAllocation& alloc, size_t size, const void* data);

void queueSubmit(VkQueue queue,  VkSubmitInfo info, VkFence fence = VK_NULL_HANDLE);