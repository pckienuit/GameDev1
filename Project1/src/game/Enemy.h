#pragma once
#include "../collision/AABB.h"
#include "../ecs/EntityManager.h"
#include "../renderer/Animation.h"
#include "../renderer/SpriteID.h"

enum class EnemyState { Patrol, Shell, Sliding, Dead, Rising };

struct EnemyDef {
    float w                 = 48.0f;
    float h                 = 48.0f;
    float patrol_speed      = 60.0f;
    float dead_duration     = 0.5f;
    float gravity           = 1200.0f;
    bool  turns_at_edges    = true;
    bool  has_shell         = false;
    float shell_wait_time   = 0.0f;
    float shell_slide_speed = 0.0f;
    int   max_slide_bounces = 3;

    // Movement modifications
    bool  unstomp_able      = false;
    bool  is_flyer          = false; // If true, treats as FlyKoopa (oscillates and turns to Koopa when stomped)
    float oscillation_amp   = 0.0f;
    float oscillation_speed = 0.0f;

    // Walk animation
    int      walk_frame_count        = 0;
    SpriteID walk_frames[4]          = {};
    float    walk_frame_duration     = 0.5f;

    // Dead/squish animation
    int      dead_frame_count        = 0;
    SpriteID dead_frames[2]          = {};
    float    dead_frame_duration     = 0.5f;

    // Shell idle animation
    int      shell_frame_count       = 0;
    SpriteID shell_frames[2]         = {};
    float    shell_frame_duration    = 1.0f;

    static const EnemyDef GOOMBA;
    static const EnemyDef KOOPA;
    static const EnemyDef PIRANHA;
    static const EnemyDef FLY_KOOPA;
};

struct Enemy {
    const EnemyDef* def    = nullptr;
    EnemyState      state  = EnemyState::Patrol;
    bool            active = false;

    float pos_x = 0.0f;
    float pos_y = 0.0f;
    float vel_x = 0.0f;
    float vel_y = 0.0f;
    
    float origin_y = 0.0f;
    float oscillation_timer = 0.0f;

    EntityID id               = 0;
    float    dead_timer       = 0.0f;
    float    shell_timer      = 0.0f;
    int      slide_bounce_count = 0;
    bool     facing_left      = false;

    Animation anim_walk;
    Animation anim_dead;
    Animation anim_shell;

    explicit Enemy(const SpriteSheet& sheet, const EnemyDef& def, EntityManager& em);
    Enemy() = default;

    float GetH() const { return def ? def->h : 48.0f; }
    float GetW() const { return def ? def->w : 48.0f; }
    bool  IsFacingLeft() const { return facing_left; }

    void ClampVelocityAlongNormal(float nx, float ny, float /*hit_time*/) {
        float dot = vel_x * nx + vel_y * ny;
        if (dot < 0.0f) { vel_x -= dot * nx; vel_y -= dot * ny; }
    }

    const Sprite& GetSprite(float dt);
    AABB GetAABB() const { return AABB{ pos_x, pos_y, GetW(), GetH() }; }
};
