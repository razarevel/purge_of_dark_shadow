#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

struct ColorAttachment {
	VkImage image  = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkImage resolveImage = VK_NULL_HANDLE;
	VkImageView resolveView = VK_NULL_HANDLE;
	VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_NONE;
};


struct DepthAttachment {
	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
};

struct BeginRenderInfo {
	float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.f };
	VkExtent2D swapExtent;
	DepthAttachment depth;
	std::vector<ColorAttachment> color;
};

struct VkCmdModule {
	 VkCmdModule(VkDevice &device, VkQueue &queue, uint32_t queueIndex);
	 ~VkCmdModule();

	 void createCommandBuffers(std::vector<VkCommandBuffer> &buffs, uint32_t max_size);
	 VkCommandPool& getCommandPool() { return commandPool; }

	 void cmdBeginRendering(VkCommandBuffer& cmdBuff, BeginRenderInfo info);
	 void cmdEndRendering(VkCommandBuffer& cmdBuff, VkImage &images);

	 void beginCommandBuffer(VkCommandBuffer& cmdBuff);
	 void endCommandBuffer(VkCommandBuffer& cmdBuff);

	 void cmdBindDepthState(VkCommandBuffer& cmdBuff, VkBool32 depthWriteEnable = VK_FALSE, VkBool32 depthTestEnable = VK_FALSE);

	 VkCommandBuffer beginSingleCommandBuffer();
	 void endSingleCommandBuffer(VkCommandBuffer& cmdBuff);

	 void transition_image_layout(VkImageAspectFlags imageAspect,
		 VkImageLayout oldLayout, VkImageLayout newLayout,
		 VkAccessFlagBits2 srcAccessMask,
		 VkAccessFlagBits2 dstAccessMask,
		 VkPipelineStageFlags2 srcStageMask,
		 VkPipelineStageFlags2 dstStageMask,
		 VkCommandBuffer& cmdBuff,
		 VkImage& image);

	 void cmdDraw(VkCommandBuffer& cmdBuff, uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t firstIntance = 0);


private:
	VkDevice device;
	VkQueue queue;
	uint32_t queueIndex;

	VkCommandPool commandPool;


	void createCommandPool();

};