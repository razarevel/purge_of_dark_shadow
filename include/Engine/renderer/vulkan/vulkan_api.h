#pragma once

#include "vk_utils.h"

#include "Engine/settings.h"


struct VulkanApi {
	VulkanApi(GLFWwindow* window, Settings& settings);
	~VulkanApi();

	bool init();

	bool enableDebugger = true;

private:

	GLFWwindow* window = nullptr;
	Settings settings;

	VkInstance instance;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	QueueFamilyIndices indices;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	bool createInstance();
	bool setupDebugger();
	bool createSurfaceKHR();
	bool pickPhysicalDevice();
	bool createLogicalDevice();

};