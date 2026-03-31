#pragma once
#include <d3d11.h>
#include <wrl/client.h>

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class Texture {
public:
    explicit Texture(ID3D11Device* device, const char* path);

    // TODO: getter trả về ShaderResourceView để bind vào PS
    ID3D11ShaderResourceView* GetSRV() const { return _srv.Get(); }

    int GetWidth()  const { return _width; }
    int GetHeight() const { return _height; }

private:
    ComPtr<ID3D11ShaderResourceView> _srv;
    int _width  = 0;
    int _height = 0;
};
