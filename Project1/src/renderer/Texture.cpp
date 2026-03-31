#define STB_IMAGE_IMPLEMENTATION   // chỉ 1 lần trong toàn project!
#include "../../third_party/stb_image.h"
#include "Texture.h"
#include <stdexcept>

Texture::Texture(ID3D11Device* device, const char* path) {
    // 1. Load pixels từ file:
    int channels;
    unsigned char* pixels = stbi_load(path, &_width, &_height, &channels, 4); // force RGBA
    if (!pixels) throw std::runtime_error("Failed to load texture");

    // 2. TODO: Tạo D3D11_TEXTURE2D_DESC → CreateTexture2D
    D3D11_TEXTURE2D_DESC desc   = {};
    desc.Width                  = _width;
    desc.Height                 = _height;
    desc.MipLevels              = 1;
    desc.ArraySize              = 1;
    desc.Format                 = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count       = 1;
    desc.Usage                  = D3D11_USAGE_DEFAULT;
    desc.BindFlags              = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init_data = {};
    init_data.pSysMem                = pixels;
    init_data.SysMemPitch            = _width * 4;

    ComPtr<ID3D11Texture2D> texture;
    HRESULT hr = device->CreateTexture2D(&desc, &init_data, &texture);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create texture");
    }
    // 3. TODO: Tạo CreateShaderResourceView → _srv
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    hr = device->CreateShaderResourceView(texture.Get(), &srv_desc, &_srv);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create shader resource view");
    }
    // 4. stbi_image_free(pixels);
    stbi_image_free(pixels);
}
