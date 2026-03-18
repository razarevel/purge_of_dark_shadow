#pragma once

#include "Engine/settings.h"
#include "Engine/renderer/renderer.h"

struct Game {
	Game();
	~Game();
	void run();

private:
	Renderer* ren_;
	Settings settings;
};