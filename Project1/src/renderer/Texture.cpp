#define STB_IMAGE_IMPLEMENTATION   // must be defined exactly once per project!
#include "../../third_party/stb_image.h"
#include "Texture.h"
#include <stdexcept>

// ---- shared helper ----
static void CreateD3DTexture(ID3D11Device* device,
                             unsigned char* pixels, int width, int height,
                             Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv)
{
    D3D11_TEXTURE2D_DESC desc   = {};
    desc.Width                  = width;
    desc.Height                 = height;
    desc.MipLevels              = 1;
    desc.ArraySize              = 1;
    desc.Format                 = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count       = 1;
    desc.Usage                  = D3D11_USAGE_DEFAULT;
    desc.BindFlags              = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init_data = {};
    init_data.pSysMem                = pixels;
    init_data.SysMemPitch            = width * 4;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    HRESULT hr = device->CreateTexture2D(&desc, &init_data, &texture);
    if (FAILED(hr)) throw std::runtime_error("Failed to create texture");

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format                          = desc.Format;
    srv_desc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels            = 1;
    hr = device->CreateShaderResourceView(texture.Get(), &srv_desc, &srv);
    if (FAILED(hr)) throw std::runtime_error("Failed to create shader resource view");
}

// ---- plain constructor ----
Texture::Texture(ID3D11Device* device, const char* path) {
    int channels;
    unsigned char* pixels = stbi_load(path, &_width, &_height, &channels, 4);
    if (!pixels) throw std::runtime_error(std::string("Failed to load texture: ") + path);

    CreateD3DTexture(device, pixels, _width, _height, _srv);
    stbi_image_free(pixels);
}

// ---- color-key constructor ----
// Pixels whose (R, G, B) match (key_r, key_g, key_b) are made fully transparent.
Texture::Texture(ID3D11Device* device, const char* path,
                 unsigned char key_r, unsigned char key_g, unsigned char key_b)
{
    int channels;
    unsigned char* pixels = stbi_load(path, &_width, &_height, &channels, 4);
    if (!pixels) throw std::runtime_error(std::string("Failed to load texture: ") + path);

    const int pixel_count = _width * _height;
    for (int i = 0; i < pixel_count; ++i) {
        unsigned char* p = pixels + i * 4;  // RGBA
        const int tol = 8;  // ±8 tolerance for compression artifacts
        if (std::abs((int)p[0] - key_r) <= tol &&
            std::abs((int)p[1] - key_g) <= tol &&
            std::abs((int)p[2] - key_b) <= tol)
        {
            p[3] = 0;  // alpha = 0 → transparent
        }
    }

    CreateD3DTexture(device, pixels, _width, _height, _srv);
    stbi_image_free(pixels);
}

// ---- solid-color 1x1 constructor ----
Texture::Texture(ID3D11Device* device,
                 unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    _width  = 1;
    _height = 1;
    unsigned char pixels[4] = { r, g, b, a };
    CreateD3DTexture(device, pixels, 1, 1, _srv);
}
