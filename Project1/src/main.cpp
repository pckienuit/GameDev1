#include "core/Window.h"
#include "renderer/Renderer.h"
#include "renderer/SpriteBatch.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Window   window("Mario Engine", 800, 600);
    Renderer renderer(window);  
    SpriteBatch sprite_batch(renderer.GetDevice(), renderer.GetContext());

    while (window.ProcessMessages()) {
        // --- Render ---
        renderer.BeginFrame(0.1f, 0.1f, 0.2f);

        sprite_batch.Begin();
            sprite_batch.Draw(100, 100, 200, 150, 1.0f, 0.5f, 0.0f, 1.0f);
            sprite_batch.Draw(310, 100, 200, 150, 1.0f, 0.5f, 0.0f, 1.0f);
            sprite_batch.Draw(520, 100, 200, 150, 1.0f, 0.5f, 0.0f, 1.0f);
            sprite_batch.Draw(730, 100, 200, 150, 1.0f, 0.5f, 0.0f, 1.0f);
        sprite_batch.End();

        renderer.EndFrame();
    }

    return 0;
}
