#pragma once
#include "../tilemap/Tilemap.h"

class Player {
public:
    Player(float start_x, float start_y);

    // TODO: nhận input từ ngoài vào (IsHeld, IsPressed)
    // Tại sao không cho Player tự gọi Input bên trong?
    void Update(float dt, bool move_left, bool move_right, 
                bool jump_pressed, const Tilemap& tilemap);

    float GetX()      const { return _pos_x; }
    float GetY()      const { return _pos_y; }
	float GetW()      const { return PLAYER_W ; }
	float GetH()      const { return PLAYER_H; }

    bool  IsGrounded() const { return _is_grounded; }

private:
    float _pos_x, _pos_y;
    float _vel_x, _vel_y;
    bool  _is_grounded = false;

    static constexpr float SPEED       = 200.0f;  // px/s
    static constexpr float JUMP_SPEED  = 800.0f;  // px/s
    static constexpr float GRAVITY     = 1200.0f;  // px/s²
    static constexpr float PLAYER_W    = 64.0f;
    static constexpr float PLAYER_H    = 64.0f;
    static constexpr float P_DISTANCE  = 1.0f;
};
