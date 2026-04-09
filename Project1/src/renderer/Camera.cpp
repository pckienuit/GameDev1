#include "Camera.h"
#include <algorithm>  // std::clamp
#include <cmath>      // expf

Camera::Camera(float vw, float vh)
    : _viewport_w(vw), _viewport_h(vh) {}

void Camera::Follow(float target_x, float target_y, float dt) {

    float player_screen_x = target_x - _x;

    float dz_left  = _viewport_w * VIEW_RATIO;
    float dz_right = _viewport_w * (1 - VIEW_RATIO);

    float target_cam_x = _x;

    if (player_screen_x < dz_left) {
        target_cam_x = _x - (dz_left-player_screen_x);
    }
    else if (player_screen_x > dz_right) {
        target_cam_x = _x + (player_screen_x-dz_right);
    }

    // Smooth follow use exponential decay
    _x += (target_cam_x - _x) * (1.0f - expf(-LERP_SPEED * dt));

    _y = 0.0f;
}

void Camera::Clamp(float world_w, float world_h) {
    _x = std::clamp(_x, 0.0f, world_w - _viewport_w);
    _y = std::clamp(_y, 0.0f, world_h - _viewport_h);
}
