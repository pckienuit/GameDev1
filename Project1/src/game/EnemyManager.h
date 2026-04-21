#pragma once
#include "Enemy.h"
#include "Player.h"
#include "../tilemap/Tilemap.h"
#include "../collision/CollisionSystem.h"
#include "../renderer/SpriteBatch.h"
#include "../renderer/SpriteSheet.h"
#include <vector>

class EnemyManager {
public:
    explicit EnemyManager(EntityManager& em);

    // Caller passes the central SpriteSheet to build enemy animations
    void Spawn(const EnemyDef& def, const SpriteSheet& sheet, float x, float y);

    void Update(float dt, const Tilemap& tilemap);
    void RegisterAll(CollisionSystem& collision_system);
    void HandleCollisions(const CollisionEventPool& pool, EntityID player_id, Player& player);
    void RenderAll(SpriteBatch& sprite_batch, float dt);

    int ActiveCount() const;
    int PopScore();    // drain accumulated score since last call
    void ClearAll();   // deactivate all enemies (call on level load)

private:
    std::vector<Enemy> _enemies;
    EntityManager&     _em;
    int                _pending_score = 0;

    int  FindFreeSlot() const;
    void MoveEnemy(Enemy& e, float dt, const Tilemap& tilemap);
    bool IsEdgeAhead(const Enemy& e, const Tilemap& tilemap) const;
};