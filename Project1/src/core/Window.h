#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <functional>

class Window {
public:
    // Init window
    Window(std::string_view title, int width, int height);
    ~Window();

    bool ProcessMessages();

    // Resize callback: called when window size changes
    void SetResizeCallback(std::function<void(int, int)> callback) { _resize_callback = callback; }

    HWND GetHandle() const { return _hwnd; }
    int  GetWidth()  const { return _width; }
    int  GetHeight() const { return _height; }

private:
    HWND        _hwnd   = nullptr;
    int         _width  = 0;
    int         _height = 0;
    std::function<void(int, int)> _resize_callback;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam);
};
