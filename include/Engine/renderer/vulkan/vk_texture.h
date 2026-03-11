#pragma once
#include "Engine/renderer/vulkan/vulkan_utils.h"
#include "Engine/renderer/vulkan/vk_descriptor.h"

enum VkTextureType {
	TextureType_2D,
	TextureType_Cube,
};

struct VkTextureAttachmentInfo {
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VmaAllocator allocator;
	VkCommandPool cmdPool;
	VkQueue graphicsQueue;
	VkDescriptor* descriptor = nullptr;
	VkBuffer ringBuff = VK_NULL_HANDLE;
	uint32_t offset = 0;
};

struct VkTextureInfo {
	VkTextureType type = TextureType_2D;
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkImageUsageFlags usage = 0;
	VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
	VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	VkExtent3D dimension;
	void* data;
	uint32_t mipLevels = 1;
	uint32_t layerCount = 1;
	uint32_t arrayLayer = 1;
};

struct VkTextureSamplerInfo {
	VkFilter minFilter = VK_FILTER_LINEAR;
	VkFilter magFilter = VK_FILTER_LINEAR;
	VkSamplerMipmapMode mipMapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	VkSamplerAddressMode wrapU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSamplerAddressMode wrapV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSamplerAddressMode wrapW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkCompareOp depthCompareOp = VK_COMPARE_OP_ALWAYS;
	bool depthCompareEnabled = false;
};


struct VkTexture {
	VkTexture(const VkTextureAttachmentInfo &attachmentInfo, const VkTextureInfo &info, const VkTextureSamplerInfo& sampler);
	~VkTexture();

	uint32_t& getImageIndex() { return imageIndex; }
	uint32_t& getSamplerIndex() { return samplerIndex; }

	VkCommandBuffer& getSingleCmdBuffer() { return singleCmdBuff; }

private:
	VkTextureInfo info;
	VkTextureSamplerInfo samplerInfo;
	VkTextureAttachmentInfo attachmentInfo;
	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;
	VkFormat format = VK_FORMAT_UNDEFINED;
	VmaAllocation alloc;

	uint32_t imageIndex;
	uint32_t samplerIndex;

	VkCommandBuffer singleCmdBuff;

	void createImage();
	void createImageView();
	void createSampler();

	void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount);
	void copyBufferToImage();
};