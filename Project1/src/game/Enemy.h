#pragma once
#include "../collision/AABB.h"
#include "../ecs/EntityManager.h"
#include "../renderer/Animation.h"
#include "../renderer/Texture.h"

enum class EnemyState { Patrol, Shell, Sliding, Dead };

struct AnimFrameDef {
    int   x, y, w, h;
    float duration;
};

struct EnemyDef {
    float w                 = 48.0f;  // render width in pixels
    float h                 = 48.0f;  // render height in pixels
    float patrol_speed      = 60.0f;
    float dead_duration     = 0.5f;
    float gravity           = 1200.0f;
    bool  turns_at_edges    = true;   // patrol reverses at ledges
    bool  has_shell         = false;  // true = Koopa-like stomp behavior
    float shell_wait_time   = 0.0f;
    float shell_slide_speed = 0.0f;
    int   max_slide_bounces = 3;   // wall bounces before shell stops sliding

    // Walk animation (up to 4 frames)
    int          walk_frame_count = 0;
    AnimFrameDef walk_frames[4]   = {};

    // Dead/squish animation (up to 2 frames)
    int          dead_frame_count = 0;
    AnimFrameDef dead_frames[2]   = {};

    // Shell idle animation (only relevant when has_shell = true, up to 2 frames)
    int          shell_frame_count = 0;
    AnimFrameDef shell_frames[2]   = {};

    // Named presets — add new enemy types here only
    static const EnemyDef GOOMBA;
    static const EnemyDef KOOPA;
};

struct Enemy {
    const EnemyDef* def    = nullptr;
    EnemyState      state  = EnemyState::Patrol;
    bool            active = false;

    float pos_x = 0.0f;
    float pos_y = 0.0f;
    float vel_x = 0.0f;
    float vel_y = 0.0f;

    EntityID id          = 0;
    float    dead_timer        = 0.0f;
    float    shell_timer       = 0.0f;
    int      slide_bounce_count = 0;

    bool     facing_left = false;

    Animation anim_walk;
    Animation anim_dead;
    Animation anim_shell;

    explicit Enemy(const Texture* texture, const EnemyDef& def, EntityManager& em);
    Enemy() : anim_walk(nullptr), anim_dead(nullptr), anim_shell(nullptr) {}

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
