#include "Input.h"

static int action_to_vk(Action action) {
    switch (action) {
        case Action::MoveLeft:  return VK_LEFT;
        case Action::MoveRight: return VK_RIGHT;
        case Action::Jump:      return VK_SPACE;
        default:                return 0;
    }
}

bool Input::_sample(Action action) {
    //Read high bit from OS
    return (GetAsyncKeyState(action_to_vk(action)) & 0x8000) != 0;
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