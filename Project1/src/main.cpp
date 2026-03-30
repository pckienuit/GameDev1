#include "core/Window.h"
#include "renderer/Renderer.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Window   window("Mario Engine", 800, 600);
    Renderer renderer(window);  

    while (window.ProcessMessages()) {
        // --- Render ---
        renderer.BeginFrame(0.1f, 0.1f, 0.2f);
        renderer.DrawQuad(100, 100, 200, 150, 1.0f, 0.5f, 0.0f);
        renderer.EndFrame();
    }

    return 0;
}
