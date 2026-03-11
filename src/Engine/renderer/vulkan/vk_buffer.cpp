#include "Engine/renderer/vulkan/vk_buffer.h"
#include <iostream>

VkBuff::VkBuff(VmaAllocator alloc, const BufferInfo& info) :allocator(alloc) {

	BufferCreateInfo buffInfo = {
		.size = info.size,
		.usages = info.usage | info.memory,
		.memoryUsage = VMA_MEMORY_USAGE_AUTO,
	};

	if (info.memory & VK_BUFFER_USAGE_TRANSFER_DST_BIT){
		buffInfo.allocflags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
}
	else if(info.memory & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) {
		buffInfo.allocflags =
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}

	createBuffer(buffInfo, alloc, buffer, allocation, allocInfo);

	if (info.data) {
		updateBuffer(alloc, info.usage, buffer, allocation, info.size, info.data);
	}
}

VkBuff::~VkBuff() {
	if (buffer == VK_NULL_HANDLE)
		vmaDestroyBuffer(allocator, buffer, allocation);
}