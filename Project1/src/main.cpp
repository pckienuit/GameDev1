#include "core/Window.h"
#include "core/GameLoop.h"
#include "core/Input.h"
#include "renderer/Renderer.h"
#include "renderer/SpriteBatch.h"
#include "renderer/Texture.h"       // ← thêm

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

    while (window.ProcessMessages()) {
        game_loop.Tick();
        while (game_loop.ShouldUpdate()) {
            input.Poll();

            vel_x = 0.0f;
            if (input.IsHeld(Action::MoveLeft))  vel_x = -SPEED;
            if (input.IsHeld(Action::MoveRight)) vel_x =  SPEED;
            if (input.IsPressed(Action::Jump) && pos_y >= GROUND_Y)
                vel_y = -JUMP_SPEED;    

            vel_y += GRAVITY * DT;
            pos_x += vel_x * DT;
            pos_y += vel_y * DT;
            if (pos_y >= GROUND_Y) {  // chạm đất
                pos_y = GROUND_Y;
                vel_y = 0.0f;
            }
            
            game_loop.ConsumeUpdate();
        }

        renderer.BeginFrame(0.1f, 0.1f, 0.2f);
        sprite_batch.Begin();
            sprite_batch.Draw(pos_x, pos_y, 64, 64, my_texture, 1.0f, 1.0f, 1.0f, 1.0f);
        sprite_batch.End();
        renderer.EndFrame();
    }
    return 0;
}
