#include "Animation.h"

Animation::Animation(const Texture* texture):
    _texture(texture),
    _current_sprite(texture, 0, 0, 0, 0)
{
    
}

void Animation::AddFrame(int src_x, int src_y, int src_w, int src_h, float duration) {
    _frames.push_back({src_x, src_y, src_w, src_h, duration});
    if (_frames.size() == 1) {
        _current_sprite = Sprite(_texture, src_x, src_y, src_w, src_h);
    }
}

void Animation::SetLooping(bool loop) {
    _looping = loop;
}

const Sprite& Animation::Update(float dt) {
    if (_frames.empty()) {
        return _current_sprite;
    }

    _timer += dt;
    while (_timer >= _frames[_current_idx].duration) {
        _timer -= _frames[_current_idx].duration;
        if (_looping) {
            _current_idx = (_current_idx + 1) % _frames.size();
        } else {
            _current_idx = min(((int)_frames.size() - 1), (_current_idx + 1));
        }
    }
    
    _current_sprite = Sprite(_texture, _frames[_current_idx].src_x, _frames[_current_idx].src_y, _frames[_current_idx].src_w, _frames[_current_idx].src_h);
    return _current_sprite;
}

void Animation::Reset() {
    _current_idx = 0;
    _timer = 0.0f;
}   