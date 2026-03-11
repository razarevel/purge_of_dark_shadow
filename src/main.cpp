#include "Game/game.h"

int main() {
	Game *game = new Game();
	game->run();
	delete game;
}