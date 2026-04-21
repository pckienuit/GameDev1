#pragma once
#include "Sprite.h"
#include "Texture.h"
#include "TextureRegistry.h"
#include "SpriteID.h"
#include <array>
#include <unordered_map>
#include <string>
#include <d3d11.h>

class SpriteSheet {
public:
    explicit SpriteSheet(TextureRegistry& registry, ID3D11Device* device);

    // Load texture vào registry (idempotent — gọi nhiều lần vẫn OK)
    void LoadTexture(const std::string& path);

    // Define một sprite by enum ID
    void Define(SpriteID id, const std::string& texture_path,
                int x, int y, int w, int h);

    // Define bằng tên string (đồng thời map name -> id)
    void DefineNamed(SpriteID id, const std::string& name,
                     const std::string& texture_path,
                     int x, int y, int w, int h);

    // Define dải frame liên tiếp (first_id, first_id+1, ...)
    // gap: pixel gap giữa các frame (nếu có)
    void DefineStrip(SpriteID first_id, int count,
                     const std::string& texture_path,
                     int start_x, int y, int frame_w, int frame_h,
                     int gap = 0);

    // --- Truy cập ---
    const Sprite& Get(SpriteID id) const;              // by enum — O(1)
    const Sprite& Get(const std::string& name) const;  // by string name
    const Sprite& GetByIndex(SpriteID first, int i) const; // strip offset

    bool Has(SpriteID id) const;
    bool Has(const std::string& name) const;

private:
    TextureRegistry& _registry;
    ID3D11Device*    _device;

    std::array<Sprite, static_cast<int>(SpriteID::Count)> _sprites;
    std::unordered_map<std::string, SpriteID>              _name_to_id;
    bool _defined[static_cast<int>(SpriteID::Count)] = {};
};
