#pragma once
#include<cstdint>
#include "Game/game_settings.h"
#include "Engine/renderer/vulkan/vulkanApi.h"

struct Renderer {
	Renderer(GameSettings &settings);
	~Renderer();

	void blackScreen();

	VulkanApi* vkApi = nullptr;
	GameSettings& settings;
	GLFWwindow* window = nullptr;


private:

	void initWindow();
};