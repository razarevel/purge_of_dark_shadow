#pragma once

#include "renderer/directx/dx11_api.h"
#include "renderer/vulkan/vulkan_api.h"


struct ImguiRenderer {
	ImguiRenderer(VulkanApi* api);
	ImguiRenderer(Dx11Api*api);

	~ImguiRenderer();

	void beginFrame();
	void endFrame();

private:
	VulkanApi* vkApi = nullptr;
	Dx11Api* dx11Api = nullptr;

};