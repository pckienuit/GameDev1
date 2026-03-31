#include "core/Window.h"
#include "renderer/Renderer.h"
#include "renderer/SpriteBatch.h"
#include "renderer/Texture.h"       // ← thêm

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Window      window("Mario Engine", 800, 600);
    Renderer    renderer(window);
    SpriteBatch sprite_batch(renderer.GetDevice(), renderer.GetContext());
    Texture     my_texture(renderer.GetDevice(), "assets/brick.png");  // ← thêm

    while (window.ProcessMessages()) {
        renderer.BeginFrame(0.1f, 0.1f, 0.2f);
        sprite_batch.Begin();
            sprite_batch.Draw(100, 100, 200, 150, my_texture, 1.0f, 1.0f, 1.0f, 1.0f);
        sprite_batch.End();
        renderer.EndFrame();
    }
    return 0;
}
