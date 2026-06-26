#include "TextureRegistry.h"

Texture* TextureRegistry::Load(ID3D11Device* device, const std::string& path) {
    auto it = _cache.find(path);
    if (it != _cache.end())
        return it->second.get();

    auto tex = std::make_unique<Texture>(device, path.c_str());
    Texture* ptr = tex.get();
    _cache.emplace(path, std::move(tex));
    return ptr;
}

Texture* TextureRegistry::LoadWithColorKey(ID3D11Device* device, const std::string& path,
                                          unsigned char r, unsigned char g, unsigned char b) {
    auto it = _cache.find(path);
    if (it != _cache.end())
        return it->second.get();

    auto tex = std::make_unique<Texture>(device, path.c_str(), r, g, b);
    Texture* ptr = tex.get();
    _cache.emplace(path, std::move(tex));
    return ptr;
}
