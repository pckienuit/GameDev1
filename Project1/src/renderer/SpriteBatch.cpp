#include "SpriteBatch.h"
#include <stdexcept>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")


static constexpr int NUM_VERTICES_PER_SPRITE = 6;

SpriteBatch::SpriteBatch(ID3D11Device* device, ID3D11DeviceContext* context) {
    _context = context;

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth      = sizeof(SpriteVertex) * MAX_SPRITES * NUM_VERTICES_PER_SPRITE;
    bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = device->CreateBuffer(&bd, nullptr, &_vertex_buffer);
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

    _vertices.reserve(MAX_SPRITES * NUM_VERTICES_PER_SPRITE);

    compile_shaders(device);
}

void SpriteBatch::Begin() {
    _vertices.clear();
}

void SpriteBatch::Draw(float x, float y, float w, float h,
                       const Texture& tex,
                       float r, float g, float b, float a) {
 
    if (_vertices.size() + NUM_VERTICES_PER_SPRITE > MAX_SPRITES*NUM_VERTICES_PER_SPRITE) {
        End();
        Begin();
    }
    
    _current_srv = tex.GetSRV();

    //Tria1
    _vertices.push_back({x, y, 0.0f, 0.0f, r, g, b, a}); //TL
    _vertices.push_back({x + w, y, 1.0f, 0.0f, r, g, b, a}); //TR
    _vertices.push_back({x, y + h, 0.0f, 1.0f, r, g, b, a}); //BL

    //Tria2
    _vertices.push_back({x, y + h, 0.0f, 1.0f, r, g, b, a}); //BL
    _vertices.push_back({x + w, y + h, 1.0f, 1.0f, r, g, b, a}); //BR
    _vertices.push_back({x + w, y, 1.0f, 0.0f, r, g, b, a}); //TR
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
