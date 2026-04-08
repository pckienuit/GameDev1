#include "core/Window.h"
#include "core/GameLoop.h"
#include "core/Input.h"
#include "renderer/Renderer.h"
#include "renderer/SpriteBatch.h"
#include "renderer/Texture.h"
#include "tilemap/Tilemap.h"
#include "game/Player.h"
#include <iostream>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Window      window("Mario Engine", 800, 600);
    Renderer    renderer(window);
    SpriteBatch sprite_batch(renderer.GetDevice(), renderer.GetContext());
    Texture     my_texture(renderer.GetDevice(), "assets/brick.png");
    GameLoop    game_loop;
    Input       input;

    static constexpr float DT = static_cast<float>(GameLoop::FIXED_DT);

    Player player(200.0f, 100.0f);
    Tilemap tilemap(0, 0, 0);
    tilemap.LoadFromFile("assets/level1.txt");

    while (window.ProcessMessages()) {
        game_loop.Tick();
        while (game_loop.ShouldUpdate()) {
            input.Poll();
            player.Update(DT, input.IsHeld(Action::MoveLeft), input.IsHeld(Action::MoveRight), input.IsPressed(Action::Jump), tilemap);
            game_loop.ConsumeUpdate();
        }

        renderer.BeginFrame(0.1f, 0.1f, 0.2f);
        sprite_batch.Begin();
            sprite_batch.Draw(player.GetX(), player.GetY(), player.GetW(), player.GetH(), my_texture, 1.0f, 1.0f, 1.0f, 1.0f);
            for (int row = 0; row < tilemap.GetRows(); ++row) {
                for (int col = 0; col < tilemap.GetCols(); ++col) {
                    const auto& tile = tilemap.GetTile(col, row);
                    int t_size = tilemap.GetTileSize();
                    int pixel_x = col * t_size;
                    int pixel_y = row * t_size;
                    
                    if (tile.type != TileType::Empty) {
                        sprite_batch.Draw(pixel_x, pixel_y, (float)t_size, (float)t_size, my_texture, 1.0f, 1.0f, 1.0f, 1.0f);
                    }
                    if (tile.type == TileType::Brick) {
                        sprite_batch.Draw(pixel_x, pixel_y, (float)t_size, (float)t_size, my_texture, 100.0f, 50.0f, 1.0f, 1.0f);
                    }
                }
            }
        sprite_batch.End();
        renderer.EndFrame();
    }
    return 0;
}
