#include "SpriteBatch.h"
#include <stdexcept>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")


static constexpr int NUM_VERTICES_PER_SPRITE = 6;

SpriteBatch::SpriteBatch(ID3D11Device* device, ID3D11DeviceContext* context) {
    _context = context;
    _device  = device;

    // Blend state
    D3D11_BLEND_DESC blend_desc = {};
    blend_desc.RenderTarget[0].BlendEnable           = TRUE;
    blend_desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
    blend_desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = device->CreateBlendState(&blend_desc, &_blend_state);
    if (FAILED(hr)) throw std::runtime_error("Failed to create blend state");


    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth      = sizeof(SpriteVertex) * MAX_SPRITES * NUM_VERTICES_PER_SPRITE;
    bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = device->CreateBuffer(&bd, nullptr, &_vertex_buffer);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create vertex buffer");
    }

    D3D11_SAMPLER_DESC sd    = {};
    sd.Filter                = D3D11_FILTER_MIN_MAG_MIP_POINT; // pixel art style
    sd.AddressU              = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV              = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW              = D3D11_TEXTURE_ADDRESS_CLAMP;

    
    hr = device->CreateSamplerState(&sd, &_sampler);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create sampler state");
    }
    
    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage                = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth            = sizeof(ScreenData);
    cbd.BindFlags            = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags       = D3D11_CPU_ACCESS_WRITE;
    
    hr = device->CreateBuffer(&cbd, nullptr, &_const_buffer);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create constant buffer");
    }

    _vertices.reserve(MAX_SPRITES * NUM_VERTICES_PER_SPRITE);

    compile_shaders(device);
}

void SpriteBatch::Begin(float screen_w, float screen_h, float cam_x, float cam_y) {
    _vertices.clear();

    ScreenData data = { screen_w, screen_h, cam_x, cam_y };

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = _context->Map(_const_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to map constant buffer");
    }

    // Update constant buffer
    memcpy(mapped.pData, &data, sizeof(data));
    _context->Unmap(_const_buffer.Get(), 0);
    _context->VSSetConstantBuffers(0, 1, _const_buffer.GetAddressOf());

    // Set blend state
    float blend_factor[4] = {1,1,1,1};
    _context->OMSetBlendState(_blend_state.Get(), blend_factor, 0xFFFFFFFF);
}

void SpriteBatch::Draw(float x, float y, float w, float h,
                       const Sprite& sprite,
                       float r, float g, float b, float a,
                       bool flip_x) {
 
    bool texture_changed = (_current_srv != nullptr && 
                            _current_srv != sprite.texture->GetSRV());
    if (_vertices.size() + NUM_VERTICES_PER_SPRITE > MAX_SPRITES*NUM_VERTICES_PER_SPRITE
        || texture_changed) {
        End();
        Begin(_screen_w, _screen_h, _cam_x, _cam_y);
    }
    _current_srv = sprite.texture->GetSRV();
    
    float u0 = (float)sprite.src_x / sprite.texture->GetWidth();
    float v0 = (float)sprite.src_y / sprite.texture->GetHeight();
    float u1 = (float)(sprite.src_x + sprite.src_w) / sprite.texture->GetWidth();
    float v1 = (float)(sprite.src_y + sprite.src_h) / sprite.texture->GetHeight();

    //Flip sprite
    if (flip_x) {
        std::swap(u0, u1);
    }

    //Tria1
    _vertices.push_back({x, y, u0, v0, r, g, b, a}); //TL
    _vertices.push_back({x + w, y, u1, v0, r, g, b, a}); //TR
    _vertices.push_back({x, y + h, u0, v1, r, g, b, a}); //BL

    //Tria2
    _vertices.push_back({x, y + h, u0, v1, r, g, b, a}); //BL
    _vertices.push_back({x + w, y + h, u1, v1, r, g, b, a}); //BR
    _vertices.push_back({x + w, y, u1, v0, r, g, b, a}); //TR
}

void SpriteBatch::End() {
    if (_vertices.empty()) return;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = _context->Map(_vertex_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD,
                                 0, &mapped);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to map vertex buffer");
    }

    memcpy(mapped.pData, _vertices.data(), _vertices.size() * sizeof(SpriteVertex));
    _context->Unmap(_vertex_buffer.Get(), 0);

    UINT stride = sizeof(SpriteVertex);
    UINT offset = 0;
    _context->IASetVertexBuffers(0, 1, _vertex_buffer.GetAddressOf(), &stride, &offset);
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    _context->PSSetShader(_pixel_shader.Get(), nullptr, 0);
    _context->VSSetShader(_vertex_shader.Get(), nullptr, 0);
    _context->IASetInputLayout(_input_layout.Get());
    
    _context->PSSetShaderResources(0, 1, &_current_srv);
    _context->PSSetSamplers(0, 1, _sampler.GetAddressOf());

    _context->Draw((UINT)_vertices.size(), 0);
  
}

void SpriteBatch::compile_shaders(ID3D11Device* device) {
    ComPtr<ID3DBlob> vs_blob, ps_blob, error_blob;
    
    // Vertex shader
    HRESULT hr = D3DCompileFromFile(L"shaders\\sprite.vs.hlsl", nullptr, nullptr,
                            "main", "vs_5_0", 0, 0, &vs_blob, &error_blob);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to compile vertex shader");
    }

    hr = device->CreateVertexShader(vs_blob->GetBufferPointer(),
                                    vs_blob->GetBufferSize(),
                                    nullptr, &_vertex_shader);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create vertex shader");
    }

    // Input layout
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    hr = device->CreateInputLayout(ied, 3,
                                   vs_blob->GetBufferPointer(),
                                   vs_blob->GetBufferSize(),
                                   &_input_layout);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create input layout");
    }

    // Pixel shader
    hr = D3DCompileFromFile(L"shaders\\sprite.ps.hlsl", nullptr, nullptr,
                            "main", "ps_5_0", 0, 0, &ps_blob, &error_blob);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to compile pixel shader");
    }

    hr = device->CreatePixelShader(ps_blob->GetBufferPointer(),
                                   ps_blob->GetBufferSize(),
                                   nullptr, &_pixel_shader);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create pixel shader");
    }
}
