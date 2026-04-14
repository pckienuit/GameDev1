#pragma once
#include "../ecs/EntityManager.h"
#include <vector>

struct CollisionEvent {
    EntityID entity_a;
    EntityID entity_b;
    float    normal_x, normal_y;  // collision direction
    float    hit_time;              // overlap depth
};
class CollisionEventPool {
public:
    explicit CollisionEventPool(int capacity);
    CollisionEvent* Acquire();
    void Reset();
    int  Count() const;
    const CollisionEvent& Get(int idx) const;
private:
    std::vector<CollisionEvent> _events;
    int _count;
};