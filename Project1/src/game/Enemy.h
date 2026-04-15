#pragma once
#include "../collision/AABB.h"
#include "../ecs/EntityManager.h"
#include "../renderer/Animation.h"
#include "../renderer/Texture.h"

enum class EnemyType  { Goomba };
enum class EnemyState { Patrol, Dead };

struct Enemy {
    EnemyType  type   = EnemyType::Goomba;
    EnemyState state  = EnemyState::Patrol;
    bool       active = false;

    float    pos_x = 0.0f;
    float    pos_y = 0.0f;
    float    vel_x = 0.0f;  // patrol speed — negative = left, positive = right
    float    vel_y = 0.0f;

    EntityID id = 0;

    float dead_timer = 0.0f;  // counts down after death before despawn

    Animation anim_walk;
    Animation anim_dead;

    // Kích thước Goomba (pixel)
    static constexpr float W = 48.0f;
    static constexpr float H = 48.0f;

    // TODO: thêm các hằng số cần thiết (tốc độ tuần tra, trọng lực, thời gian dead...)
    // Gợi ý: PATROL_SPEED, GRAVITY, DEAD_DURATION
    static constexpr float PATROL_SPEED = 60.0f;
    static constexpr float GRAVITY = 1200.0f;
    static constexpr float DEAD_DURATION = 0.5f;

    explicit Enemy(const Texture* texture, EntityManager& em);

    Enemy() : anim_walk(nullptr), anim_dead(nullptr) {}

    float GetH() const { return H; }
    float GetW() const { return W; }

    bool IsFacingLeft() const { return vel_x < 0.0f; }

    void ClampVelocityAlongNormal(float nx, float ny, float hit_time) {
        float dot = vel_x * nx + vel_y * ny;
        if (dot < 0.0f) {
            vel_x -= dot * nx;
            vel_y -= dot * ny;
        }
    }

    const Sprite& GetSprite(float dt);

    AABB GetAABB() const { return AABB{ pos_x, pos_y, W, H }; }
};
