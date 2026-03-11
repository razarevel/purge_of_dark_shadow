#include "Game/game.h"
#include <iostream>

Game::Game(){
	settings.loadSettings();
	ren = new Renderer(settings);
	sandbox = new Sandbox(ren, settings);
}

void Game::run() {

#ifdef _DEBUG
	std::cout << "game is running on debug" << std::endl;
#else
	std::cout << "game is running on release" << std::endl;
#endif

	sandbox->run();

}

Game::~Game() {
	delete sandbox;
	delete ren;
}