#include "core/Window.h"
#include "core/GameLoop.h"
#include "core/Input.h"
#include "renderer/Renderer.h"
#include "renderer/SpriteBatch.h"
#include "renderer/Texture.h"
#include "tilemap/Tilemap.h"
#include "game/Player.h"
#include "renderer/TextureRegistry.h"
#include "renderer/Sprite.h"
#include "renderer/Camera.h"
#include "collision/CollisionSystem.h"
#include <iostream>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Window      window("Mario Engine", 800, 600);
    Renderer    renderer(window);
    SpriteBatch sprite_batch(renderer.GetDevice(), renderer.GetContext());
    Texture     my_texture(renderer.GetDevice(), "assets/brick.png");
    Texture     mario_texture(renderer.GetDevice(), "assets/mario.png");
    GameLoop    game_loop;
    Input       input;

    static constexpr float DT = static_cast<float>(GameLoop::FIXED_DT);

    EntityManager entity_manager;

    Player player(200.0f, 100.0f, &mario_texture, entity_manager);
    Camera camera(800.0f, 600.0f);
    
    Tilemap tilemap(0, 0, 0);
    tilemap.LoadFromFile("assets/level1.txt");
    
    CollisionSystem collision_system(static_cast<int>(tilemap.GetWidth()), static_cast<int>(tilemap.GetHeight()), 32, 1024);
    // Sprite  player_sprite(&my_texture, 0, 0, my_texture.GetWidth(), my_texture.GetHeight());
    Sprite  brick_sprite(&my_texture, 0, 0, my_texture.GetWidth(), my_texture.GetHeight());
    const CollisionEventPool& pool = collision_system.GetEvents();
    
    while (window.ProcessMessages()) {
        game_loop.Tick();
        
        while (game_loop.ShouldUpdate()) {
            input.Poll();
            player.Update(DT, input.IsHeld(Action::MoveLeft), input.IsHeld(Action::MoveRight), input.IsPressed(Action::Jump), tilemap);
            collision_system.BeginFrame();
            collision_system.Register(player.GetID(), player.GetAABB());
            collision_system.Detect();
            for (int i = 0; i < pool.Count(); ++i) {
                const CollisionEvent& ev = pool.Get(i);
                
                float push_x = ev.normal_x * ev.depth * 0.5f;
                float push_y = ev.normal_y * ev.depth * 0.5f;
                
                if (ev.entity_a == player.GetID()) {
                    player.ApplyPush(push_x, push_y);
                }
                if (ev.entity_b == player.GetID()) {
                    player.ApplyPush(-push_x, -push_y);
                }
            }
            camera.Follow(player.GetX(), player.GetY(), DT);
            //char buf[128];
            //sprintf_s(buf, "cam=%.0f player=%.0f\n",
            //    camera.GetX(), player.GetX());
            //OutputDebugStringA(buf);
            camera.Clamp(tilemap.GetWidth(), tilemap.GetHeight());
            game_loop.ConsumeUpdate();
        }

        renderer.BeginFrame(0.1f, 0.1f, 0.2f);
        sprite_batch.Begin(800.0f, 600.0f, camera.GetX(), camera.GetY());
            sprite_batch.Draw(player.GetX(), player.GetY(), player.GetW(), player.GetH(), player.GetSprite(DT), 1.0f, 1.0f, 1.0f, 1.0f, player.IsFacingLeft());
            for (int row = 0; row < tilemap.GetRows(); ++row) {
                for (int col = 0; col < tilemap.GetCols(); ++col) {
                    const auto& tile = tilemap.GetTile(col, row);
                    int t_size = tilemap.GetTileSize();
                    int pixel_x = col * t_size;
                    int pixel_y = row * t_size;
                    
                    if (tile.type != TileType::Empty) {
                        sprite_batch.Draw(pixel_x, pixel_y, (float)t_size, (float)t_size, brick_sprite, 1.0f, 1.0f, 1.0f, 1.0f);
                    }
                }
            }
        sprite_batch.End();
        renderer.EndFrame();
    }
    return 0;
}
