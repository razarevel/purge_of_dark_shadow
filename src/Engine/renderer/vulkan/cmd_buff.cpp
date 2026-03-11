#include "Engine/renderer/vulkan/cmd_buff.h"
#include <stdexcept>

CmdBuff::CmdBuff(VkDevice device, VkCommandPool cmdPool, uint32_t MAX_FRAMES):device(device) {
	commandBuffers.resize(MAX_FRAMES);

	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = cmdPool,
		.commandBufferCount = MAX_FRAMES,
	};

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) !=
		VK_SUCCESS)
		throw std::runtime_error("failed to allocate command buffer");
}


void CmdBuff::cmdBeginBuffer(uint32_t frameIndex) {
	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		 .flags = 0,
	     .pInheritanceInfo = nullptr,
	};

	if (vkBeginCommandBuffer(commandBuffers[frameIndex], &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("failed to begin command buffer");
}

void CmdBuff::cmdEndBuffer(uint32_t frameIndex) {
	vkEndCommandBuffer(commandBuffers[frameIndex]);
}

void CmdBuff::cmdBeginRendering(BeginInfo info) {

	for (auto c : info.colors) {
		transition_image_layout(
			VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, c.image, info.frameIndex);

		if (c.resolveImage != VK_NULL_HANDLE)
			transition_image_layout(
				VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0,
				VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, c.resolveImage, info.frameIndex);
	}

	VkClearValue clearColor = {
	 {info.clearColor[0], info.clearColor[1], info.clearColor[2],
	  info.clearColor[3]},
	};
	VkClearValue clearDepth = { {1.0f, 0.0f} };

	VkViewport viewport = {
	 .x = 0.0f,
	 .y = 0.0f,
	 .width = static_cast<float>(info.extent.width),
	 .height = static_cast<float>(info.extent.height),
	 .minDepth = 0.0f,
	 .maxDepth = 1.0f,
	};

	VkRect2D scissor = {
		.offset = {0, 0},
		.extent = info.extent,
	};


	VkRenderingAttachmentInfo depthAttachmentInfo;

	if (info.depth.image != VK_NULL_HANDLE) {
		transition_image_layout(
			VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
			VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			info.depth.image, info.frameIndex);

		depthAttachmentInfo = {
	     .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
	      .imageView = info.depth.view,
	       .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
	     .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
	     .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	     .clearValue = clearDepth,
		};
	}

	std::vector<VkRenderingAttachmentInfo> colorAttachments;

	for (auto& c : info.colors) {
		VkRenderingAttachmentInfo attachmentInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = c.view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = c.loadOp,
			.storeOp = c.storeOp,
			.clearValue = clearColor,
		};

		if (c.resolveView != VK_NULL_HANDLE) {
			attachmentInfo.resolveImageView = c.resolveView;
			attachmentInfo.resolveImageLayout =
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
		}

		colorAttachments.emplace_back(attachmentInfo);
	}

	VkRenderingInfo renderingInfo{
	.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
	.renderArea =
		{
			.offset = {0, 0},
			.extent = info.extent,
		},
	.layerCount = 1,
	.colorAttachmentCount = (uint32_t)colorAttachments.size(),
	.pColorAttachments = colorAttachments.data(),
	};

	if(info.depth.image != VK_NULL_HANDLE)
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;

	vkCmdBeginRendering(commandBuffers[info.frameIndex], &renderingInfo);
	vkCmdSetViewport(commandBuffers[info.frameIndex], 0, 1, &viewport);
	vkCmdSetScissor(commandBuffers[info.frameIndex], 0, 1, &scissor);
}

void CmdBuff::cmdEndRendering(VkImage image, uint32_t frameIndex) {
	vkCmdEndRendering(commandBuffers[frameIndex]);
	transition_image_layout(
		VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		{}, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		image, frameIndex);
}

void CmdBuff::transition_image_layout(VkImageAspectFlags imageAspect,
	VkImageLayout oldLayout, VkImageLayout newLayout,
	VkAccessFlagBits2 srcAccessMask,
	VkAccessFlagBits2 dstAccessMask,
	VkPipelineStageFlags2 srcStageMask,
	VkPipelineStageFlags2 dstStageMask,
	VkImage& image, uint32_t frameIndex) {

	VkImageMemoryBarrier2 barrier = {
	 .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	 .srcStageMask = srcStageMask,
	 .srcAccessMask = srcAccessMask,
	 .dstStageMask = dstStageMask,
	 .dstAccessMask = dstAccessMask,
	 .oldLayout = oldLayout,
	 .newLayout = newLayout,
	 .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	 .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	 .image = image,
	 .subresourceRange =
		 {
			 .aspectMask = imageAspect,
			 .baseMipLevel = 0,
			 .levelCount = 1,
			 .baseArrayLayer = 0,
			 .layerCount = 1,
		 },
	};
	VkDependencyInfo dependencyInfo = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier,
	};

	vkCmdPipelineBarrier2(commandBuffers[frameIndex], &dependencyInfo);
}

CmdBuff::~CmdBuff(){
}
