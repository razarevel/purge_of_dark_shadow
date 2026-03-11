#include "Engine/renderer/vulkan/vk_texture.h"
#include "Engine/vk_mem_alloc.h"
#include <array>
#include <stdexcept>


VkTexture::VkTexture(const VkTextureAttachmentInfo& attachmentInfo, const VkTextureInfo& info, const VkTextureSamplerInfo& samInfo)
	: info(info), attachmentInfo(attachmentInfo), samplerInfo(samInfo) {
	createImage();
	createImageView();
	createSampler();

	if (attachmentInfo.descriptor) {

		if(info.viewType == VK_IMAGE_VIEW_TYPE_2D)
			imageIndex = attachmentInfo.descriptor->updateDescImageWrite(imageView, 0);
		else if(info.viewType == VK_IMAGE_VIEW_TYPE_CUBE)
			imageIndex = attachmentInfo.descriptor->updateDescImageWrite(imageView, 2);

		samplerIndex = attachmentInfo.descriptor->updateDescSamplerWrite(sampler, 1);
	}
}

void VkTexture::createImage() {
	VkDeviceSize imageSize = info.dimension.width * info.dimension.height * 4;

	if (format == VK_FORMAT_R16G16B16A16_SFLOAT)
		imageSize = info.dimension.width * info.dimension.height * 8;

	if (info.type == TextureType_Cube) {
		imageSize *= sizeof(float) * 6;
		info.arrayLayer = 6;
	}

	VkImageCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = info.format,
		.extent = info.dimension,
		.mipLevels = info.mipLevels,
		.arrayLayers = info.arrayLayer,
		.samples = info.sampleCount,
		.tiling = info.tiling,
		.usage = info.usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VmaAllocationCreateInfo allocaCreateInfo{
		.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO,
		.priority = 1.0f,
	};

	if (vmaCreateImage(attachmentInfo.allocator, &createInfo, &allocaCreateInfo, &image, &alloc, nullptr) != VK_SUCCESS)
		throw std::runtime_error("failed to create image");

	if (attachmentInfo.ringBuff) {
		

		singleCmdBuff = beginSingleCommandBuffer(attachmentInfo.cmdPool, attachmentInfo.device);

		transitionImageLayout(info.format, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			info.layerCount);
		// copy buffer to image
		copyBufferToImage();
		transitionImageLayout(info.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, info.layerCount);
	}
}

void VkTexture::transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount) {
	VkImageMemoryBarrier barrier = {
	 .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
	  .oldLayout = oldLayout,
	  .newLayout = newLayout,
	  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	  .image = image,
	  .subresourceRange =
		  {
			  .aspectMask = info.aspect,
			  .baseMipLevel = 0,
			  .levelCount = 1,
			  .baseArrayLayer = 0,
			  .layerCount = layerCount,
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
	else
		throw std::runtime_error("unsupported layout transition!");

	vkCmdPipelineBarrier(singleCmdBuff, sourcesStage, destinationStage, 0, 0,
		nullptr, 0, nullptr, 1, &barrier);

}

void VkTexture::copyBufferToImage() {

	if (info.type == TextureType_2D) {
		VkBufferImageCopy region = {
			.bufferOffset= attachmentInfo.offset,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
					.aspectMask = info.aspect,
					.mipLevel = 0,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
		    .imageOffset = {0, 0, 0},
		    .imageExtent = {info.dimension.width, info.dimension.height, 1},

		};

		vkCmdCopyBufferToImage(singleCmdBuff, attachmentInfo.ringBuff, image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}
	else if (info.type == TextureType_Cube) {
		std::array<VkBufferImageCopy, 6> regions{};
		VkDeviceSize faceSize = info.dimension.width * info.dimension.height * 4 * sizeof(float);
		for (uint32_t face = 0; face < 6; face++) {
			regions[face].bufferOffset = attachmentInfo.offset + face * faceSize;
			regions[face].bufferRowLength = 0;
			regions[face].bufferImageHeight = 0;

			regions[face].imageSubresource = {
				.aspectMask = info.aspect,
				.mipLevel = 0,
				.baseArrayLayer = face,
				.layerCount = 1,
			};
			regions[face].imageOffset = { 0, 0, 0 };
			regions[face].imageExtent = { info.dimension.width, info.dimension.height, 1 };
		}

		vkCmdCopyBufferToImage(
			singleCmdBuff, attachmentInfo.ringBuff, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(regions.size()), regions.data());
	}
	else 
		throw std::runtime_error("invalid image type");

}



void VkTexture::createImageView() {
	VkImageViewCreateInfo viewCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = info.viewType,
		.format = info.format,
		.subresourceRange = {
				.aspectMask = info.aspect,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = info.layerCount,
			},
	};

	if (vkCreateImageView(attachmentInfo.device, &viewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
		throw std::runtime_error("failed to create image view");
}

void VkTexture::createSampler() {
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(attachmentInfo.physicalDevice, &properties);

	VkSamplerCreateInfo samplerCreateInfo{
	.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	  .magFilter = samplerInfo.magFilter,
	  .minFilter = samplerInfo.minFilter,
	  .mipmapMode = samplerInfo.mipMapMode,
	  .addressModeU = samplerInfo.wrapU,
	  .addressModeV = samplerInfo.wrapV,
	  .addressModeW = samplerInfo.wrapW,
	  .mipLodBias = 0.0f,
	  .anisotropyEnable = VK_TRUE,
	  .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
	  .compareOp = samplerInfo.depthCompareOp,
	};

	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.compareEnable = samplerInfo.depthCompareEnabled;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		
	if (vkCreateSampler(attachmentInfo.device, &samplerCreateInfo, nullptr, &sampler) !=
		VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler");
}

VkTexture::~VkTexture() {
	if (sampler != VK_NULL_HANDLE)
		vkDestroySampler(attachmentInfo.device, sampler, nullptr);
	if (imageView != VK_NULL_HANDLE)
		vkDestroyImageView(attachmentInfo.device, imageView, nullptr);
	if (image != VK_NULL_HANDLE)
		vmaDestroyImage(attachmentInfo.allocator, image, nullptr);
}