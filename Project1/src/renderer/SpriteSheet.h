#pragma once
#include "Sprite.h"
#include "Texture.h"
#include <unordered_map>
#include <string>

class SpriteSheet {
public:
    explicit SpriteSheet(const Texture* texture);

    void Define(const std::string& name, int x, int y, int w, int h);
    const Sprite& Get(const std::string& name) const;
    const Sprite& GetChar(char c) const { return Get(std::string(1, c)); }
    bool Has(const std::string& name) const { return _regions.count(name);}

private:
    const Texture* _texture;
    std::unordered_map<std::string, Sprite> _regions;
};
