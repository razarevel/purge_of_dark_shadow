#pragma once 

#include <cstdint>
#include <string>

enum Window_Mode {
	Fullscreen,
	Window,
	Window_BoarderLess,
};

enum Api {
	Vulkan,
	Directx11,
};

struct Settings {
	uint32_t width = 1200;
	uint32_t height = 800;
	const std::string appName = "Purge of Black Shadow";
	Window_Mode win_mode = Window;
	Api api = Directx11;

	void load();
	void save();
};