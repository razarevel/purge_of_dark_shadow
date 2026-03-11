#pragma once
#include "Engine/renderer/vulkan/vulkan_utils.h"


struct BufferInfo {
	VkBufferUsageFlags usage;
	VkMemoryPropertyFlags memory;
	VkDeviceSize size;
	void* data;
};

struct VkBuff {
	VkBuff(VmaAllocator alloc, const BufferInfo& info);
	~VkBuff();

	VkBuffer &getBuffer() { return buffer; }
	VmaAllocation& getAllocation() { return allocation; }
	VmaAllocationInfo& getAllocInfo() { return allocInfo; }

	size_t head = 0;

private:
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocator allocator;
	VmaAllocation allocation;
	VmaAllocationInfo allocInfo;


};