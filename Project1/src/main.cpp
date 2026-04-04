#include "core/Window.h"
#include "core/GameLoop.h"
#include "core/Input.h"
#include "renderer/Renderer.h"
#include "renderer/SpriteBatch.h"
#include "renderer/Texture.h"
#include "tilemap/Tilemap.h"
#include <iostream>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Window      window("Mario Engine", 800, 600);
    Renderer    renderer(window);
    SpriteBatch sprite_batch(renderer.GetDevice(), renderer.GetContext());
    Texture     my_texture(renderer.GetDevice(), "assets/brick.png");
    GameLoop    game_loop;
    Input       input;

    float pos_x  = 100.0f;
    float pos_y  = 100.0f;
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
        
            // === MOVE & RESOLVE X ===
            pos_x += vel_x * DT;

            if (vel_x > 0.0f) {
                // check cạnh PHẢI của player
                int col_right = tilemap.PixelToCol(pos_x + PLAYER_W);
                int row_top   = tilemap.PixelToRow(pos_y + 1.0f);
                int row_bot   = tilemap.PixelToRow(pos_y + PLAYER_H - 1.0f);

                // TODO: nếu IsSolid ở col_right, row_top HOẶC row_bot:
                //       snap pos_x = col_right * tile_size - PLAYER_W
                //       vel_x = 0
                if (tilemap.IsSolid(col_right, row_top) || tilemap.IsSolid(col_right, row_bot)) {
                    pos_x = col_right * tilemap.GetTileSize() - PLAYER_W;
                    vel_x = 0;
                }
            }
            else if (vel_x < 0.0f) {
                int col_left = tilemap.PixelToCol(pos_x);
                int row_top   = tilemap.PixelToRow(pos_y + 1.0f);
                int row_bot   = tilemap.PixelToRow(pos_y + PLAYER_H - 1.0f);
                // TODO: kiểm tra cạnh TRÁI — col_left = PixelToCol(pos_x)
                //       snap: pos_x = (col_left + 1) * tile_size
                //       vel_x = 0
                if (tilemap.IsSolid(col_left, row_top) || tilemap.IsSolid(col_left, row_bot)) {
                    pos_x = (col_left + 1) * tilemap.GetTileSize();
                    vel_x = 0;
                }
            }

            // === MOVE & RESOLVE Y ===
            vel_y += GRAVITY * DT;
            pos_y += vel_y * DT;

            bool is_grounded = false;  // ← dùng flag, KHÔNG dùng vel_y == 0

            if (vel_y > 0.0f) {
                // Falling — check ĐÁYCHÂN (foot)
                // TODO: lấy tile_row từ (pos_y + PLAYER_H)
                //       check col_left và col_right
                //       nếu solid: snap pos_y, vel_y = 0, is_grounded = true
                int tile_row = tilemap.PixelToRow(pos_y + PLAYER_H);
                int col_left = tilemap.PixelToCol(pos_x + 1.0f);
                int col_right = tilemap.PixelToCol(pos_x + PLAYER_W - 1.0f);

                if (tilemap.IsSolid(col_left, tile_row) || tilemap.IsSolid(col_right, tile_row)) {
                    pos_y = tile_row * tilemap.GetTileSize() - PLAYER_H;
                    vel_y = 0;
                    is_grounded = true;
                }
            }
            else if (vel_y < 0.0f) {
                // Jumping — check ĐỈNH ĐẦU (head)
                // TODO: lấy tile_row từ pos_y (đỉnh player)
                //       check col_left và col_right
                //       nếu solid: snap pos_y = (tile_row + 1) * tile_size, vel_y = 0
                int tile_row = tilemap.PixelToRow(pos_y);
                int col_left = tilemap.PixelToCol(pos_x + 1.0f);
                int col_right = tilemap.PixelToCol(pos_x + PLAYER_W - 1.0f);
                if (tilemap.IsSolid(col_left, tile_row) || tilemap.IsSolid(col_right, tile_row)) {
                    pos_y = (tile_row+1) * tilemap.GetTileSize();
                    vel_y = 0;
                }
            }

            // Jump chỉ cho phép khi đang đứng trên đất
            if (input.IsPressed(Action::Jump) && is_grounded)
                vel_y = -JUMP_SPEED;
                
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
