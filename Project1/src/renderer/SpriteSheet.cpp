#include "SpriteSheet.h"
#include <cassert>
#include <stdexcept>

SpriteSheet::SpriteSheet(TextureRegistry& registry, ID3D11Device* device)
    : _registry(registry), _device(device) {}

void SpriteSheet::LoadTexture(const std::string& path) {
    _registry.Load(_device, path);
}

void SpriteSheet::Define(SpriteID id, const std::string& texture_path,
                         int x, int y, int w, int h) {
    int idx = static_cast<int>(id);
    assert(idx >= 0 && idx < static_cast<int>(SpriteID::Count));
    
    Texture* tex = _registry.Load(_device, texture_path);
    _sprites[idx] = Sprite(tex, x, y, w, h);
    _defined[idx] = true;
}

void SpriteSheet::DefineNamed(SpriteID id, const std::string& name,
                              const std::string& texture_path,
                              int x, int y, int w, int h) {
    Define(id, texture_path, x, y, w, h);
    _name_to_id[name] = id;
}

void SpriteSheet::DefineStrip(SpriteID first_id, int count,
                              const std::string& texture_path,
                              int start_x, int y, int frame_w, int frame_h,
                              int gap) {

    for (int i = 0; i < count; ++i) {
        Define(static_cast<SpriteID>(static_cast<int>(first_id) + i),
               texture_path,
               start_x + i * (frame_w + gap),
               y,
               frame_w,
               frame_h);
    }
}

const Sprite& SpriteSheet::Get(SpriteID id) const {
    int idx = static_cast<int>(id);
    assert(_defined[idx] && "SpriteID not defined");
    return _sprites[idx];
}

const Sprite& SpriteSheet::Get(const std::string& name) const {
    auto it = _name_to_id.find(name);
    assert(it != _name_to_id.end() && "Sprite not found");
    return Get(it->second);  // reuse assert in Get(SpriteID)
}

const Sprite& SpriteSheet::GetByIndex(SpriteID first, int i) const {
    return Get(static_cast<SpriteID>(static_cast<int>(first) + i));
}

bool SpriteSheet::Has(SpriteID id) const {
    int idx = static_cast<int>(id);
    return idx >= 0 && idx < static_cast<int>(SpriteID::Count) && _defined[idx];
}

bool SpriteSheet::Has(const std::string& name) const {
    return _name_to_id.count(name) > 0;
}
