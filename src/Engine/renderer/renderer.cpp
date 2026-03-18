#include "Engine/renderer/renderer.h"
#include <cassert>
#include <iostream>

Renderer::Renderer(const Settings &set) : settings(set) {
	initWindow();
	bool vkWorked = false;

	Api api = settings.api;

	if (api == Vulkan) {
		vkApi = new VulkanApi(window, settings);
		if (!vkApi->init()) {
			api = Directx11;
			std::cerr << "failed to initialize vulkan" << std::endl;
		}
		else
			std::cout << "vulkan initialized successfully" << std::endl;
	}

	if (api == Directx11) {
		dx11Api = new Dx11Api(window, settings);
		if (!dx11Api->Initialize()) {
			std::cerr << "failed to intialize dx 11" << std::endl;
			assert(false);
		}
		std::cout << "Directx 11 initialized successfully" << std::endl;
	}
		
}

void Renderer::initWindow() {
	if (!glfwInit())
		throw std::runtime_error("failed to init glfw");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(
		settings.width, settings.height, settings.appName.c_str(),
		settings.win_mode == Fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);

	if (window == nullptr)
		throw std::runtime_error("failed to create window");

	glfwSetFramebufferSizeCallback(window, HandleResize);

}

void Renderer::HandleResize(GLFWwindow* window, int width, int height) {
	Renderer* ren = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	ren->onResize(width, height);
}

void Renderer::onResize(int width, int height) {
	settings.width = (uint32_t)width;
	settings.height = (uint32_t)height;
}

Renderer::~Renderer() {
	if (vkApi != nullptr)
		delete vkApi;
	if (dx11Api != nullptr)
		delete dx11Api;
}
