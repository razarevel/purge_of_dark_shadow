#include "Engine/renderer/vulkan/vk_buff.h"
#include <cassert>
#include <iostream>

VkBuff::VkBuff(VmaAllocator& allocator, VkCmdModule* cmd, const VkBuffInfo& info) 
	: allocator(allocator), info(info), cmd(cmd) {
	createBuffer();
	if (info.data)
		updateBuffer();
}

void VkBuff::createBuffer() {
	VkBufferUsageFlags usage = info.usage;

	if (info.type == Storage_Type)
		usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	else if (info.type == Host_Visible)
		usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	if (info.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
		usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	if (info.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
		usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;


	VkBufferCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = info.size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VmaAllocationCreateInfo allocCreateInfo = {
		.flags = 0,
		.usage = VMA_MEMORY_USAGE_AUTO,
		.priority = 1.0f,
	};

	if (info.type == Storage_Type)
		allocCreateInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	else if (info.type == Host_Visible)
		allocCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT;


	if (vmaCreateBuffer(allocator, &createInfo, &allocCreateInfo, &buffer,
		&alloc, &allocInfo) != VK_SUCCESS)
		throw std::runtime_error("failed to create buffer usage");
}

void VkBuff::updateBuffer() {
	if (info.type == Storage_Type) {
		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;
		VmaAllocationInfo stagingAllocInfo;

		createStagingBuffer(allocator, info.size, stagingBuffer, stagingAllocation, stagingAllocInfo);

		vmaCopyMemoryToAllocation(allocator, info.data, stagingAllocation, 0,  info.size);

		copyBuffer(stagingBuffer);

		vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
	}
	else if (info.type == Host_Visible) 
		if (vmaCopyMemoryToAllocation(allocator, info.data, alloc, 0, info.size) != VK_SUCCESS) {
			std::cerr << "Vulkan: failed to update buffer" << std::endl;
			assert(false);
		}
	else
		assert(false);
}

void VkBuff::createStagingBuffer(VmaAllocator& alloc, VkDeviceSize &size, VkBuffer& buffer, VmaAllocation& allocation, VmaAllocationInfo& allocInfo) {
	VkBufferCreateInfo info{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VmaAllocationCreateInfo allocCreateInfo = {
		.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
				 VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
				 VMA_ALLOCATION_CREATE_MAPPED_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO,
		.priority = 1.0f,
	};

	if (vmaCreateBuffer(alloc, &info, &allocCreateInfo, &buffer,
		&allocation, &allocInfo) != VK_SUCCESS) {
		std::cerr << "failed to create staging buffer" << std::endl;
		assert(false);
	}
}

void VkBuff::copyBuffer(VkBuffer &staginBuff) {
	VkCommandBuffer cmdBuff = cmd->beginSingleCommandBuffer();

	VkBufferCopy copyRegin = {
		.size = info.size,
	};

	vkCmdCopyBuffer(cmdBuff, staginBuff, buffer, 1, &copyRegin);

	cmd->endSingleCommandBuffer(cmdBuff);
}

VkBuff::~VkBuff() {
	vmaDestroyBuffer(allocator, buffer, alloc);
}