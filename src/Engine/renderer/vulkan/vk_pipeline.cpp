#include "Engine/renderer/vulkan/vk_pipeline.h"
#include <fstream>
#include <iostream>
#include <cassert>

std::vector<char> readShaderFile(const char* filename);


VkPipelineModule::VkPipelineModule(VkDevice& device, const VkPipelineInfo& info) 
	: device(device), info(info)  {
	createPipelineLayout();
}

void VkPipelineModule::initGraphics() {
	createPipeline();
}


void VkPipelineModule::addShaderStage(const char* filename, const char* entryName, VkShaderStageFlagBits stage) {
	if(code.empty())
		code = readShaderFile(filename);

	VkShaderModuleCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = (uint32_t)code.size(),
		.pCode = reinterpret_cast<const uint32_t*>(code.data()),
	};

	VkShaderModule sm;

	if (vkCreateShaderModule(device, &createInfo, nullptr, &sm) != VK_SUCCESS) {
		std::cerr << "Failed to create shader module" << std::endl;
	}

	shaders.emplace_back(sm);


	stages.emplace_back(VkPipelineShaderStageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = stage,
			.module = sm,
			.pName  = entryName,
		});

}

void VkPipelineModule::createPipeline() {
	std::vector<VkSpecializationMapEntry> entries;

	if (!info.specInfo.entries.empty()) {
		uint32_t offset = 0;
		for (auto& entry : info.specInfo.entries) {
			entries.emplace_back(VkSpecializationMapEntry{
					.constantID = entry.constantID,
					.offset = offset,
					.size = entry.size,
				});
			offset += entry.size;
		}

		VkSpecializationInfo specInfo{
			.mapEntryCount = (uint32_t)entries.size(),
			.pMapEntries = entries.data(),
			.dataSize = info.specInfo.dataSize,
			.pData = info.specInfo.data,
		};

		for (auto& stage : stages) {
			stage.pSpecializationInfo = &specInfo;
		}
	}

	std::vector<VkVertexInputAttributeDescription> attributes;
	VkVertexInputBindingDescription binding;


	VkPipelineVertexInputStateCreateInfo vertexInputInfo{
	  .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	  .vertexBindingDescriptionCount = 0,
	  .vertexAttributeDescriptionCount = 0,
	};

	if (!info.input.vertexInput.empty()) {
		attributes.reserve(info.input.vertexInput.size());

		for (auto& input : info.input.vertexInput)
			attributes.push_back(VkVertexInputAttributeDescription{
					.location = input.location,
					.binding = input.binding,
					.format = input.format,
					.offset = input.offset,
				});

		binding = {
			.binding = info.input.binding.binding,
			.stride = info.input.binding.stride,
			.inputRate = info.input.binding.rate,
		};

		vertexInputInfo.vertexAttributeDescriptionCount =
			static_cast<uint32_t>(attributes.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &binding;
	}

	std::vector dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
		VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {
	  .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	  .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
	  .pDynamicStates = dynamicStates.data(),
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = info.topology,
		.primitiveRestartEnable = VK_FALSE,
	};

	VkPipelineTessellationStateCreateInfo tesseInfo{};
	if (info.patchControllPoints) {
		tesseInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
			.patchControlPoints = info.patchControllPoints,
		};
	}

	VkPipelineViewportStateCreateInfo viewportState{
	  .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
	  .viewportCount = 1,
	  .scissorCount = 1,
	};

	VkPipelineRasterizationStateCreateInfo raserization{
	 .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	 .depthClampEnable = VK_FALSE,
	 .rasterizerDiscardEnable = VK_FALSE,
	 .polygonMode = info.polygon,
	 .cullMode = info.cullMode,
	 .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
	 .depthBiasEnable = VK_FALSE,
	 .lineWidth = 1.0f,
	};

	VkPipelineMultisampleStateCreateInfo multiSampling = {
	  .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	  .rasterizationSamples = info.sampleCount,
	  .sampleShadingEnable = VK_FALSE,
	};


	VkPipelineColorBlendAttachmentState colorBlendAttachment{
	 .blendEnable = VK_FALSE,
	 .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
					   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	if (info.enableBlend) {
		colorBlendAttachment.blendEnable = VK_TRUE;

		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor =
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

	
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor =
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	VkPipelineColorBlendStateCreateInfo colorBlending{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
	.logicOpEnable = VK_FALSE,
	.logicOp = VK_LOGIC_OP_COPY,
	.attachmentCount = 1,
	.pAttachments = &colorBlendAttachment,
	};

	VkPipelineRenderingCreateInfo pipelineRenderCreateInfo = {
	 .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
	 .colorAttachmentCount = 1,
	 .pColorAttachmentFormats = &info.colorFormat,
	};
	if (info.depthFormat != VK_FORMAT_UNDEFINED)
		pipelineRenderCreateInfo.depthAttachmentFormat = info.depthFormat;

	VkPipelineDepthStencilStateCreateInfo depthStencil{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
	};

	VkGraphicsPipelineCreateInfo pipelineInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &pipelineRenderCreateInfo,
		.stageCount = static_cast<uint32_t>(stages.size()),
		.pStages = stages.data(),
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pTessellationState = (info.patchControllPoints) ? &tesseInfo : nullptr,
		.pViewportState = &viewportState,
		.pRasterizationState = &raserization,
		.pMultisampleState = &multiSampling,
		.pDepthStencilState = &depthStencil,
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamicState,
		.layout = layout,
		.renderPass = nullptr,
	};

	if(vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS){
		std::cerr << "failed to create pipeline" << std::endl;
		assert(false);
	}


	for (auto& sm : shaders)
		vkDestroyShaderModule(device, sm, nullptr);

	stages.clear();
}

void VkPipelineModule::createPipelineLayout() {
	VkPushConstantRange range = {
		.stageFlags = VK_SHADER_STAGE_ALL,
		.offset = 0,
		.size = info.pushConstantSize,
	};

	VkPipelineLayoutCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &range,
	};

	createInfo.setLayoutCount = 1;
	createInfo.pSetLayouts = &info.setLayout;

	if (vkCreatePipelineLayout(device, &createInfo, nullptr, &layout) != VK_SUCCESS) {
		std::cerr << "failed to create layout" << std::endl;
		assert(false);
	}

}

VkPipelineModule::~VkPipelineModule() {
	vkDestroyPipeline(device, pipeline, nullptr);
}


std::vector<char> readShaderFile(const char* filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		std::cerr << "failed to open file: " << filename << std::endl;
		assert(false);
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}
