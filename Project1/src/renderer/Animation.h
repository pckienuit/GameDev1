#pragma once
#include <vector>
#include <algorithm>
#include "SpriteSheet.h"

class Animation {
public:
    // For static frame lists: Animation(sheet, {ID::Walk0, ID::Walk1}, 0.15f)
    Animation(const SpriteSheet& sheet, std::initializer_list<SpriteID> frame_ids,
              float frame_duration, bool looping = true);

    // For dynamic arrays: EnemyDef::walk_frames[]
    Animation(const SpriteSheet& sheet, const SpriteID* frame_ids, int count,
              float frame_duration, bool looping = true);

    Animation() = default;

    const Sprite& Update(float dt);
    void Reset();

private:
    const SpriteSheet*    _sheet          = nullptr;
    std::vector<SpriteID> _frame_ids;
    float                 _frame_duration = 0.1f;  // shared across all frames
    Sprite                _current_sprite;
    int                   _current_idx    = 0;
    float                 _timer          = 0.0f;
    bool                  _looping        = true;
};
