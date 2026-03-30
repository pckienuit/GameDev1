#include "core/Window.h"
#include "renderer/Renderer.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Window   window("Mario Engine", 800, 600);
    Renderer renderer(window);  

    while (window.ProcessMessages()) {
        // --- Render ---
        renderer.BeginFrame(0.1f, 0.1f, 0.2f);
        renderer.EndFrame();
    }

    return 0;
}
