#pragma once
#include "Game/game_settings.h"
#include "Engine/renderer/renderer.h"
#include "Engine/renderer/vulkan/vk_descriptor.h"
#include "Engine/textures.h"

struct Sandbox {
	Sandbox(Renderer* ren, GameSettings& setings);
	~Sandbox();

	void run();
private:
	Renderer* ren_;
	GameSettings settings;
	Textures* textures = nullptr;
	VkDescriptor *descriptor;
	VkCommandPool secondCmdPool;

	uint32_t MAX_TEXTURES = 100;
};