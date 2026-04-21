#include "Animation.h"

// Shared init — called by both constructors
static void InitAnimation(const SpriteSheet& sheet,
                          const SpriteID* ids, int count,
                          float duration, bool looping,
                          const SpriteSheet*& out_sheet,
                          std::vector<SpriteID>& out_ids,
                          float& out_duration,
                          bool& out_looping)
{
    out_sheet    = &sheet;
    out_ids.assign(ids, ids + count);
    out_duration = duration;
    out_looping  = looping;
    // NOTE: do NOT call sheet.Get() here — sprites may not be Define()d yet.
    //       Player/Enemy are constructed before Game's constructor body runs.
    //       Update() sets _current_sprite lazily on every call.
}

Animation::Animation(const SpriteSheet& sheet,
                     std::initializer_list<SpriteID> frame_ids,
                     float frame_duration, bool looping)
{
    InitAnimation(sheet,
                  frame_ids.begin(), static_cast<int>(frame_ids.size()),
                  frame_duration, looping,
                  _sheet, _frame_ids, _frame_duration, _looping);
}

Animation::Animation(const SpriteSheet& sheet,
                     const SpriteID* frame_ids, int count,
                     float frame_duration, bool looping)
{
    InitAnimation(sheet, frame_ids, count,
                  frame_duration, looping,
                  _sheet, _frame_ids, _frame_duration, _looping);
}

const Sprite& Animation::Update(float dt) {
    if (!_sheet || _frame_ids.empty()) return _current_sprite;

    _timer += dt;
    while (_timer >= _frame_duration) {
        _timer -= _frame_duration;
        if (_looping) {
            _current_idx = (_current_idx + 1) % static_cast<int>(_frame_ids.size());
        } else {
            _current_idx = min(_current_idx + 1,
                                    static_cast<int>(_frame_ids.size()) - 1);
        }
    }

    _current_sprite = _sheet->Get(_frame_ids[_current_idx]);
    return _current_sprite;
}

void Animation::Reset() {
    _current_idx = 0;
    _timer       = 0.0f;
    if (_sheet && !_frame_ids.empty()) {
        _current_sprite = _sheet->Get(_frame_ids[0]);
    }
}