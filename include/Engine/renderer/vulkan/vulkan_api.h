#pragma once

#include "vk_utils.h"
#include "vk_cmd.h"
#include "vk_mem_alloc.h"

#include "Engine/settings.h"


struct VulkanApi {
	VulkanApi(GLFWwindow* window, Settings& settings, bool enableDeb = false);
	~VulkanApi();

	bool init();

	void submit(const std::vector<VkCommandBuffer> &cmdBuffs, uint32_t frameIndex);

	void waitDeviceIdle() { vkDeviceWaitIdle(device); }

	VkDevice& getDevice() { return device; }
	QueueFamilyIndices& getQueueFamilyIndices() { return indices; }
	VkQueue& getGraphicsQueue() { return graphicsQueue; }

	VkExtent2D& getSwapChainExtent() { return swapchainExtent; }
	VkFormat getSwapChainFormat() { return swapchainFormat; }

	std::vector<VkImage>& getSwapChainImages() { return swapChainImages; }
	std::vector<VkImageView>& getSwapChainImageViews() { return swapChainImageViews; }

	uint32_t &getImageIndex() { return imageIndex; }

	void acquireSwapChainIndex(uint32_t frameIndex);

	VmaAllocator& getAllocator() { return allocator; }

private:
	bool enableDebugger = true;

	GLFWwindow* window = nullptr;
	Settings settings;

	VkInstance instance;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;

	QueueFamilyIndices indices;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VmaAllocator allocator;

	VkFormat swapchainFormat;
	VkExtent2D swapchainExtent;

	uint32_t imageIndex = 0;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkSemaphore> imageAvailableSemaphore;
	std::vector<VkSemaphore> renderFinishSemaphore;
	std::vector<VkFence> drawFences;

	bool createInstance();
	bool setupDebugger();
	bool createSurfaceKHR();
	bool pickPhysicalDevice();
	bool createLogicalDevice();
	bool createVmaAllocation();
	bool createSwapChain();
	bool createSwapChainImageViews();
	bool createSyncObj();

	void cleanupSwapChain();
	void recreateSwapChain();
};