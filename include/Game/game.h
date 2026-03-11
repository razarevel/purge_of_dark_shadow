#pragma once
#include "Engine/renderer/renderer.h"
#include "Game/game_settings.h"
#include "Game/sandbox.h"

struct Game {
	Game();
	void run();
	~Game();
private:
	Renderer* ren;
	GameSettings settings;
	Sandbox* sandbox;
};