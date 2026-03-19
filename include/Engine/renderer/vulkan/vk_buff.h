#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vk_cmd.h"

#include "vk_mem_alloc.h"

enum BufferType {
	Host_Visible,
	Storage_Type,
};

struct VkBuffInfo {
	VkDeviceSize size;
	const void* data;
	VkBufferUsageFlagBits usage;
	BufferType type;
};

struct VkBuff {
	VkBuff(VmaAllocator& alloc,VkCmdModule *cmd, const VkBuffInfo &info);
	~VkBuff();

	VkBuffer& getBuffer() { return buffer; }
	VmaAllocation& getAllocation() { return alloc; }
	VmaAllocationInfo& getAllocInfo() { return allocInfo; }

	static void createStagingBuffer(VmaAllocator& alloc, VkDeviceSize &size, VkBuffer& buffer, VmaAllocation& allocation, VmaAllocationInfo& allocInfo);

private:
	VkCmdModule* cmd;
	VmaAllocator allocator;

	VkBuffer buffer;
	VmaAllocation alloc;
	VmaAllocationInfo allocInfo;
	VkBuffInfo info;

	void createBuffer();
	void updateBuffer();
	void copyBuffer(VkBuffer &staginBuff);
};