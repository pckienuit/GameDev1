#pragma once
#include "Enemy.h"
#include "Player.h"
#include "../tilemap/Tilemap.h"
#include "../collision/CollisionSystem.h"
#include "../renderer/SpriteBatch.h"
#include <vector>

class EnemyManager {
public:
    EnemyManager(const Texture* goomba_texture, EntityManager& em);

    void Spawn(EnemyType type, float x, float y);

    void Update(float dt, const Tilemap& tilemap);

    void RegisterAll(CollisionSystem& collision_system);

    void HandleCollisions(const CollisionEventPool& pool, EntityID player_id, Player& player);

    void RenderAll(SpriteBatch& sprite_batch, float dt);

    // --- Accessors ---
    int ActiveCount() const;

    // Drain accumulated score since last call (call once per frame after HandleCollisions)
    int PopScore();

private:
    std::vector<Enemy> _enemies;   // pool cố định — không alloc trong gameplay

    const Texture* _goomba_texture;
    EntityManager& _em;
    int            _pending_score = 0;  // accumulates stomp points between PopScore() calls

    // --- Private helpers ---

    int FindFreeSlot() const;
    void MoveEnemy(Enemy& e, float dt, const Tilemap& tilemap);
    bool IsEdgeAhead(const Enemy& e, const Tilemap& tilemap) const;
};