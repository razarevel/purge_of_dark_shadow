#include "Engine/renderer/vulkan/vk_descriptor.h"
#include <array>
#include <stdexcept>


VkDescriptor::VkDescriptor(VkDevice device, const VkDescriptorInfo& info)
	:device(device), info(info) {
	createDescriptorPool();
	createDescriptorSetLayout();
	createDescriptorSet();
}

void VkDescriptor::createDescriptorPool() {
	VkDescriptorPoolCreateInfo  poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
		.maxSets = info.maxSets,
		.poolSizeCount = (uint32_t)info.poolSize.size(),
		.pPoolSizes = info.poolSize.data(),
	};

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) !=
		VK_SUCCESS)
		throw std::runtime_error("failed to create descritptor fool");
}

void VkDescriptor::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBindingFlagsCreateInfo flagsCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount = (uint32_t)info.bindingFlags.size(),
		.pBindingFlags = info.bindingFlags.data(),
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &flagsCreateInfo,
		.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
		.bindingCount = (uint32_t)info.uboLayouts.size(),
		.pBindings = info.uboLayouts.data(),
	};

	writes.resize(info.uboLayouts.size());

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
		&setLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set layouts");
}

void VkDescriptor::createDescriptorSet() {
	sets.resize(2);

	std::vector<VkDescriptorSetLayout> layouts(2, setLayout);
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
		VK_SUCCESS)
		throw std::runtime_error("failed to allocate descriptor set");
}

int VkDescriptor::updateDescImageWrite(VkImageView& view, uint32_t dstBinding) {
	writes[dstBinding]++;

	VkDescriptorImageInfo imageInfo = {
		.imageView = view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	for (size_t i = 0; i < 2; i++) {
		VkWriteDescriptorSet descriptorWrites{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = sets[i],
			 .dstBinding = dstBinding,
			 .dstArrayElement = (uint32_t)writes[dstBinding],
			 .descriptorCount = 1,
			 .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			 .pImageInfo = &imageInfo,
		};
		vkUpdateDescriptorSets(device, 1, &descriptorWrites, 0, nullptr);
	}
	return writes[dstBinding];
}

int VkDescriptor::updateDescSamplerWrite(VkSampler &sampler, uint32_t dstBinding) {
	writes[dstBinding]++;
	VkDescriptorImageInfo samplerInfo{
		.sampler = sampler,
	};
	for (size_t i = 0; i < 2; i++) {
		VkWriteDescriptorSet descriptorWrites{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = sets[i],
			 .dstBinding = dstBinding,
			 .dstArrayElement = (uint32_t)writes[dstBinding],
			 .descriptorCount = 1,
			 .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
			 .pImageInfo = &samplerInfo,
		};

		vkUpdateDescriptorSets(device, 1, &descriptorWrites, 0, nullptr);
	}
	return writes[dstBinding];
}

VkDescriptor::~VkDescriptor() {
	vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
	vkDestroyDescriptorPool(device, pool, nullptr);
}