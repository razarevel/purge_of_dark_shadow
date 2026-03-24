#pragma once

#include "renderer/directx/dx11_api.h"
#include "Engine/renderer/directx/dx_image.h"
#include "renderer/vulkan/vulkan_api.h"
#include "renderer/vulkan/vk_pipeline.h"
#include "renderer/vulkan/vk_descriptor.h"
#include "renderer/vulkan/vk_cmd.h"
#include "renderer/vulkan/vk_image.h"

#include "Engine/utils.h"

#include <map>
#include <vector>


struct Textures {
	Textures(VulkanApi* api, VkDescriptor* desc, VkCmdModule* cmd);
	Textures(Dx11Api* api);

	void loadToVk();
	void loadToDX();

	~Textures();

	std::map<std::string, std::map<std::string, VkTexture*>>& getVkTextures() {
		return vkTextures;
	}

	std::map<std::string, std::map<std::string, DxImage*>>& getDxTextures() {
		return dxTextures;
	}


private:
	VulkanApi* vkApi;
	VkDescriptor* descriptor;
	VkCmdModule *cmdPool;

	Dx11Api* dxApi;

	std::map<std::string, std::map<std::string, VkTexture*>> vkTextures;
	std::map<std::string, std::map<std::string, DxImage*>> dxTextures;
};