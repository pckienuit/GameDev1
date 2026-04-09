#pragma once
#include <windows.h>
#include <vector>
#include <map>

enum class Action {
    MoveLeft,
    MoveRight,
    Jump,
    Count
};

class Input {
public:
    Input();
    void Poll();

    bool IsPressed(Action action)  const;
    bool IsHeld(Action action)     const;
    bool IsReleased(Action action) const;

private:
    bool _current[static_cast<int>(Action::Count)] = {};
    bool _previous[static_cast<int>(Action::Count)] = {};

    std::map<Action, std::vector<int>> _bindings;

    bool _sample(Action action) const;
};