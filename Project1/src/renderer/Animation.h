#pragma once
#include <vector>
#include <algorithm>
#include "Sprite.h"

class Animation {
public:
    explicit Animation(const Texture* texture);

    void AddFrame(int src_x, int src_y, int src_w, int src_h, float duration);
    void SetLooping(bool loop);

    const Sprite& Update(float dt);

    void Reset();

private:
    const Texture*         _texture;
    std::vector<AnimFrame> _frames;
    Sprite                 _current_sprite;
    int                    _current_idx = 0;
    float                  _timer       = 0.0f;
    bool                   _looping     = true;
};
