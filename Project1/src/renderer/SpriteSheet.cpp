#include "SpriteSheet.h"
#include <cassert>

SpriteSheet::SpriteSheet(const Texture* texture) : _texture(texture) {}

void SpriteSheet::Define(const std::string& name, int x, int y, int w, int h) {
    _regions[name] = Sprite(_texture, x, y, w, h);
}

const Sprite& SpriteSheet::Get(const std::string& name) const {
    assert(_regions.count(name) && "Sprite not found");
    return _regions.at(name);
}