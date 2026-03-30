#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

class Window {
public:
    // Init window
    Window(std::string_view title, int width, int height);
    ~Window();

    bool ProcessMessages();

    HWND GetHandle() const { return _hwnd; }
    int  GetWidth()  const { return _width; }
    int  GetHeight() const { return _height; }

private:
    HWND        _hwnd   = nullptr;
    int         _width  = 0;
    int         _height = 0;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam);
};
