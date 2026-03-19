#pragma once

#include "Engine/settings.h"
#include "directx/dx11_api.h"
#include "vulkan/vulkan_api.h"
#include "vulkan/vk_buff.h"
#include "vulkan/vk_descriptor.h"
#include "vulkan/vk_pipeline.h"

#include <glm/glm.hpp>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
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
	ComPtr<ID3D11Buffer> _triangleBuff = nullptr;
	ComPtr<ID3D11InputLayout> inputLayout = nullptr;
	ComPtr<ID3D11VertexShader> _vertexShader = nullptr;
	ComPtr<ID3D11PixelShader >_pixelShader = nullptr;

	// vk resources
	VkBuff* vertBuf = nullptr;
	VkDescriptor* descriptor = nullptr;
	VkPipelineModule* pipeline;

	void initWindow();

	static void HandleResize(GLFWwindow* window, int width, int height);
	void onResize(int widht, int height);

	void drawTriangle(uint32_t frameIndex = 0);
};