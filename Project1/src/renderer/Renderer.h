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
    
    void BeginFrame(float r, float g, float b);

    void EndFrame();

    ID3D11Device* GetDevice() const { return _device.Get(); }
    ID3D11DeviceContext* GetContext() const { return _context.Get(); }

private:
    ComPtr<ID3D11Device>        _device;
    ComPtr<ID3D11DeviceContext> _context;
    ComPtr<IDXGISwapChain>      _swap_chain;

    ComPtr<ID3D11RenderTargetView>  _render_target_view;
    
    //Buffer
    ComPtr<ID3D11Buffer> _screen_cb;
    ComPtr<ID3D11RasterizerState> _rasterizer_state;  // 2D: no culling
};
