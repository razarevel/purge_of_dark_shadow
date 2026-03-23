#pragma once

#include "renderer/directx/dx11_api.h"
#include "renderer/vulkan/vulkan_api.h"
#include "renderer/vulkan/vk_pipeline.h"
#include "renderer/vulkan/vk_descriptor.h"
#include "renderer/vulkan/vk_cmd.h"
#include "renderer/vulkan/vk_buff.h"
#include "renderer/vulkan/vk_image.h"


#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_dx11.h"

struct ImguiRenderer {
	ImguiRenderer(VulkanApi* api, GLFWwindow* window, VkDescriptor *desc, VkCmdModule *cmd);
	ImguiRenderer(Dx11Api* api, GLFWwindow* window);

	~ImguiRenderer();

	void vkBeginFrame(const VkExtent2D& dim);
	void vkEndFrame(VkCommandBuffer& cmdBuf, uint32_t fIndex = 0);

	void dxBeginFrame();
	void dxEndFrame();

private:
	VulkanApi* vkApi = nullptr;
	Dx11Api* dx11Api = nullptr;

	struct ImGuiRendererImpl* pimpl_ = nullptr;
	float displaceScale = 1.0f;
	uint32_t frameIndex = 0;

	// vulkan
	VkPipelineModule* pipeline = nullptr;
	VkDescriptor* descriptor = nullptr;
	VkCmdModule* cmdPool = nullptr;
	struct DrawableData {
		VkBuff* vb_ = nullptr;
		VkBuff* ib_ = nullptr;
		uint32_t numAllocateIndices = 0;
		uint32_t numAllocateVertices = 0;
	};
	DrawableData drawables_[2] = {};
	
	void createPipeline();
};