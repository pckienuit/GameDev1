#pragma once
#include <cstdint>

class GameLoop {
public:
    // FIXED_DT = 1.0 / 50.0 → tại sao 50Hz chứ không phải 60Hz?
    static constexpr double FIXED_DT = 1.0 / 50.0;

    GameLoop();   // Gọi QueryPerformanceFrequency ở đây

    // Gọi mỗi đầu frame — trả về delta time thực tế (giây)
    double Tick();

    // Trả về true khi cần chạy 1 physics step
    bool ShouldUpdate();

    // Sau khi render xong — gọi để "tiêu thụ" 1 fixed step
    void ConsumeUpdate();

    // Dùng để interpolate vị trí khi render
    // 0.0 = frame cũ, 1.0 = frame mới
    double GetAlpha() const;

private:
    int64_t _frequency;     // QueryPerformanceFrequency
    int64_t _last_time;     // QueryPerformanceCounter lần trước
    double  _accumulator;   // thời gian tích lũy chưa xử lý
};