#include "Window.h"
#include <stdexcept>

static constexpr const char* CLASS_NAME = "MarioEngineWindow";

Window::Window(std::string_view title, int width, int height)
    : _width(width), _height(height)
{
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEX wc       = {};
    wc.cbSize           = sizeof(WNDCLASSEX);
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = WndProc;
    wc.hInstance        = hInstance;
    wc.hCursor          = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName    = CLASS_NAME;

    if (RegisterClassEx(&wc) == 0) {
        throw std::runtime_error("Failed to register window class");
    }

    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    _hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        title.data(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        (rect.right - rect.left),
        (rect.bottom-rect.top),
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );

    if (_hwnd == nullptr) {
        throw std::runtime_error("Failed to create window");
    }

    ShowWindow(_hwnd, SW_SHOW);
    UpdateWindow(_hwnd);
}

Window::~Window() {
    DestroyWindow(_hwnd);
    UnregisterClass(CLASS_NAME, GetModuleHandle(nullptr));
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT msg,
                                  WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool Window::ProcessMessages()
{
    MSG msg = {};

    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return false;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return true;
}
