#pragma once
#include "SpatialGrid.h"
#include "CollisionEvent.h"
#include "AABB.h"
#include <unordered_map>

// Đơn giản nhất: mỗi entity có thể tham gia collision
// cần cung cấp thông tin này
struct CollidableEntry {
    EntityID id;
    AABB     box;
    float    vel_x;
    float    vel_y;
};

class CollisionSystem {
public:
    CollisionSystem(int world_w, int world_h, int cell_size, int max_events);

    // Gọi đầu frame: clear grid + pool
    void BeginFrame();

    // Register entity vào hệ thống frame này
    void Register(EntityID id, const AABB& box, float vel_x, float vel_y);

    // Chạy broadphase + narrowphase → fill event pool
    void Detect();

    // Đọc kết quả
    const CollisionEventPool& GetEvents() const { return _pool; }

private:
    SpatialGrid        _grid;
    CollisionEventPool _pool;

    std::unordered_map<EntityID, CollidableEntry> _entities;
};
