#include "Engine/renderer/renderer.h"
#include <stdexcept>
#include <iostream>

Renderer::Renderer(GameSettings& settings) :settings(settings) {
	if (settings.api == VULKAN) {
		initWindow();
		vkApi = new VulkanApi(window, settings.appName.c_str());
	}
}

void Renderer::initWindow() {
	if (!glfwInit())
		throw std::runtime_error("failed to init window");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWmonitor* monitor = nullptr;

	if(settings.win_mode == Fullscreen)
		monitor = glfwGetPrimaryMonitor();


	window = glfwCreateWindow(settings.width, settings.height, settings.appName.c_str(), monitor, nullptr);

	glfwSetKeyCallback(window,
		[](auto* window, int keys, int, int action, int mods) {
			if (keys == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
				glfwSetWindowShouldClose(window, GLFW_TRUE);
		});
}

void Renderer::blackScreen() {
		CmdBuff* buff = vkApi->acquireCmdBuff();
		buff->cmdBeginBuffer(vkApi->frameIndex);
		{
			buff->cmdBeginRendering({
				.frameIndex = vkApi->frameIndex,
				.extent = vkApi->swapChainExtent,
				.colors = {{
							.image = vkApi->getSwapChainImage(),
							.view = vkApi->getSwapChainImageView(),

					}},
				});
			buff->cmdEndRendering(vkApi->getSwapChainImage(), vkApi->frameIndex);
		}
		buff->cmdEndBuffer(vkApi->frameIndex);

		vkApi->submit();
}

Renderer::~Renderer() {
	if (settings.api == VULKAN) {
		glfwDestroyWindow(window);
		glfwTerminate();
		delete vkApi;
	}

}