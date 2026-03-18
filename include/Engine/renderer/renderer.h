#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Engine/settings.h"
#include "directx/dx11_api.h"
#include "vulkan/vulkan_api.h"

struct Renderer {
	Renderer(const Settings &settings);
	~Renderer();

private:
	GLFWwindow* window = nullptr;
	Settings settings;
	VulkanApi* vkApi = nullptr;
	Dx11Api* dx11Api = nullptr;

	void initWindow();

	static void HandleResize(GLFWwindow* window, int width, int height);
	void onResize(int widht, int height);
};