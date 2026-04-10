#include "CollisionSystem.h"


CollisionSystem::CollisionSystem(int world_w, int world_h, int cell_size, int max_events):
    _grid(world_w, world_h, cell_size),
    _pool(max_events)
{
    
}

void CollisionSystem::BeginFrame() {
    _grid.Clear();
    _pool.Reset();
    _entities.clear();
}

void CollisionSystem::Register(EntityID id, const AABB& box) {
    _grid.Insert(id, box.x, box.y, box.w, box.h);
    _entities[id] = box;
}

void CollisionSystem::Detect() {
    std::vector<EntityID> candidates;    // reuse buffer — no alloc per entity

    for (auto const& [id, box] : _entities) {
        candidates.clear();
        _grid.Query(box.x, box.y, box.w, box.h, candidates);

        for (EntityID other_id : candidates) {
            if (other_id <= id) continue;  // avoid duplicate pairs (A,B) and (B,A)

            const AABB& other_box = _entities[other_id];

            AABB::HitInfo hit;
            if (AABB::GetHitInfo(box, other_box, hit)) {
                CollisionEvent* ev = _pool.Acquire();
                if (ev) {
                    ev->entity_a = id;
                    ev->entity_b = other_id;
                    ev->normal_x = hit.normal_x;
                    ev->normal_y = hit.normal_y;
                    ev->depth    = hit.depth;
                }
            }
        }
    }
}
