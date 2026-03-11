#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <cstdint>

struct DepthAttachment {
	VkImage image = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
};

struct ColorAttachment {
	VkImage image = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkImage resolveImage = VK_NULL_HANDLE;
	VkImageView resolveView = VK_NULL_HANDLE;
	VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_NONE;
};

struct BeginInfo {
	uint32_t frameIndex;
	float clearColor[4] = { 0.f, 0.f, 0.f, 0.f };
	VkExtent2D extent;
	DepthAttachment depth;
	std::vector<ColorAttachment> colors;

};

struct CmdBuff {
	CmdBuff(VkDevice device, VkCommandPool cmdPool, uint32_t MAX_FRAMES);
	~CmdBuff();

	

	void cmdBeginBuffer(uint32_t frameIndex);
	void cmdEndBuffer(uint32_t frameIndex);

	void cmdBeginRendering(BeginInfo info);
	void cmdEndRendering(VkImage image, uint32_t frameIndex);

	std::vector<VkCommandBuffer>& getCommandBuffers() { return commandBuffers; }

private:
	VkDevice device;
	std::vector<VkCommandBuffer> commandBuffers;

	void transition_image_layout(VkImageAspectFlags imageAspect,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		VkAccessFlagBits2 srcAccessMask,
		VkAccessFlagBits2 dstAccessMask,
		VkPipelineStageFlags2 srcStageMask,
		VkPipelineStageFlags2 dstStageMask,
		VkImage& image, uint32_t frameIndex);
};