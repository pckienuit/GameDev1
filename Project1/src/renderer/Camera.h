#pragma once

class Camera {
public:
    Camera(float viewport_w, float viewport_h);

    void Follow(float target_x, float target_y, float dt);

    void Clamp(float world_w, float world_h);

    float GetX() const { return _x; }
    float GetY() const { return _y; }

private:
    float _x = 0.0f;
    float _y = 0.0f;
    float _viewport_w;
    float _viewport_h;
    
    // Dead zone: vùng ở giữa màn hình player được di chuyển tự do
    // Ví dụ: dead_zone_w = viewport_w * 0.3f
    static constexpr float VIEW_RATIO = 0.3f;
    static constexpr float LERP_SPEED   = 5.0f;  // bao nhiêu là mượt?
};
