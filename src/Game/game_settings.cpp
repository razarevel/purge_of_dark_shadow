#include "Game/game_settings.h"
#include "Engine/json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

const char* path = RESOURCES_PATH "scenes/game_settings.json";

void GameSettings::loadSettings() {

	std::ifstream f(path);
	if (!f.is_open())
		std::cerr << "failed to open game_settings" << std::endl;
	

	json data = json::parse(f);
	uint32_t mode = 0;
	appName = data["name"].get<std::string>();
	width = data["width"].get<uint32_t>();
	height = data["height"].get<uint32_t>();
	mode = data["window_mode"].get<uint32_t>();


	if (mode == 0)
		win_mode = BorderLess;
	else if (mode == 1)
		win_mode = Bordered;
	else if (mode == 2)
		win_mode = Fullscreen;

	

	mode = data["anti_alising"].get<uint32_t>();

	if (mode == 0)
		alising = AN_OFF;
	else if (mode == 1)
		alising = FXAA;
	else if (mode == 2)
		alising = MSAA;
	else if (mode == 3)
		alising = TAA;

	mode = data["shadows"].get<uint32_t>();

	if (mode == 0)
		shadows = OFF;
	else if (mode == 1)
		shadows = LOW;
	else if (mode == 2)
		shadows = MEDIUM;
	else if (mode == 3)
		shadows = HIGH;
	else if (mode == 4)
		shadows = ULTRA;

	mode = data["ambient_occlusion"].get<uint32_t>();

	if (mode == 0)
		ambient_occlusion = LOW;
	else if (mode == 1)
		ambient_occlusion = MEDIUM;
	else if (mode == 2)
		ambient_occlusion = HIGH;

	mode = data["lighting"].get<uint32_t>();

	if (mode == 0)
		lighting = LOW;
	else if (mode == 1)
		lighting = MEDIUM;
	else if (mode == 2)
		lighting = HIGH;

	mode = data["textures"].get<uint32_t>();

	if (mode == 0)
		textures = LOW;
	else if (mode == 1)
		textures = MEDIUM;
	else if (mode == 2)
		textures = HIGH;

	mode = data["bloom"].get<uint32_t>();

	if (mode == 0)
		bloom = DISABLE;
	else if (mode == 1)
		bloom = ENABLE;

	mode = data["motion_blur"].get<uint32_t>();

	if (mode == 0)
		motion_blur = DISABLE;
	else if (mode == 1)
		motion_blur = ENABLE;

	mode = data["api"].get<uint32_t>();

	if (mode == 0)
		api = VULKAN;
	else if (mode == 1)
		api = DIRECTX12;

	f.close();
}

void GameSettings::saveSettings() {}