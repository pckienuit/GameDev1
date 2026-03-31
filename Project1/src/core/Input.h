#pragma once
#include <windows.h>

// Actions thay vì raw keys — dễ rebind
enum class Action {
    MoveLeft,
    MoveRight,
    Jump,
    Count   // ← luôn để cuối, dùng để biết size
};

class Input {
public:
    // Gọi 1 lần mỗi fixed update step
    void Poll();

    // Câu hỏi: 3 method này trả về gì?
    bool IsPressed(Action action)  const; // chỉ true frame ĐẦU nhấn
    bool IsHeld(Action action)     const; // true khi đang giữ
    bool IsReleased(Action action) const; // chỉ true frame ĐẦU thả

private:
    // Hint: cần lưu trạng thái frame HIỆN TẠI và frame TRƯỚC
    // để phân biệt Pressed vs Held
    bool _current[static_cast<int>(Action::Count)] = {};
    bool _previous[static_cast<int>(Action::Count)] = {};

    // Câu hỏi: method này có nhiệm vụ gì?
    static bool _sample(Action action);
};