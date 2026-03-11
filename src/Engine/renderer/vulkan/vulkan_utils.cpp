#include "Engine/renderer/vulkan/vulkan_utils.h"
#include <vector>
#include <iostream>
#include <assert.h>
#include <algorithm>
#include <set>
#include <mutex>

bool checkValidation() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers)
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		if (!layerFound)
			return false;
	}
	return true;
}

std::vector<const char*> getRequiredExtensions() {
    uint32_t glfwExtensionsCount = 0;
    const char** glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

    std::vector<const char*> extensions(glfwExtensions,
        glfwExtensions + glfwExtensionsCount);
    if (enableDebugger)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallback, void*) {
    std::cerr << "validation layer: " << pCallback->pMessage << std::endl;
    return VK_FALSE;
}

VkResult
CreateDebugUtilsMessengerEXT(VkInstance instance,
    VkDebugUtilsMessengerCreateInfoEXT* createInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        return func(instance, createInfo, nullptr, pDebugMessenger);
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
    VkDebugUtilsMessengerEXT messenger,
    const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, messenger, nullptr);
}

void populateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
    };
}

VkSurfaceFormatKHR
chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats)
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    return availableFormats[0];
}

VkPresentModeKHR chooseSwapChainPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& presentMode : availablePresentModes)
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return presentMode;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapChainExtent(VkSurfaceCapabilitiesKHR& capabilities,
    GLFWwindow* window) {
    assert(window);
    if (capabilities.minImageExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.minImageExtent;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    VkExtent2D extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
    };

    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);
    return extent;
}

SwapChainSupportDetails querrySwapChainSupport(VkPhysicalDevice device,
    VkSurfaceKHR surface) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
        &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.surfaceFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
            details.surfaceFormats.data());
    }
    uint32_t presentCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentCount,
        nullptr);
    if (presentCount != 0) {
        details.presentModes.resize(presentCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentCount,
            details.presentModes.data());
    }

    return details;
}

bool isDeviceSuitable(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
        nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
        extensions.data());

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    bool isSuitable = properties.apiVersion >= VK_API_VERSION_1_3;

    std::set<const char*> requiredExtensions(deviceExtensions.begin(),
        deviceExtensions.end());

    for (uint32_t i = 0; i < extensionCount; i++)
        for (const char* extension : deviceExtensions)
            if (strcmp(extension, extensions[i].extensionName) == 0)
                requiredExtensions.erase(extension);

    isSuitable = requiredExtensions.empty();

    return isSuitable;
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
    VkSurfaceKHR surface) {
    QueueFamilyIndices indices;
    uint32_t queueFamiliesCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount,
        nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(
        queueFamiliesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount,
        queueFamilyProperties.data());
    for (uint32_t i = 0; i < queueFamiliesCount; i++) {
        if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
            indices.graphcisFamily = i;

        VkBool32 supported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supported);

        if (supported)
            indices.presentFamily = i;

        if (indices.isComplete())
            break;
    }

    return indices;
}

namespace util {
    std::mutex mtx;
};

VkCommandBuffer beginSingleCommandBuffer(VkCommandPool cmdPool, VkDevice device) {
    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void endCmdBuffs(uint32_t size, const VkCommandBuffer* cmdBuffs, VkCommandPool cmdPool, VkDevice device, VkQueue graphicsQueue) {
    for(uint32_t i=0; i < size;i++)
        vkEndCommandBuffer(cmdBuffs[i]);

    VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = size,
      .pCommandBuffers = cmdBuffs,
    };

    queueSubmit(graphicsQueue, submitInfo);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, cmdPool, size, cmdBuffs);
}

void createBuffer(const BufferCreateInfo& info, VmaAllocator& alloc, VkBuffer& buffer, VmaAllocation& allocation, VmaAllocationInfo& allocInfo) {
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = info.size,
        .usage = info.usages,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo allocCreateInfo = {
    .flags = info.allocflags,
    .usage = info.memoryUsage,
    .priority = 1.0f,
    };

    if (vmaCreateBuffer(alloc, &bufferInfo, &allocCreateInfo, &buffer,
        &allocation, &allocInfo) != VK_SUCCESS)
        throw std::runtime_error("failed to create buffer usage");
}

void copyBuffer(VkDevice &device, VkCommandPool &cmdPool, VkQueue &graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleCommandBuffer(cmdPool, device);
    VkBufferCopy copyRegion{
     .size = size,
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    endCmdBuffs(1, &commandBuffer, cmdPool, device, graphicsQueue);
}


void updateBuffer(VmaAllocator allocator, VkBufferUsageFlags usage, VkBuffer& buffer, VmaAllocation& alloc, size_t size, const void* data) {
    if (!data)
        assert(false);

    if (usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) {
        if (vmaCopyMemoryToAllocation(allocator, data, alloc, 0, size) != VK_SUCCESS)
            throw std::runtime_error("failed to update host visible buffer");
    }
    else if (usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) {
        BufferCreateInfo bufferInfo{
        .size = size,
        .usages = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .allocflags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .memoryUsage = VMA_MEMORY_USAGE_AUTO,
        };

        VkBuffer stagingBuf;
        VmaAllocation stagingAlloc;
        VmaAllocationInfo stagingAllocInfo;
        createBuffer(bufferInfo, allocator, stagingBuf, stagingAlloc, stagingAllocInfo);

        vmaCopyMemoryToAllocation(allocator, data, stagingAlloc, 0, size);

        vmaDestroyBuffer(allocator, stagingBuf, stagingAlloc);
    }
    else 
        throw std::runtime_error("invalid buffer udpate request");
}

namespace utils {
    std::mutex mtx;
};
void queueSubmit(VkQueue queue, VkSubmitInfo info, VkFence fence) {
    std::lock_guard<std::mutex> lock(utils::mtx);

    if (vkQueueSubmit(queue, 1, &info, fence) != VK_SUCCESS)
        throw std::runtime_error("failed to submit to queue");
}