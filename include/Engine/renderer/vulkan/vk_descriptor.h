#pragma once
#include "Engine/renderer/vulkan/vulkan_utils.h"

struct VkDescriptorInfo {
	uint32_t maxSets = 2;
	uint32_t MAX_TEXTURES = 100;
	std::vector<VkDescriptorPoolSize> poolSize;
	std::vector<VkDescriptorBindingFlags> bindingFlags;
	std::vector<VkDescriptorSetLayoutBinding> uboLayouts;
};

struct VkDescriptor {
	VkDescriptor(VkDevice device, const VkDescriptorInfo &info);
	~VkDescriptor();

	int updateDescImageWrite(VkImageView &image, uint32_t dstBinding);
	int updateDescSamplerWrite(VkSampler &sampler, uint32_t dstBinding);

private:
	VkDevice device;
	VkDescriptorPool pool;
	VkDescriptorSetLayout setLayout;
	VkDescriptorInfo info;

	std::vector<VkDescriptorSet> sets;

	std::vector<int> writes;

	void createDescriptorPool();
	void createDescriptorSetLayout();
	void createDescriptorSet();
};