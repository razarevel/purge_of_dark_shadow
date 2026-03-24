#include "Engine/renderer/vulkan/vk_image.h"
#include <iostream>
#include <cassert>

VkTexture::VkTexture(const VkTextureAttachInfo& attachInfo, const VkTextureInfo& texInfo, const VkTexturSample& samInfo)
	:attachInfo(attachInfo), texInfo(texInfo), sampInfo(samInfo) {
	createImage();
	createImageView();

	if (texInfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
		createImageSampler();
		index = attachInfo.descriptor->updateImageIndex(view, sampler);
	}
}

void VkTexture::createImage() {
	VkImageCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = texInfo.type,
		.format = texInfo.format,
		.extent = VkExtent3D(texInfo.extent.width,texInfo.extent.height, 1),
		.mipLevels = texInfo.mipLevel,
		.arrayLayers = texInfo.arrayLayer,
		.samples = texInfo.sampleCount,
		.tiling = texInfo.tiling,
		.usage = texInfo.usage,
		.sharingMode = texInfo.sharingMode,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VmaAllocationCreateInfo allocaCreateInfo{
	  .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
	  .usage = VMA_MEMORY_USAGE_AUTO,
	  .priority = 1.0f,
	};

	if (vmaCreateImage(attachInfo.alloc, &info, &allocaCreateInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
		std::cerr << "failed to create Image" << std::endl;
		assert(false);
	}

	if (texInfo.data) {
		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;
		VmaAllocationInfo stagingAllocInfo;
		VkBuff::createStagingBuffer(attachInfo.alloc, texInfo.size, stagingBuffer, stagingAllocation, stagingAllocInfo);
		vmaCopyMemoryToAllocation(attachInfo.alloc, texInfo.data, stagingAllocation, 0, texInfo.size);
		transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VkRect2D region = {
				.offset = {0, 0},
			.extent = {texInfo.extent.width, texInfo.extent.height},
		};
		copyBufferToImage(stagingBuffer, region);
		transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		vmaDestroyBuffer(attachInfo.alloc, stagingBuffer, stagingAllocation);

	} 
	else if (attachInfo.ringBuff != VK_NULL_HANDLE) {
		transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VkRect2D region = {
				.offset = {0, 0},
			.extent = {texInfo.extent.width, texInfo.extent.height},
		};
		copyBufferToImage(attachInfo.ringBuff, region, 0, attachInfo.offset);
		transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

void VkTexture::createImageView() {
	VkImageViewCreateInfo viewInfo{
	.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	.image = image,
	.viewType = texInfo.viewTyp,
	.format = texInfo.format,
	.subresourceRange =
		{
			.aspectMask = texInfo.aspect,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = texInfo.arrayLayer,
		},
	};

	if (vkCreateImageView(attachInfo.device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
		std::cerr << "Vulkan: failed to create image view" << std::endl;
		assert(false);
	}
}

void VkTexture::createImageSampler() {
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(attachInfo.physicalDevice, &properties);

	VkSamplerCreateInfo samplerCreateInfo{
	 .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	 .magFilter = sampInfo.magFilte,
	 .minFilter = sampInfo.minFilter,
	 .mipmapMode = sampInfo.mipMap,
	 .addressModeU = sampInfo.wrapU,
	 .addressModeV = sampInfo.wrapV,
	 .addressModeW = sampInfo.wrapW,
	 .mipLodBias = 0.0f,
	 .anisotropyEnable = VK_TRUE,
	 .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
	 .compareOp = sampInfo.depthCompareOp,
	};
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.compareEnable = sampInfo.enableDepth;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	if (vkCreateSampler(attachInfo.device, &samplerCreateInfo, nullptr, &sampler) != VK_SUCCESS) {
		std::cerr << "failed to create image sampler" << std::endl;
		assert(false);
	}
}

void VkTexture::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = attachInfo.cmd->beginSingleCommandBuffer();

	VkImageMemoryBarrier barrier = {
	 .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
	 .oldLayout = oldLayout,
	 .newLayout = newLayout,
	 .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	 .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	 .image = image,
	 .subresourceRange =
		 {
			 .aspectMask = texInfo.aspect,
			 .baseMipLevel = 0,
			 .levelCount = 1,
			 .baseArrayLayer = 0,
			 .layerCount = texInfo.arrayLayer,
		 },

	};

	VkPipelineStageFlags sourcesStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourcesStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourcesStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		std::cerr << "unspported layout transition" << std::endl;
		assert(false);
	}

	vkCmdPipelineBarrier(commandBuffer, sourcesStage, destinationStage, 0, 0,
		nullptr, 0, nullptr, 1, &barrier);


	attachInfo.cmd->endSingleCommandBuffer(commandBuffer);
}

void VkTexture::copyBufferToImage(VkBuffer& buffer, const VkRect2D& imageRegion, uint32_t bufferRowLength, uint32_t offset) {
	VkCommandBuffer commandBuffer = attachInfo.cmd->beginSingleCommandBuffer();
	VkBufferImageCopy region{
		.bufferOffset = offset,
		.bufferRowLength = bufferRowLength,
		.bufferImageHeight = 0,
		.imageSubresource =
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		.imageOffset = {imageRegion.offset.x, imageRegion.offset.y, 0},
		.imageExtent = {imageRegion.extent.width, imageRegion.extent.height, 1},
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	attachInfo.cmd->endSingleCommandBuffer(commandBuffer);

}

void VkTexture::update(const VkRect2D& region, const void* data, uint32_t bufferRowLenght) {
	VkDeviceSize imageSize= bufferRowLenght * region.extent.height * 4;

	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingAllocInfo;

	VkBuff::createStagingBuffer(attachInfo.alloc, imageSize, stagingBuffer, stagingAllocation, stagingAllocInfo);

	vmaCopyMemoryToAllocation(attachInfo.alloc, data, stagingAllocation, 0, imageSize);

	transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copyBufferToImage(stagingBuffer, region, bufferRowLenght);

	transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vmaDestroyBuffer(attachInfo.alloc, stagingBuffer, stagingAllocation);
}

VkTexture::~VkTexture() {
	if(sampler != VK_NULL_HANDLE)
		vkDestroySampler(attachInfo.device, sampler, nullptr);
	if (view != VK_NULL_HANDLE)
		vkDestroyImageView(attachInfo.device, view, nullptr);

	vkDestroyImage(attachInfo.device, image, nullptr);
}
