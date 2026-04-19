#pragma once
#include "../tilemap/Tilemap.h"
#include "../renderer/Animation.h"
#include "../collision/AABB.h"
#include "../ecs/EntityManager.h"
#include <cmath>
#include <algorithm>

class Player {
public:
    Player(float start_x, float start_y, const Texture* texture, EntityManager& entity_manager);

    void Update(float dt, bool move_left, bool move_right, 
                bool jump_pressed, const Tilemap& tilemap);

    void PrepareVelocity(float dt, bool move_left, bool move_right, bool jump_pressed, bool jump_held);
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

    void Hurt();
    void Bounce(float bounce_height) { _vel_y = bounce_height; }

    void  TickInvincibility(float dt);
    bool  IsInvincible() const { return _inv_timer > 0.0f; }
    bool  IsGameOver()   const { return _game_over; }
    int   GetLives()     const { return _lives; }
    bool  ShouldRender() const;          // blink

private:
    EntityID _id;

    float _pos_x, _pos_y;
    float _vel_x, _vel_y;
    bool  _is_grounded = false;
    bool  _is_dead     = false;
    int   _lives       = STARTING_LIVES;
    float _inv_timer   = 0.0f;
    bool  _game_over   = false;
    float _coyote_timer = 0.0f;
    float _jump_buffer_timer = 0.0f;

    static constexpr float SPEED       = 200.0f;  // px/s
    static constexpr float JUMP_SPEED  = 800.0f;  // px/s
    static constexpr float GRAVITY     = 1200.0f;  // px/s^2
    static constexpr float PLAYER_W    = 48.0f;
    static constexpr float PLAYER_H    = 96.0f;
    static constexpr float P_DISTANCE  = 1.0f;
    static constexpr int   STARTING_LIVES = 9;
    static constexpr float INVINCIBILITY_TIME = 2.0f; //seconds
    static constexpr float COYOTE_TIME        = 0.1f;
    static constexpr float JUMP_BUFFER_TIME   = 0.1f;
    static constexpr float JUMP_CUT_FACTOR    = 1.6f;
    static constexpr float MAX_SPEED          = 200.0f; //px/s
    static constexpr float ACCELERATION       = 800.0f; //px/s^2
    static constexpr float DECELERATION       = 600.0f; //px/s^2
    static constexpr float SKID_DECEL         = 1200.0f; //px/s^2
    static constexpr float AIR_CTRL_FACTOR    = 0.6f;

    Animation _anim_idle;
    Animation _anim_walk;
    Animation _anim_jump;
    bool _facing_left = false;
};
