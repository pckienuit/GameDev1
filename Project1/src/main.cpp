#include "core/Window.h"
#include "core/GameLoop.h"
#include "core/Input.h"
#include "renderer/Renderer.h"
#include "renderer/SpriteBatch.h"
#include "renderer/Texture.h"
#include "tilemap/Tilemap.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Window      window("Mario Engine", 800, 600);
    Renderer    renderer(window);
    SpriteBatch sprite_batch(renderer.GetDevice(), renderer.GetContext());
    Texture     my_texture(renderer.GetDevice(), "assets/brick.png");
    GameLoop    game_loop;
    Input       input;

    float pos_x  = 100.0f;
    float pos_y  = 400.0f;
    float vel_x  = 0.0f;
    float vel_y  = 0.0f;
    constexpr float GROUND_Y    = 400.0f;
    constexpr float SPEED       = 200.0f;  // px/s
    constexpr float JUMP_SPEED  = 800.0f;  // px/s
    constexpr float GRAVITY     = 1200.0f;  // px/s²
    constexpr float DT          = static_cast<float>(GameLoop::FIXED_DT);
    constexpr float PLAYER_W    = 64.0f;
    constexpr float PLAYER_H    = 64.0f;

    Tilemap tilemap(0, 0, 0);
    tilemap.LoadFromFile("assets/level1.txt");

    while (window.ProcessMessages()) {
        game_loop.Tick();
        while (game_loop.ShouldUpdate()) {
            input.Poll();

            vel_x = 0.0f;
            if (input.IsHeld(Action::MoveLeft))  vel_x = -SPEED;
            if (input.IsHeld(Action::MoveRight)) vel_x =  SPEED;
            
            bool on_ground = (vel_y == 0.0f);
            if (input.IsPressed(Action::Jump) && on_ground)
                vel_y = -JUMP_SPEED;    

            vel_y += GRAVITY * DT;
            pos_x += vel_x * DT;
            pos_y += vel_y * DT;
            
            if (vel_y > 0.0f) {
                float foot_y  = pos_y + PLAYER_H;
                int   tile_row = tilemap.PixelToRow(foot_y);
                int   col_left  = tilemap.PixelToCol(pos_x + 1.0f);          // sát cạnh trái
                int   col_right = tilemap.PixelToCol(pos_x + PLAYER_W - 1.0f); // sát cạnh phải
                // TODO: check tilemap.GetTile(col_left, tile_row) và col_right
                // TODO: nếu solid → snap pos_y, vel_y = 0
                // HINT: cần guard tile_row < tilemap.GetRows() để tránh out-of-bounds!
         
                if (tilemap.IsSolid(col_left, tile_row) || tilemap.IsSolid(col_right, tile_row)) {
                    pos_y = tile_row * tilemap.GetTileSize() - PLAYER_H;
                    vel_y = 0.0f;
                }
            }
            
            game_loop.ConsumeUpdate();
        }

        renderer.BeginFrame(0.1f, 0.1f, 0.2f);
        sprite_batch.Begin();
            sprite_batch.Draw(pos_x, pos_y, PLAYER_W, PLAYER_H, my_texture, 1.0f, 1.0f, 1.0f, 1.0f);
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
