#pragma once

#include "Engine/settings.h"
#include "directx/dx11_api.h"
#include "vulkan/vulkan_api.h"
#include "vulkan/vk_buff.h"
#include "vulkan/vk_descriptor.h"
#include "vulkan/vk_pipeline.h"
#include "vulkan/vk_image.h"

#include "directx/dx_image.h"

#include <glm/glm.hpp>

struct Vertex {
	glm::vec3 pos;
	glm::vec2 uv;
};

struct Renderer {
	Renderer(Settings &settings);
	~Renderer();

	GLFWwindow* window = nullptr;
	VulkanApi* vkApi = nullptr;
	Dx11Api* dx11Api = nullptr;

	void blackScreen();

private:
	Settings &settings;
	std::vector<VkCommandBuffer> commandBuffers;
	VkCmdModule* buff;

	// dx resources


	// vk resouces
	VkDescriptor* descriptor;

	void initWindow();

	static void HandleResize(GLFWwindow* window, int width, int height);
	void onResize(int widht, int height);

	void drawTriangle(uint32_t frameIndex = 0);
};