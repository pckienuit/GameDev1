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
    
    void DrawQuad(float x, float y, float w, float h,
              float r, float g, float b, float a = 1.0f);

    void BeginFrame(float r, float g, float b);

    void EndFrame();

private:
    ComPtr<ID3D11Device>        _device;
    ComPtr<ID3D11DeviceContext> _context;
    ComPtr<IDXGISwapChain>      _swap_chain;

    ComPtr<ID3D11RenderTargetView>  _render_target_view;

    // Shader objects
    ComPtr<ID3D11VertexShader>  _vertex_shader;
    ComPtr<ID3D11PixelShader>   _pixel_shader;
    ComPtr<ID3D11InputLayout>   _input_layout;
    
    //Buffer
    ComPtr<ID3D11Buffer> _screen_cb;
    ComPtr<ID3D11Buffer> _quad_vb;
    ComPtr<ID3D11RasterizerState> _rasterizer_state;  // 2D: no culling

    // Helpers
    void load_shaders(const Window& window);
};
