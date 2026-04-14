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

void CollisionSystem::Register(EntityID id, const AABB& box, float vel_x, float vel_y) {
    _grid.Insert(id, box.x, box.y, box.w, box.h);
    _entities[id] = {id, box, vel_x, vel_y};
}

void CollisionSystem::Detect() {
    std::vector<EntityID> candidates;
    for (auto const& [id, entry] : _entities) {
        candidates.clear();
        _grid.Query(entry.box.x, entry.box.y, entry.box.w, entry.box.h, candidates);
        for (EntityID other_id : candidates) {
            if (other_id <= id) continue;

            const auto& other = _entities[other_id];

            AABB::SweptResult result = AABB::Swept(entry.box, entry.vel_x, entry.vel_y, other.box);
            if (result.hit_time < 1.0f) {
                CollisionEvent* ev = _pool.Acquire();
                if (ev) {
                    ev->entity_a = id;
                    ev->entity_b = other_id;
                    ev->normal_x = result.normal_x;
                    ev->normal_y = result.normal_y;
                    ev->hit_time = result.hit_time;
                }
            }
        }
    }
}
