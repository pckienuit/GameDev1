#pragma once
#include <vector>
#include <d3d11.h>
#include <wrl/client.h>
#include "Sprite.h"
#include "Texture.h"

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

struct SpriteVertex {
    float x, y;      // pixel position
    float u, v;      // texture coordinate [0, 1]
    float r, g, b, a; // color
};

struct ScreenData {
    float screen_width;
    float screen_height;
    float cam_x;
    float cam_y;
};

class SpriteBatch {
public:
    explicit SpriteBatch(ID3D11Device* device, ID3D11DeviceContext* context);

    void Begin(float screen_w, float screen_h, float cam_x=0.0f, float cam_y=0.0f);

    void Draw(float x, float y, float w, float h,
              const Sprite& sprite,
              float r, float g, float b, float a,
              bool flip_x = false);

    void End();  

private:
    static constexpr int MAX_SPRITES = 1024;

    float _screen_w = 800.0f;
    float _screen_h = 600.0f;
    float _cam_x    = 0.0f;
    float _cam_y    = 0.0f;


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

    //constant buffer
    ComPtr<ID3D11Buffer>        _const_buffer;
    ID3D11Device*               _device;

    //blend state
    ComPtr<ID3D11BlendState>    _blend_state;
    
    void compile_shaders(ID3D11Device* device);
};
