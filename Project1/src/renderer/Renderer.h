#pragma once

#include <d3d11.h>
#include <wrl/client.h>

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class Window;

class Renderer {
public:
    explicit Renderer(const Window& window);

    ~Renderer() = default;
    
    void Resize(int width, int height);
    void SetViewport(int width, int height);

    void BeginFrame(float r, float g, float b);

    void EndFrame();

    int GetWidth() const { return _width; }
    int GetHeight() const { return _height; }

    ID3D11Device* GetDevice() const { return _device.Get(); }
    ID3D11DeviceContext* GetContext() const { return _context.Get(); }

private:
    int _width = 0;
    int _height = 0;

    ComPtr<ID3D11Device>        _device;
    ComPtr<ID3D11DeviceContext> _context;
    ComPtr<IDXGISwapChain>      _swap_chain;

    ComPtr<ID3D11RenderTargetView>  _render_target_view;
    ComPtr<ID3D11RasterizerState>   _rasterizer_state;
    ComPtr<ID3D11Buffer>            _screen_cb;
};
