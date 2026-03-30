// src/renderer/Renderer.cpp
#include "Renderer.h"
#include "core/Window.h"
#include <stdexcept>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

Renderer::Renderer(const Window& window) {

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

    D3D11_VIEWPORT vp = {};
    vp.Width          = static_cast<float>(window.GetWidth());
    vp.Height         = static_cast<float>(window.GetHeight());
    vp.MaxDepth       = 1.0f;
    _context->RSSetViewports(1, &vp);
}

void Renderer::BeginFrame(float r, float g, float b) {
    float color[4] = {r, g, b, 1.0f};
    _context->ClearRenderTargetView(_render_target_view.Get(), color);
}

void Renderer::EndFrame() {
    _swap_chain->Present(1, 0);  // 1 = vsync on
}
