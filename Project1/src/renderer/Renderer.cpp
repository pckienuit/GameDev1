#include "Renderer.h"
#include "core/Window.h"
#include <stdexcept>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// Phải khớp với cbuffer trong HLSL — bội số 16 bytes
struct ScreenConstants {
    float screen_width;
    float screen_height;
    float padding[2];
};

Renderer::Renderer(const Window& window) : _width(window.GetWidth()), _height(window.GetHeight()) {

    DXGI_SWAP_CHAIN_DESC sd               = {};
    sd.BufferCount                        = 1;
    sd.BufferDesc.Width                   = window.GetWidth();
    sd.BufferDesc.Height                  = window.GetHeight();
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = window.GetHandle();
    sd.SampleDesc.Count                   = 1;  // No MSAA for now
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr, flags,
        nullptr, 0,
        D3D11_SDK_VERSION,
        &sd,
        &_swap_chain,
        &_device,
        nullptr,
        &_context
    );

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create device and swap chain");
    }

    ComPtr<ID3D11Texture2D> back_buffer;

    hr = _swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                    reinterpret_cast<void**>(back_buffer.GetAddressOf()));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to get back buffer");
    }

    hr = _device->CreateRenderTargetView(back_buffer.Get(), nullptr,
                                              &_render_target_view);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create render target view");
    }
    
    _context->OMSetRenderTargets(1, _render_target_view.GetAddressOf(), nullptr);

    // 2D engine: disable backface culling (sprites can be flipped)
    D3D11_RASTERIZER_DESC rs_desc = {};
    rs_desc.FillMode              = D3D11_FILL_SOLID;
    rs_desc.CullMode              = D3D11_CULL_NONE;
    rs_desc.FrontCounterClockwise = FALSE;
    _device->CreateRasterizerState(&rs_desc, &_rasterizer_state);
    _context->RSSetState(_rasterizer_state.Get());

    // Create screen constant buffer (will be updated on resize)
    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth  = sizeof(ScreenConstants);
    cbd.Usage      = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags  = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = _device->CreateBuffer(&cbd, nullptr, &_screen_cb);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create screen constant buffer");
    }

    // Set initial viewport
    SetViewport(window.GetWidth(), window.GetHeight());
}

void Renderer::SetViewport(int width, int height) {
    constexpr int GAME_WIDTH = 800;
    constexpr int GAME_HEIGHT = 600;

    float window_aspect = static_cast<float>(width) / static_cast<float>(height);
    float game_aspect = static_cast<float>(GAME_WIDTH) / static_cast<float>(GAME_HEIGHT);

    float vp_width, vp_height;
    if (window_aspect > game_aspect) {
        // Window is wider: pillarbox (black bars on sides)
        vp_height = static_cast<float>(GAME_HEIGHT);
        vp_width = vp_height * game_aspect;
    } else {
        // Window is taller: letterbox (black bars on top/bottom)
        vp_width = static_cast<float>(GAME_WIDTH);
        vp_height = vp_width / game_aspect;
    }

    D3D11_VIEWPORT vp = {};
    vp.Width    = vp_width;
    vp.Height   = vp_height;
    vp.TopLeftX = (static_cast<float>(width) - vp_width) / 2.0f;
    vp.TopLeftY = (static_cast<float>(height) - vp_height) / 2.0f;
    vp.MaxDepth = 1.0f;
    _context->RSSetViewports(1, &vp);
}

void Renderer::Resize(int width, int height) {
    if (width <= 0 || height <= 0) return;

    _width = width;
    _height = height;

    // Unset all render targets and shader resources, then flush
    ID3D11ShaderResourceView* null_srv[16] = { nullptr };
    _context->VSSetShaderResources(0, 16, null_srv);
    _context->PSSetShaderResources(0, 16, null_srv);
    _context->OMSetRenderTargets(0, nullptr, nullptr);
    _context->Flush();

    // Release old render target
    _render_target_view.Reset();

    // Resize swap chain to match window
    HRESULT hr = _swap_chain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to resize swap chain");
    }

    // Recreate render target
    ComPtr<ID3D11Texture2D> back_buffer;
    hr = _swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                reinterpret_cast<void**>(back_buffer.GetAddressOf()));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to get back buffer after resize");
    }

    hr = _device->CreateRenderTargetView(back_buffer.Get(), nullptr,
                                          &_render_target_view);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create render target view after resize");
    }

    _context->OMSetRenderTargets(1, _render_target_view.GetAddressOf(), nullptr);

    // Update viewport for letterboxing
    SetViewport(width, height);
}

void Renderer::BeginFrame(float r, float g, float b) {
    // Update screen constants for world size (800x720)
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = _context->Map(_screen_cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr)) {
        ScreenConstants sc = { 800.0f, 720.0f };  // Fixed world size
        memcpy(mapped.pData, &sc, sizeof(sc));
        _context->Unmap(_screen_cb.Get(), 0);
    }

    _context->VSSetConstantBuffers(0, 1, _screen_cb.GetAddressOf());
    
    // Clear entire backbuffer (including letterbox areas)
    float color[4] = {r, g, b, 1.0f};
    _context->ClearRenderTargetView(_render_target_view.Get(), color);
}


void Renderer::EndFrame() {
    _swap_chain->Present(1, 0);  // 1 = vsync on
}
