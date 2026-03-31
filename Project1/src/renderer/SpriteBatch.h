#pragma once
#include <vector>
#include <d3d11.h>
#include <wrl/client.h>
#include "Texture.h"

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

struct SpriteVertex {
    float x, y;      // pixel position
    float u, v;      // texture coordinate [0, 1]
    float r, g, b, a; // color
};

class SpriteBatch {
public:
    // TODO: constructor nhận device + context, tạo buffer với MAX_SPRITES capacity
    explicit SpriteBatch(ID3D11Device* device, ID3D11DeviceContext* context);

    void Begin();  // TODO: clear _vertices

    void Draw(float x, float y, float w, float h,
              const Texture& tex,
              float r, float g, float b, float a);
    // TODO: push 4 vertices (hoặc 6 nếu dùng index buffer) vào _vertices

    void End();    // TODO: upload _vertices → GPU → Draw

private:
    static constexpr int MAX_SPRITES = 1024;

    ID3D11DeviceContext* _context;
    ComPtr<ID3D11Buffer> _vertex_buffer; // pre-allocated, D3D11_USAGE_DYNAMIC

    std::vector<SpriteVertex> _vertices; // CPU-side staging

    //shader
    ComPtr<ID3D11VertexShader> _vertex_shader;
    ComPtr<ID3D11PixelShader>  _pixel_shader;
    ComPtr<ID3D11InputLayout>  _input_layout;

    //texture
    ComPtr<ID3D11SamplerState>  _sampler;
    ID3D11ShaderResourceView*   _current_srv = nullptr;
    
    void compile_shaders(ID3D11Device* device);
};
