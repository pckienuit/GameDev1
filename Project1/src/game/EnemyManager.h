#pragma once
#include "Enemy.h"
#include "../tilemap/Tilemap.h"
#include "../collision/CollisionSystem.h"
#include "../renderer/SpriteBatch.h"
#include <vector>

// Quản lý toàn bộ enemy trong level:
//   - Spawn / despawn
//   - Update AI (patrol, gravity, tường/vực)
//   - Register vào CollisionSystem mỗi frame
//   - Render qua SpriteBatch
class EnemyManager {
public:
    EnemyManager(const Texture* goomba_texture, EntityManager& em);

    // Spawn 1 Goomba tại (x, y) — tìm slot trống trong _enemies
    void Spawn(EnemyType type, float x, float y);

    // Gọi mỗi physics step:
    //   1. Áp gravity, di chuyển
    //   2. Check va chạm tường/vực → đổi hướng
    //   3. Check state DEAD → đếm dead_timer → deactivate
    void Update(float dt, const Tilemap& tilemap);

    // Register tất cả active enemy vào collision system
    void RegisterAll(CollisionSystem& collision_system);

    // Sau khi CollisionSystem::Detect() chạy xong,
    // xử lý các event liên quan đến enemy
    void HandleCollisions(const CollisionEventPool& pool, EntityID player_id);

    // Render tất cả active enemy
    void RenderAll(SpriteBatch& sprite_batch, float dt);

    // --- Accessors ---
    int ActiveCount() const;

private:
    std::vector<Enemy> _enemies;   // pool cố định — không alloc trong gameplay

    const Texture* _goomba_texture;

    // TODO: thêm reference (hoặc pointer) tới EntityManager nếu cần tạo EntityID khi spawn
    EntityManager& _em;

    // --- Private helpers ---

    // Tìm slot trống (active == false) để tái sử dụng
    // Trả về index, hoặc -1 nếu pool đầy
    int FindFreeSlot() const;

    // Cập nhật movement + tilemap collision cho 1 enemy
    // TODO: implement — tham khảo Player::Move() để hiểu pattern
    void MoveEnemy(Enemy& e, float dt, const Tilemap& tilemap);

    // Kiểm tra phía trước enemy có vực (edge) không
    // → nếu true thì đổi hướng
    // TODO: implement — gợi ý: check tile dưới chân phía trước
    bool IsEdgeAhead(const Enemy& e, const Tilemap& tilemap) const;
};