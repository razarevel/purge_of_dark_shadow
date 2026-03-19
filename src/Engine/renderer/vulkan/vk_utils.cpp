#include "Engine/renderer/vulkan/vk_utils.h"
#include <iostream>
#include <cassert>
#include <algorithm>

bool checkValidation() {
	uint32_t layerCount;
	if (vkEnumerateInstanceLayerProperties(&layerCount, nullptr) != VK_SUCCESS)
		return false;

	std::vector<VkLayerProperties> availableLayers(layerCount);
	if (vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()) != VK_SUCCESS)
		return false;

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

std::vector<const char*> getRequiredExtensions(bool enableValidtion) {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidtion)
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
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
		return func(instance, createInfo, nullptr, pDebugMessenger);

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT messenger,
	const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
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

bool isDeviceSuitable(VkPhysicalDevice device) {
	uint32_t extensionCount;
	VkResult result =
		result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);


	std::vector<VkExtensionProperties> extensions(extensionCount);
	result =  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	if (result) {
		std::cerr << "failed to enumerate device extensions" << std::endl;
		return false;
	}

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device, &properties);


	bool isSuitable = properties.apiVersion >= VK_API_VERSION_1_3;

	std::set<const char*> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (uint32_t i = 0; i < extensionCount; i++)
		for (const char* extension : deviceExtensions)
			if (strcmp(extension, extensions[i].extensionName) == 0)
				requiredExtensions.erase(extension);

	isSuitable = requiredExtensions.empty();

	return isSuitable;
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
	QueueFamilyIndices indices;

	uint32_t queueFamiliesCount;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamiliesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, queueFamilyProperties.data());

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

VkSurfaceFormatKHR chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats)
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	return availableFormats[0];
}

VkPresentModeKHR chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& presentMode : availablePresentModes)
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return presentMode;
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapChainExtent(VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
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

SwapChainSupportDetails querrySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
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
