#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

struct VkDescriptorInfo {
	 std::vector<VkDescriptorPoolSize> pool;
	 std::vector<VkDescriptorBindingFlags> bindingFlags;
	 std::vector<VkDescriptorSetLayoutBinding> uboLayouts;
	 uint32_t MAX_TEXTURES;
};

struct VkDescriptor {
	VkDescriptor(VkDevice& device, const VkDescriptorInfo &info);
	~VkDescriptor();

	VkDescriptorPool& getPool() { return pool; }
	VkDescriptorSetLayout& getSetLayout() { return layout; }
	std::vector<VkDescriptorSet>& getSets() { return sets; }

private:
	VkDevice device;
	VkDescriptorInfo info;

	VkDescriptorPool pool;
	VkDescriptorSetLayout layout;
	std::vector<VkDescriptorSet> sets;

	void createDescriptorPool();
	void createDescriptorSetLayout();
	void createDescriptorSets();
};