#pragma once
#include <unordered_map>
#include <string>
#include <d3d11.h>
#include <memory>
#include "Texture.h"

class TextureRegistry {
public:
    Texture* Load(ID3D11Device* device, const std::string& path);

private:
    std::unordered_map< std::string, std::unique_ptr<Texture> > _cache;
};
