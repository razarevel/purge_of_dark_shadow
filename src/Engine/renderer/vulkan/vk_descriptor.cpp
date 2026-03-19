#include "Engine/renderer/vulkan/vk_descriptor.h"
#include <iostream>
#include <cassert>

VkDescriptor::VkDescriptor(VkDevice& device, const VkDescriptorInfo& info) 
	:info(info), device(device) {
	createDescriptorPool();
	createDescriptorSetLayout();
	createDescriptorSets();
}

void VkDescriptor::createDescriptorPool() {
	VkDescriptorPoolCreateInfo poolInfo{
	  .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
	  .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
	  .maxSets = 2,
	  .poolSizeCount = (uint32_t)info.pool.size(),
	  .pPoolSizes = info.pool.data(),
	};

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
		std::cerr << "failed to create descriptor Pool " << std::endl;
		assert(false);
	}
}

void VkDescriptor::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBindingFlagsCreateInfo flagsCreateInfo = {
	.sType =
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
	.bindingCount = static_cast<uint32_t>(info.bindingFlags.size()),
	.pBindingFlags = info.bindingFlags.data(),
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &flagsCreateInfo,
		.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
		.bindingCount = static_cast<uint32_t>(info.uboLayouts.size()),
		.pBindings = info.uboLayouts.data(),
	};

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
		std::cerr << "failed to create descriptor set layotu " << std::endl;
		assert(false);
	}

}

void VkDescriptor::createDescriptorSets() {
	sets.resize(2);
	std::vector<VkDescriptorSetLayout> layouts(2, layout);
	uint32_t counts[] = { info.MAX_TEXTURES, info.MAX_TEXTURES };

	VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo{
	 .sType =
		 VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
	 .descriptorSetCount = static_cast<uint32_t>(2),
	 .pDescriptorCounts = counts,
	};

	VkDescriptorSetAllocateInfo allocInfo{
	 .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
	 .pNext = &countInfo,
	 .descriptorPool = pool,
	 .descriptorSetCount = 2,
	 .pSetLayouts = layouts.data(),
	};

	if (vkAllocateDescriptorSets(device, &allocInfo, sets.data()) !=
		VK_SUCCESS) {
		std::cerr << "failed to create descriptor sets" << std::endl;
		assert(false);
	}
}

VkDescriptor::~VkDescriptor() {
	vkDestroyDescriptorPool(device, pool, nullptr);
}