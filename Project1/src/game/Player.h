#pragma once
#include "../tilemap/Tilemap.h"
#include "../renderer/Animation.h"
#include "../collision/AABB.h"
#include "../ecs/EntityManager.h"
#include <cmath>

class Player {
public:
    Player(float start_x, float start_y, const Texture* texture, EntityManager& entity_manager);

    void Update(float dt, bool move_left, bool move_right, 
                bool jump_pressed, const Tilemap& tilemap);

    void PrepareVelocity(float dt, bool move_left, bool move_right, bool jump_pressed);
    void Move(float dt, const Tilemap& tilemap);
    void ClampVelocityAlongNormal(float normal_x, float normal_y, float hit_time);

    float GetX()      const { return _pos_x; }
    float GetY()      const { return _pos_y; }
	float GetW()      const { return PLAYER_W ; }
	float GetH()      const { return PLAYER_H; }

    AABB GetAABB() const {
        return AABB{_pos_x, _pos_y, PLAYER_W, PLAYER_H};
    }

    EntityID GetID() const { return _id; }

    bool  IsGrounded() const { return _is_grounded; }

    const Sprite& GetSprite(float dt);
    bool          IsFacingLeft() const; 

    void ApplyPush(float dx, float dy) {
        _pos_x += dx;
        _pos_y += dy;
    }

    float GetVelX() const { return _vel_x; }
    float GetVelY() const { return _vel_y; }

private:
    EntityID _id;

    float _pos_x, _pos_y;
    float _vel_x, _vel_y;
    bool  _is_grounded = false;

    static constexpr float SPEED       = 200.0f;  // px/s
    static constexpr float JUMP_SPEED  = 800.0f;  // px/s
    static constexpr float GRAVITY     = 1200.0f;  // px/s²
    static constexpr float PLAYER_W    = 48.0f;
    static constexpr float PLAYER_H    = 96.0f;
    static constexpr float P_DISTANCE  = 1.0f;

    Animation _anim_idle;
    Animation _anim_walk;
    Animation _anim_jump;
    bool _facing_left = false;
};
