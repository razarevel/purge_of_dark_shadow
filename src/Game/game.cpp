#include "Game/game.h"
#include <iostream>

Game::Game() {
	ren_ = new Renderer(settings);
}

void Game::run() {
	std::cout << "yui ga doku son" << std::endl;
}

Game::~Game() {
	delete ren_;
}