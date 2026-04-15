#include "game/Game.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Game game;
    while (game.Update()) {
        game.Render();
    }
    return 0;
}
