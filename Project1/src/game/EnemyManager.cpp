#include "EnemyManager.h"

EnemyManager::EnemyManager(const Texture* goomba_texture, EntityManager& em)
    : _goomba_texture(goomba_texture),
      _em(em)
{
    _enemies.resize(128);
}

int EnemyManager::FindFreeSlot() const {
    for (int i = 0; i < _enemies.size(); ++i) {
        if (! _enemies[i].active) return i;
    }
    return -1;
}   

void EnemyManager::MoveEnemy(Enemy& e, float dt, const Tilemap& tilemap) {
    constexpr float P = 1.0f;  // edge margin
    // === X-AXIS ===
    e.pos_x += e.vel_x * dt;
    if (e.vel_x > 0.0f) {
        int col = tilemap.PixelToCol(e.pos_x + Enemy::W);
        int row_t = tilemap.PixelToRow(e.pos_y + P);
        int row_b = tilemap.PixelToRow(e.pos_y + Enemy::H - P);
        if (tilemap.IsSolid(col, row_t) || tilemap.IsSolid(col, row_b)) {
            e.pos_x = col * tilemap.GetTileSize() - Enemy::W;
            e.vel_x = -e.vel_x;  // đảo hướng khi chạm tường
        }
    } else if (e.vel_x < 0.0f) {
        int col_left = tilemap.PixelToCol(e.pos_x);
        int row_top   = tilemap.PixelToRow(e.pos_y + P);
        int row_bot   = tilemap.PixelToRow(e.pos_y + Enemy::H - P);

        if (tilemap.IsSolid(col_left, row_top) || 
            tilemap.IsSolid(col_left, row_bot)) {
            e.pos_x = (col_left + 1) * tilemap.GetTileSize();
            e.vel_x = -e.vel_x;
        }
    }
    // === Y-AXIS ===
    e.pos_y += e.vel_y * dt;
    if (e.vel_y > 0.0f) {
        // Check falling
        int tile_row = tilemap.PixelToRow(e.pos_y + Enemy::H);
        int col_left = tilemap.PixelToCol(e.pos_x + P);
        int col_mid  = tilemap.PixelToCol(e.pos_x + Enemy::W*0.5f);
        int col_right = tilemap.PixelToCol(e.pos_x + Enemy::W - P);

        if (tilemap.IsSolid(col_left, tile_row) || 
            tilemap.IsSolid(col_right, tile_row)|| 
            tilemap.IsSolid(col_mid, tile_row)) {
            e.pos_y = tile_row * tilemap.GetTileSize() - Enemy::H;
            e.vel_y = 0;
        }
    }
}

bool EnemyManager::IsEdgeAhead(const Enemy& e, const Tilemap& tilemap) const {
    int cell_x = static_cast<int>(e.pos_x / tilemap.GetTileSize());
    int cell_y = static_cast<int>(e.pos_y / tilemap.GetTileSize());
    int next_cell_x = cell_x + (e.vel_x > 0 ? 1 : -1);
    int next_cell_y = cell_y + 1;
    return tilemap.GetTile(next_cell_x, next_cell_y).type == TileType::Empty;
}   

void EnemyManager::Spawn(EnemyType type, float x, float y) {
    int idx = FindFreeSlot();
    if (idx == -1) return;

    // Reconstruct slot với texture đúng — setup animations + tạo EntityID
    _enemies[idx] = Enemy(_goomba_texture, _em);

    Enemy& e = _enemies[idx];
    e.active = true;
    e.type   = type;
    e.state  = EnemyState::Patrol;
    e.pos_x  = x;
    e.pos_y  = y;
    e.vel_x  = Enemy::PATROL_SPEED;
    e.vel_y  = 0.0f;
    e.dead_timer = 0.0f;
}

void EnemyManager::Update(float dt, const Tilemap& tilemap) {
    for (auto& e : _enemies) {
        if (!e.active) continue;
        
        // Apply gravity
        e.vel_y += Enemy::GRAVITY * dt;
        
        // Move
        MoveEnemy(e, dt, tilemap);
        
        // Check edge
        if (IsEdgeAhead(e, tilemap)) {
            e.vel_x = -e.vel_x;
        }
        
        // Update dead timer
        if (e.state == EnemyState::Dead) {
            e.dead_timer -= dt;
            if (e.dead_timer <= 0.0f) {
                e.active = false;
                _em.Destroy(e.id);
            }
        }
    }
}

void EnemyManager::RegisterAll(CollisionSystem& collision_system) {
    for (auto& e : _enemies) {
        if (e.active) {
            collision_system.Register(e.id, e.GetAABB(), e.vel_x, e.vel_y);
        }
    }
}

void EnemyManager::HandleCollisions(const CollisionEventPool& pool, EntityID player_id) {
    for (auto& e : _enemies) {
        if (!e.active) continue;
        
        for (int i = 0; i < pool.Count(); ++i) {
            const CollisionEvent& ev = pool.Get(i);
            
            if (ev.entity_a == e.id) {
                e.ClampVelocityAlongNormal(ev.normal_x, ev.normal_y, ev.hit_time);
            }
            if (ev.entity_b == e.id) {
                e.ClampVelocityAlongNormal(-ev.normal_x, -ev.normal_y, ev.hit_time);
            }
        }
    }
}

void EnemyManager::RenderAll(SpriteBatch& sprite_batch, float dt) {
    for (auto& e : _enemies) {
        if (e.active) {
            sprite_batch.Draw(e.pos_x, e.pos_y, e.GetW(), e.GetH(), e.GetSprite(dt), 1.0f, 1.0f, 1.0f, 1.0f, e.IsFacingLeft());
        }
    }
}

int EnemyManager::ActiveCount() const {
    int count = 0;
    for (auto& e : _enemies) {
        if (e.active) count++;
    }
    return count;
}