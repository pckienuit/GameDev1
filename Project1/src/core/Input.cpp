#include "Input.h"


Input::Input() {
    _bindings[Action::MoveLeft]  = { VK_LEFT,  'A' };
    _bindings[Action::MoveRight] = { VK_RIGHT, 'D' };
    _bindings[Action::Jump]      = { VK_SPACE, 'W', VK_UP };
}


bool Input::_sample(Action action) const {
    for (int vk : _bindings.at(action)) {
        if (GetAsyncKeyState(vk) & 0x8000) return true;
    }
    return false;
}

void Input::Poll() {
    constexpr int N = static_cast<int>(Action::Count);
    for (int i = 0; i < N; ++i) {
        _previous[i] = _current[i];
        _current[i]  = _sample(static_cast<Action>(i));
    }
}

bool Input::IsPressed(Action a) const {
    int i = static_cast<int>(a);
    return _current[i] && !_previous[i];
}

bool Input::IsHeld(Action a) const {
    int i = static_cast<int>(a);
    return _current[i];
}

bool Input::IsReleased(Action a) const {
    int i = static_cast<int>(a);
    return !_current[i] && _previous[i];
}