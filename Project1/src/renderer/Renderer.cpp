// src/renderer/Renderer.cpp
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

    load_shaders(window);
}

void Renderer::BeginFrame(float r, float g, float b) {
    // Bind constant buffer vào slot b0 của VS
    _context->VSSetConstantBuffers(0, 1, _screen_cb.GetAddressOf());
    
    float color[4] = {r, g, b, 1.0f};
    _context->ClearRenderTargetView(_render_target_view.Get(), color);
}


void Renderer::EndFrame() {
    _swap_chain->Present(1, 0);  // 1 = vsync on
}

void Renderer::load_shaders(const Window& window) {
    ComPtr<ID3DBlob> vs_blob, ps_blob, error_blob;

    // Compile Vertex Shader từ file
    HRESULT hr = D3DCompileFromFile(
        L"shaders\\sprite.vs.hlsl", nullptr, nullptr,
        "main", "vs_5_0",
        D3DCOMPILE_DEBUG, 0,
        &vs_blob, &error_blob
    );
    if (FAILED(hr)) {
        if (error_blob)
            OutputDebugStringA((char*)error_blob->GetBufferPointer());
        throw std::runtime_error("Failed to compile vertex shader");
    }

    // TODO ①: Compile Pixel Shader tương tự
    //          Đổi "sprite.vs.hlsl" → "sprite.ps.hlsl"
    //          Đổi "vs_5_0" → "ps_5_0"
    //          Lưu vào ps_blob
    hr = D3DCompileFromFile(
        L"shaders\\sprite.ps.hlsl", nullptr, nullptr,
        "main", "ps_5_0",
        D3DCOMPILE_DEBUG, 0,
        &ps_blob, &error_blob
    );
    if (FAILED(hr)) {
        if (error_blob)
            OutputDebugStringA((char*)error_blob->GetBufferPointer());
        throw std::runtime_error("Failed to compile pixel shader");
    }

    // Tạo shader objects từ bytecode
    // TODO ②: _device->CreateVertexShader(
    //              vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
    //              nullptr, &_vertex_shader)

    // TODO ③: _device->CreatePixelShader(...)
    //          tương tự nhưng dùng ps_blob và &_pixel_shader
    hr = _device->CreateVertexShader(
        vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
        nullptr, &_vertex_shader
    );
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create vertex shader");
    }

    hr = _device->CreatePixelShader(
        ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(),
        nullptr, &_pixel_shader
    );
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create pixel shader");
    }
    

    // Mô tả layout vertex cho GPU — phải khớp với C++ struct sẽ tạo sau
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0,  8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //                                                        ^^ offset: COLOR bắt đầu sau 8 bytes của float2
    };

    // TODO ④: _device->CreateInputLayout(
    //              layout, 2,
    //              vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
    //              &_input_layout)
    hr = _device->CreateInputLayout(
        layout, 2,
        vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(),
        &_input_layout
    );
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create input layout");
    }

    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth      = sizeof(ScreenConstants);
    cbd.Usage          = D3D11_USAGE_IMMUTABLE;  // không đổi sau khi tạo
    cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;

    ScreenConstants sc = { window.GetWidth(), window.GetHeight() };  // TODO: dùng window.GetWidth/Height

    D3D11_SUBRESOURCE_DATA init_data = {};
    init_data.pSysMem = &sc;

    // TODO: _device->CreateBuffer(&cbd, &init_data, &_screen_cb)
    //       Kiểm tra FAILED(hr) → throw

    hr = _device->CreateBuffer(&cbd, &init_data, &_screen_cb);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create constant buffer");
    }
}
