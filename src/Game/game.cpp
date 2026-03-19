#include "Game/game.h"
#include <iostream>

Game::Game() {

	ren_ = new Renderer(settings);
}

void Game::run() {

	std::cout << "Grapics Api: " << (settings.api == Directx11 ? "Directx 11" : "Vulkan") << std::endl;

	ren_->blackScreen();
}

Game::~Game() {
	delete ren_;
}