#pragma once
#include "Engine/renderer/vulkan/vulkan_utils.h"
#include "Engine/renderer/vulkan/cmd_buff.h"
#include "Engine/vk_mem_alloc.h"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

struct VulkanApi {
	VulkanApi(GLFWwindow* win, const char* appName);
	~VulkanApi();

	void submit();

	CmdBuff* acquireCmdBuff() { 
		acquireSwapChainIndex();
		return cmdBuff; 
	}
	VkImage getSwapChainImage() { return swapChainImages[imageIndex]; }
	VkImageView getSwapChainImageView() { return swapChainImageViews[imageIndex]; }
	VkDevice getDevice() { return device; }
	VkPhysicalDevice getPhysicalDevice() { return physicalDevice; }
	VkQueue getGraphicsQueue() { return graphicsQueue; }
	VmaAllocator getAllocator() { return allocator; }
	VkCommandPool getCommandPool() { return commandPool; }
	void waitDeviceIdle() {  vkDeviceWaitIdle(device); }

	uint32_t frameIndex = 0;
	uint32_t imageIndex = 0;
	VkFormat swapChainFormat;
	VkExtent2D swapChainExtent;
	VkColorSpaceKHR swapChainColorSpace;
	bool frameBufferResize = false;

	
	VkCommandPool createCommandPool();


private:
	GLFWwindow* window;
	const char *name;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VmaAllocator allocator;
	VkSwapchainKHR swapChain;
	VkCommandPool commandPool;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkSemaphore> imageAvailableSemaphore;
	std::vector<VkSemaphore> renderFinishSemaphore;
	std::vector<VkFence> drawFences;
	std::vector<VkCommandBuffer> commandBuffers;
	
	
	uint32_t minImageCount;
	QueueFamilyIndices indices;

	CmdBuff *cmdBuff;

	void createInstance();
	void setupDebugger();
	void createSurfaceKHR();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createVmaAllocation();
	void createSwapChain();
	void createSwapChainImageViews();
	void createSyncObj();
	void cleanupSwapChain();

	void acquireSwapChainIndex();
	void recreateSwapChain();
};