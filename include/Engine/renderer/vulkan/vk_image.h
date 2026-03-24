#pragma once

#include "vk_buff.h"
#include "vk_descriptor.h"

struct VkTextureAttachInfo {
	VkDevice& device;
	VkPhysicalDevice& physicalDevice;
	VmaAllocator& alloc;
	VkCmdModule* cmd;
	VkDescriptor* descriptor;
	VkBuffer ringBuff = VK_NULL_HANDLE;
	uint32_t offset = 0;
};

struct VkTextureInfo {
	VkImageType type = VK_IMAGE_TYPE_2D;
	VkImageViewType viewTyp = VK_IMAGE_VIEW_TYPE_2D;
	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
	VkExtent2D extent = {};
	VkDeviceSize size = 0;
	const void* data = nullptr;
	uint32_t mipLevel = 1;
	uint32_t arrayLayer = 1;
	VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	VkImageUsageFlags usage;
};

struct VkTexturSample {
	VkFilter minFilter = VK_FILTER_LINEAR;
	VkFilter magFilte = VK_FILTER_LINEAR;
	VkSamplerMipmapMode mipMap = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	VkSamplerAddressMode wrapU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSamplerAddressMode wrapV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSamplerAddressMode wrapW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkCompareOp depthCompareOp = VK_COMPARE_OP_ALWAYS;
	bool enableDepth = false;
};

struct VkTexture {
	VkTexture(const VkTextureAttachInfo& attachInfo, const VkTextureInfo &texInfo, const VkTexturSample &samInfo);
	~VkTexture();

	uint32_t& getIndex() { return index; }

	void update(const VkRect2D& region, const void* data, uint32_t bufferRowLenght);

private:
	VkTextureAttachInfo attachInfo;
	VkTextureInfo texInfo;
	VkTexturSample sampInfo;

	VkImage image;
	VkImageView view = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;
	VmaAllocation allocation;

	uint32_t index = 0;

	void createImage();
	void createImageView();
	void createImageSampler();

	void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer& buffer,const VkRect2D& region, uint32_t bufferRowLength = 0, uint32_t offset = 0);
};