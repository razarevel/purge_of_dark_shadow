#include "Engine/renderer/vulkan/vk_cmd.h"
#include <cassert>
#include <iostream>

VkCmdModule::VkCmdModule(VkDevice& device, VkQueue& queue, uint32_t queueIndex)
    :device(device), queueIndex(queueIndex), queue(queue){
	createCommandPool();
}

void VkCmdModule::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queueIndex,
    };

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
        VK_SUCCESS) {
        std::cerr << "Vulkan: failed to create command pool" << std::endl;
        assert(false);
    }
}

void VkCmdModule::createCommandBuffers(std::vector<VkCommandBuffer>& buffs, uint32_t max_size) {
    buffs.resize(max_size);

    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .commandBufferCount = max_size,
    };

    if (vkAllocateCommandBuffers(device, &allocInfo, buffs.data()) !=
        VK_SUCCESS) {
        std::cerr << "Vulkan: failed to create command buffers" << std::endl;
        assert(false);
    }
}


void VkCmdModule::cmdBeginRendering(VkCommandBuffer& cmdBuff, BeginRenderInfo info) {

    for (auto c : info.color) {

        if (c.image != VK_NULL_HANDLE) 
            transition_image_layout(
                VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, cmdBuff, c.image);

        if (c.resolveImage != VK_NULL_HANDLE)
            transition_image_layout(
                VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, cmdBuff, c.resolveImage);
    }

    VkClearValue clearColor = { {info.clearColor[0], info.clearColor[1], info.clearColor[2], info.clearColor[3]} };
    VkClearValue clearDepth = { {1.0f, 0.0f} };

    VkViewport viewport = {
      .x = 0.0f,
      .y = static_cast<float>(info.swapExtent.height),
      .width = static_cast<float>(info.swapExtent.width),
      .height = -static_cast<float>(info.swapExtent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = info.swapExtent,
    };

    VkRenderingAttachmentInfo  depthAttachmentInfo;
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
            cmdBuff, info.depth.image);

        depthAttachmentInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = info.depth.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = clearDepth,
        };
    }

    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    for (auto& c : info.color) {
        VkRenderingAttachmentInfo attachmentInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = c.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = c.loadOp,
            .storeOp = c.storeOp,
            .clearValue = clearColor,
        };

        if (c.resolveImage) {
            attachmentInfo.resolveImageView = c.resolveView;
            attachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
        }
        colorAttachments.emplace_back(attachmentInfo);
    }

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {
              .offset = {0, 0},
              .extent = info.swapExtent,
        },
        .layerCount = 1,
        .colorAttachmentCount = (uint32_t)colorAttachments.size(),
        .pColorAttachments = colorAttachments.data(),
    };

    if (info.depth.image)
        renderingInfo.pDepthAttachment = &depthAttachmentInfo;

    vkCmdBeginRendering(cmdBuff, &renderingInfo);
    vkCmdSetViewport(cmdBuff, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuff, 0, 1, &scissor);

    cmdBindDepthState(cmdBuff);
}

void VkCmdModule::cmdEndRendering(VkCommandBuffer& cmdBuff, VkImage& image) {

    vkCmdEndRendering(cmdBuff);

    transition_image_layout(
        VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        {}, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, cmdBuff, image);
}


void VkCmdModule::endCommandBuffer(VkCommandBuffer& cmdBuff) {
    if (vkEndCommandBuffer(cmdBuff) != VK_SUCCESS) {
        std::cerr << "Vulkan: Failed to end command buffer" << std::endl;
        assert(false);
    }
}

void VkCmdModule::cmdBindDepthState(VkCommandBuffer& cmdBuff, VkBool32 depthWriteEnable, VkBool32 depthTestEnable) {
    vkCmdSetDepthWriteEnable(cmdBuff, depthWriteEnable);
    vkCmdSetDepthTestEnable(cmdBuff, depthTestEnable);
}

VkCommandBuffer VkCmdModule::beginSingleCommandBuffer() {
    VkCommandBufferAllocateInfo allocaInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocaInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void VkCmdModule::endSingleCommandBuffer(VkCommandBuffer& cmdBuff) {
    vkEndCommandBuffer(cmdBuff);
    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount =1,
        .pCommandBuffers = &cmdBuff,
    };

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, commandPool, 1, &cmdBuff);
}

void VkCmdModule::beginCommandBuffer(VkCommandBuffer& cmdBuff) {
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };

    if (vkBeginCommandBuffer(cmdBuff, &beginInfo) != VK_SUCCESS) {
        std::cerr << "Vulkan:  Failed to begin command buffer" << std::endl;
        assert(false);
    }
}

void VkCmdModule::transition_image_layout(VkImageAspectFlags imageAspect,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkAccessFlagBits2 srcAccessMask,
    VkAccessFlagBits2 dstAccessMask,
    VkPipelineStageFlags2 srcStageMask,
    VkPipelineStageFlags2 dstStageMask,
    VkCommandBuffer& cmdBuff,
    VkImage& image) {

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

            vkCmdPipelineBarrier2(cmdBuff, &dependencyInfo);
}

void VkCmdModule::cmdDraw(VkCommandBuffer& cmdBuff, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    vkCmdDraw(cmdBuff, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VkCmdModule::cmdDrawIndex(VkCommandBuffer& cmdBuff, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    vkCmdDrawIndexed(cmdBuff, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

VkCmdModule::~VkCmdModule() {
    vkDestroyCommandPool(device, commandPool, nullptr);
}