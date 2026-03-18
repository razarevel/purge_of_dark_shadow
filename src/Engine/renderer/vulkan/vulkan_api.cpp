#include "Engine/renderer/vulkan/vulkan_api.h"
#include <iostream>

VulkanApi::VulkanApi(GLFWwindow* window, Settings& settings): window(window), settings(settings) {}

bool VulkanApi::init() {
	if (!createInstance())
		return false;

	if (!setupDebugger())
		return false;

	
	if (!createSurfaceKHR())
		return false;

	if (!pickPhysicalDevice())
		return false;

	if (!createLogicalDevice())
		return false;

	return true;
}

bool VulkanApi::createInstance() {
	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = settings.appName.c_str(),
		.applicationVersion = VK_API_VERSION_1_0,
		.pEngineName = settings.appName.c_str(),
		.engineVersion = VK_API_VERSION_1_0,
		.apiVersion = VK_API_VERSION_1_3,
	};

	if (enableDebugger && !checkValidation()) {
		std::cerr << "validation layer requested but not available" << std::endl;
		return false;
	}

	auto extensions = getRequiredExtensions(enableDebugger);

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = (uint32_t)extensions.size(),
		.ppEnabledExtensionNames = extensions.data(),
	};

	VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
	if (enableDebugger) {
		createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
		populateDebugMessenger(debugInfo);
		createInfo.pNext = &debugInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		std::cerr << "failed to create vulkan instance" << std::endl;
		return false;
	}

	return true;
}

bool VulkanApi::setupDebugger() {
	if (!enableDebugger)
		return true;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	populateDebugMessenger(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		std::cerr << "failed to create vulkan debug utils" << std::endl;
		return false;
	}
	return true;
}

bool VulkanApi::createSurfaceKHR() {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		std::cerr << "failed to create window surface for vulkan" << std::endl;
		return false;
	}

	return true;
}

bool VulkanApi::pickPhysicalDevice() {
	uint32_t deviceCount;
	if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS)  {
		std::cerr << "failed to fetch physical device with vk support" << std::endl;
		return false;
	}

	if (deviceCount == 0) {
		std::cerr << "failed to find any device with vulkan support" << std::endl;
		return false;
	}

	std::vector<VkPhysicalDevice> devices;
	devices.resize(deviceCount);
	if (vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()) != VK_SUCCESS) {
		std::cerr << "failed to fetch physical device with vk support" << std::endl;
		return false;
	}

	for(uint32_t i=0; i < deviceCount; i++)
		if (isDeviceSuitable(devices[i])) {
			physicalDevice = devices[i];
			break;
		}
	if (physicalDevice == VK_NULL_HANDLE) {
		std::cerr << "failed to fetch physical device with vk support" << std::endl;
		return false;
	}

	return true;
}

bool VulkanApi::createLogicalDevice() {
	indices = findQueueFamilies(physicalDevice, surface);

	std::set<uint32_t> uniqueQueueFamilies = { indices.graphcisFamily.value(), indices.presentFamily.value() };

	std::vector<VkDeviceQueueCreateInfo> deviceQueueInfos;
	float queuePriority = 0.5f;

	for (const auto& uniqueQueue : uniqueQueueFamilies)
		deviceQueueInfos.emplace_back(VkDeviceQueueCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = uniqueQueue,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority,
			});
	
	VkPhysicalDeviceBufferDeviceAddressFeatures bda{
	  .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
	  .bufferDeviceAddress = VK_TRUE,
	};

	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
		.pNext = &bda,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
		.descriptorBindingPartiallyBound = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
	};

	VkPhysicalDeviceFeatures deviceFeatures{
		.geometryShader = VK_TRUE,
		.tessellationShader = VK_TRUE,
		.depthBiasClamp = VK_TRUE,
		.fillModeNonSolid = VK_TRUE,
		.samplerAnisotropy = VK_TRUE,
	};

	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateFeatues = {
		.sType =
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
		.pNext = &indexingFeatures,
		.extendedDynamicState = true,
	};

	VkPhysicalDeviceVulkan13Features vulkan13Features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &dynamicStateFeatues,
		.synchronization2 = true,
		.dynamicRendering = true,
	};

	VkPhysicalDeviceVulkan11Features vulkan11Features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		.pNext = &vulkan13Features,
		.storageBuffer16BitAccess = VK_TRUE,
	};

	VkPhysicalDeviceFeatures2 deviceFeatures2 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &vulkan11Features,
		.features = deviceFeatures,
	};

	VkDeviceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &deviceFeatures2,
		.queueCreateInfoCount = (uint32_t)deviceQueueInfos.size(),
		.pQueueCreateInfos = deviceQueueInfos.data(),
		.enabledExtensionCount = (uint32_t)deviceExtensions.size(),
		.ppEnabledExtensionNames = deviceExtensions.data(),
	};

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		std::cerr << "failed to create vk logical device" << std::endl;
		return false;
	}

	vkGetDeviceQueue(device, indices.graphcisFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

	return true;
}

VulkanApi::~VulkanApi() {

	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

	vkDestroyInstance(instance, nullptr);
}