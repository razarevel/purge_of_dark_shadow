#include "Engine/renderer/vulkan/vulkanApi.h"

#include "Engine/vk_mem_alloc.h"
#include <iostream>
#include <set>

VulkanApi::VulkanApi(GLFWwindow* win, const char* appName) :window(win), name(appName) {
	createInstance();
	setupDebugger();
	createSurfaceKHR();
	pickPhysicalDevice();
	createLogicalDevice();
	createVmaAllocation();
	createSwapChain();
	createSwapChainImageViews();
	createSyncObj();

	commandPool = createCommandPool();
	cmdBuff = new CmdBuff(device, commandPool, MAX_FRAMES_IN_FLIGHT);

}


void VulkanApi::createInstance() {

	VkApplicationInfo appInfo{
	 .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
	 .pApplicationName = name,
	.applicationVersion = VK_API_VERSION_1_0,
	.pEngineName = name,
	.engineVersion = VK_API_VERSION_1_0,
	.apiVersion = VK_API_VERSION_1_3,
	};

	if (enableDebugger && !checkValidation())
		throw std::runtime_error("validdation layer requried bot not avialble");

	auto extensions = getRequiredExtensions();

	VkInstanceCreateInfo createInfo{
	.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
	.pApplicationInfo = &appInfo,
	.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
	.ppEnabledExtensionNames = extensions.data(),
	};

	VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
	if (enableDebugger) {
		createInfo.enabledLayerCount =
			static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		populateDebugMessenger(debugInfo);
		createInfo.pNext = &debugInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		throw std::runtime_error("failed to create instance");
}

void VulkanApi::setupDebugger() {
	if (!enableDebugger)
		return;
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	populateDebugMessenger(createInfo);
	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
		&debugMessenger) != VK_SUCCESS)
		throw std::runtime_error("failed to create debug utils messenger");
}

void VulkanApi::createSurfaceKHR() {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
		VK_SUCCESS)
		throw std::runtime_error("failed to create window surface");
}

void VulkanApi::pickPhysicalDevice() {
	uint32_t deviceCount;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
		throw std::runtime_error("failed to find any device with vulkan support");

	std::vector<VkPhysicalDevice> devices;
	devices.resize(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (uint32_t i = 0; i < deviceCount; i++)
		if (isDeviceSuitable(devices[i])) {
			physicalDevice = devices[i];
			break;
		}

	if (physicalDevice == nullptr)
		throw std::runtime_error("failed to find suitable GPU");
}

void VulkanApi::createLogicalDevice() {
	indices = findQueueFamilies(physicalDevice, surface);

	std::set<uint32_t> uniqueQueueFamilies = { indices.graphcisFamily.value(),
											  indices.presentFamily.value() };
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
		.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueInfos.size()),
		.pQueueCreateInfos = deviceQueueInfos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data(),
	};

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
		VK_SUCCESS)
		throw std::runtime_error("failed to create logical device");

	vkGetDeviceQueue(device, indices.graphcisFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void VulkanApi::createVmaAllocation() {
	VmaAllocatorCreateInfo createInfo{
		.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT |
				 VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = physicalDevice,
		.device = device,
		.instance = instance,
		.vulkanApiVersion = VK_API_VERSION_1_3,
	};
	if (vmaCreateAllocator(&createInfo, &allocator) != VK_SUCCESS)
		throw std::runtime_error("failed to create vma allocator");
}

void VulkanApi::createSwapChain() {
	uint32_t formatCount;
	SwapChainSupportDetails swapChainDetails =
		querrySwapChainSupport(physicalDevice, surface);
	VkSurfaceFormatKHR surfaceFormat =
		chooseSwapChainFormat(swapChainDetails.surfaceFormats);
	VkPresentModeKHR presentMode =
		chooseSwapChainPresentMode(swapChainDetails.presentModes);
	VkExtent2D extents =
		chooseSwapChainExtent(swapChainDetails.capabilities, window);

	uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;

	if (swapChainDetails.capabilities.maxImageCount > 0 &&
		imageCount < swapChainDetails.capabilities.maxImageCount)
		imageCount = swapChainDetails.capabilities.maxImageCount;

	minImageCount = imageCount;

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

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
		VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain");

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
		swapChainImages.data());
	swapChainFormat = surfaceFormat.format;
	swapChainColorSpace = surfaceFormat.colorSpace;
	swapChainExtent = extents;
}

void VulkanApi::createSwapChainImageViews() {
	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = swapChainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapChainFormat,
			.subresourceRange =
				{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
		};

		if (vkCreateImageView(device, &createInfo, nullptr,
			&swapChainImageViews[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create swap chain image views");
	}
}

void VulkanApi::createSyncObj() {
	imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishSemaphore.resize(swapChainImages.size());
	drawFences.resize(MAX_FRAMES_IN_FLIGHT);

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

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
			&imageAvailableSemaphore[i]) ||
			vkCreateFence(device, &fenceInfo, nullptr, &drawFences[i]) !=
			VK_SUCCESS)
			throw std::runtime_error("failed to create fence");
}

VkCommandPool VulkanApi::createCommandPool() {
	VkCommandPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = indices.graphcisFamily.value(),
	};

	VkCommandPool pool;

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &pool) !=
		VK_SUCCESS)
		throw std::runtime_error("failed to get command pool");

	return pool;
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

void VulkanApi::acquireSwapChainIndex() {
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


void VulkanApi::submit() {
	VkSemaphore waitSemaphore[] = { imageAvailableSemaphore[frameIndex] };
	VkSemaphore signalSemaphore[] = { renderFinishSemaphore[imageIndex] };

	VkPipelineStageFlags waitStage[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	};

	VkSubmitInfo submitInfo = {
	  .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	  .waitSemaphoreCount = 1,
	  .pWaitSemaphores = waitSemaphore,
	  .pWaitDstStageMask = waitStage,
	  .commandBufferCount = 1,
	  .pCommandBuffers = &cmdBuff->getCommandBuffers()[frameIndex],
	  .signalSemaphoreCount = 1,
	  .pSignalSemaphores = signalSemaphore,
	};

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[frameIndex]) != VK_SUCCESS)
		throw std::runtime_error("faile to submit to the queue");


	VkSwapchainKHR swapChains[] = { swapChain };

	VkPresentInfoKHR presentInfo{
	.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
	.waitSemaphoreCount = 1,
	.pWaitSemaphores = signalSemaphore,
	.swapchainCount = 1,
	.pSwapchains = swapChains,
	.pImageIndices = &imageIndex,
	};

	VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
		frameBufferResize) {
		frameBufferResize = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("failed to present swap chain image");

	frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}


VulkanApi::~VulkanApi() {
	delete cmdBuff;
	vkDestroyCommandPool(device, commandPool, nullptr);

	for (size_t i = 0; i != MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, imageAvailableSemaphore[i], nullptr);
		vkDestroyFence(device, drawFences[i], nullptr);
	}

	for (size_t i = 0; i != swapChainImages.size(); i++)
		vkDestroySemaphore(device, renderFinishSemaphore[i], nullptr);

	cleanupSwapChain();
	vmaDestroyAllocator(allocator);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	if(enableDebugger)
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

	vkDestroyInstance(instance, nullptr);
}
