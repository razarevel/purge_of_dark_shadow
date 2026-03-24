#pragma once

#include "renderer/directx/dx11_api.h"
#include "Engine/renderer/directx/dx_image.h"
#include "renderer/vulkan/vulkan_api.h"
#include "renderer/vulkan/vk_pipeline.h"
#include "renderer/vulkan/vk_descriptor.h"
#include "renderer/vulkan/vk_cmd.h"
#include "renderer/vulkan/vk_image.h"

struct Mesh {
	Mesh(VulkanApi* api, VkDescriptor* desc, VkCmdModule* cmd);
	Mesh(Dx11Api* api);
	~Mesh();

private:
};