#include "Engine/renderer/vulkan/vulkan_api.h"
#include <iostream>
#include <cassert>

VulkanApi::VulkanApi(GLFWwindow* window, Settings& settings, bool enableDeb): window(window), settings(settings), enableDebugger(enableDeb) {}

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

	if (!createVmaAllocation())
		return false;

	if (!createSwapChain())
		return false;
	
	if (!createSwapChainImageViews())
		return false;

	if (!createSyncObj())
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

bool VulkanApi::createVmaAllocation() {
	VmaAllocatorCreateInfo createInfo = {
		.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT |
			   VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = physicalDevice,
		.device = device,
		.instance = instance,
		.vulkanApiVersion = VK_API_VERSION_1_3,
	};

	if (vmaCreateAllocator(&createInfo, &allocator) != VK_SUCCESS) {
		std::cerr << "Vulkan: failed to create vma allocator" << std::endl;
		return false;
	}

	return true;
}

bool VulkanApi::createSwapChain() {
	uint32_t format;
	SwapChainSupportDetails swapChainDetails = querrySwapChainSupport(physicalDevice, surface);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapChainFormat(swapChainDetails.surfaceFormats);
	VkPresentModeKHR presentMode = chooseSwapChainPresentMode(swapChainDetails.presentModes);
	VkExtent2D extents = chooseSwapChainExtent(swapChainDetails.capabilities, window);

	uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;

	if (swapChainDetails.capabilities.maxImageCount > 0 &&
		imageCount < swapChainDetails.capabilities.maxImageCount)
		imageCount = swapChainDetails.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extents,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = swapChainDetails.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = true,
		.oldSwapchain = nullptr,
	};

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		std::cerr << "Vulkan: failed to create swap chain" << std::endl;
		return false;
	}

	vkGetSwapchainImagesKHR(device, swapChain,&imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapchainFormat = surfaceFormat.format;
	swapchainExtent = extents;

	return true;
}

bool VulkanApi::createSwapChainImageViews() {
	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = swapChainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapchainFormat,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
		};

		if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
			std::cerr << "failed to create swap chain image view" << std::endl;
			return false;
		}
	}


	return true;
}

void VulkanApi::cleanupSwapChain() {
	for (size_t i = 0; i < swapChainImages.size(); i++)
		vkDestroyImageView(device, swapChainImageViews[i], nullptr);

	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void VulkanApi::recreateSwapChain() {
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);
	cleanupSwapChain();

	createSwapChain();
	createSwapChainImageViews();

}

bool VulkanApi::createSyncObj() {
	imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHTS);
	renderFinishSemaphore.resize(swapChainImages.size());
	drawFences.resize(MAX_FRAMES_IN_FLIGHTS);

	VkSemaphoreCreateInfo semaphoreInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	VkFenceCreateInfo fenceInfo = {
	.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	for (size_t i = 0; i < swapChainImages.size(); i++)
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
			&renderFinishSemaphore[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create renderSemaphore");

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHTS; i++)
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
			&imageAvailableSemaphore[i]) ||
			vkCreateFence(device, &fenceInfo, nullptr, &drawFences[i]) !=
			VK_SUCCESS) {
			std::cerr << "Vulkan: failed to create sync objs" << std::endl;
			return false;
		}

	return true;
}

void VulkanApi::acquireSwapChainIndex(uint32_t frameIndex) {
	if (vkWaitForFences(device, 1, &drawFences[frameIndex], VK_TRUE,
		UINT64_MAX) != VK_SUCCESS)
		throw std::runtime_error("failed to wait for draw fence");

	vkResetFences(device, 1, &drawFences[frameIndex]);

	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
		imageAvailableSemaphore[frameIndex],
		nullptr, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		recreateSwapChain();
}

void VulkanApi::submit(const std::vector<VkCommandBuffer>& cmdBuffs, uint32_t frameIndex) {

	VkSemaphore waitSemaphore[] = { imageAvailableSemaphore[frameIndex] };
	VkSemaphore signalSemaphore[] = { renderFinishSemaphore[imageIndex] };

	VkPipelineStageFlags  waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphore,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = (uint32_t)cmdBuffs.size(),
		.pCommandBuffers = cmdBuffs.data(),
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphore,
	};

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[frameIndex]) != VK_SUCCESS) {
		std::cerr << "Vulkan: failed to submit the graphics queue" << std::endl;
		assert(false);
	}

	VkSwapchainKHR swapChains[] = { swapChain };

	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphore,
		.swapchainCount = 1,
		.pSwapchains = swapChains,
		.pImageIndices = &imageIndex,
	};

	VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) 
		recreateSwapChain();
	else if (result != VK_SUCCESS) {
		std::cerr << "Vulkan: Failed to present swap chain image" << std::endl;
		assert(false);
	}
}

VulkanApi::~VulkanApi() {
	for (size_t i = 0; i != MAX_FRAMES_IN_FLIGHTS; i++) {
		vkDestroySemaphore(device, imageAvailableSemaphore[i], nullptr);
		vkDestroyFence(device, drawFences[i], nullptr);
	}

	for (uint32_t i = 0; i < swapChainImages.size(); i++)
		vkDestroySemaphore(device, renderFinishSemaphore[i], nullptr);


	cleanupSwapChain();

	vmaDestroyAllocator(allocator);

	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

	vkDestroyInstance(instance, nullptr);
}