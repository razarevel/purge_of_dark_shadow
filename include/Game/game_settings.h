#pragma once
#include<cstdint>
#include <string>

enum Window_Mode {
	Bordered,
	BorderLess,
	Fullscreen, 
};

enum AntiAliasing {
	AN_OFF,
	FXAA ,
	MSAA,
	TAA ,
};

enum GRAPHICS_QUALITY {
	OFF,
	LOW,
	MEDIUM, 
	HIGH,
	ULTRA,
};

enum SWITCH_OPTION {
	ENABLE, 
	DISABLE, 
};

enum BACKEND {
	VULKAN,
	DIRECTX12,
};

struct GameSettings {
	uint32_t width;
	uint32_t height;
	std::string appName;
	Window_Mode win_mode = Bordered;
	AntiAliasing alising = AN_OFF;
	GRAPHICS_QUALITY shadows = OFF;
	GRAPHICS_QUALITY ambient_occlusion = OFF;
	GRAPHICS_QUALITY lighting = LOW;
	GRAPHICS_QUALITY textures = LOW;
	SWITCH_OPTION bloom = DISABLE;
	SWITCH_OPTION motion_blur = DISABLE;
	BACKEND api = VULKAN;

	void loadSettings();
	void saveSettings();
};