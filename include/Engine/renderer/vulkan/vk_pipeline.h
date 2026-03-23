#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

struct Attributes {
	uint32_t binding = 0;
	uint32_t location = 0;
	VkFormat format;
	uint32_t offset = 0;
};
struct VertexBindingInfo {
	uint32_t binding = 0;
	uint32_t stride;
	VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX;
};

struct VertexInputInfo {
	std::vector<Attributes> vertexInput;
	VertexBindingInfo binding;
};

struct SpecInfo {
	std::vector<VkSpecializationMapEntry> entries;
	uint32_t dataSize;
	const void* data;
};

struct VkPipelineInfo {
	VertexInputInfo input;
	SpecInfo specInfo;
	VkPrimitiveTopology topology= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	uint32_t patchControllPoints;
	VkPolygonMode polygon = VK_POLYGON_MODE_FILL;
	VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
	VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
	bool enableBlend = false;
	VkFormat depthFormat = VK_FORMAT_UNDEFINED;
	VkFormat colorFormat = VK_FORMAT_UNDEFINED;
	uint32_t pushConstantSize = 256;
	VkDescriptorSetLayout &setLayout;
};

struct VkPipelineModule {
	VkPipelineModule(VkDevice &device, const VkPipelineInfo& info);
	~VkPipelineModule();

	void addShaderStage(const char *filename, const char* entryName, VkShaderStageFlagBits stage, bool reload = false);

	VkPipeline& getPipeline() { return pipeline; }
	VkPipelineLayout& getLayout() { return layout; }

	void initGraphics();

private:
	VkPipelineInfo info;
	VkDevice device;

	VkPipeline pipeline;
	VkPipelineLayout layout;

	std::vector<VkPipelineShaderStageCreateInfo> stages;
	std::vector<char> code;

	std::vector<VkShaderModule> shaders;

	void createPipeline();
	void createPipelineLayout();
};