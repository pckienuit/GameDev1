#pragma once
#include <cstdint>
#include <vector>
#include <queue>

using EntityID = uint32_t;
constexpr EntityID NULL_ENTITY = UINT32_MAX;

class EntityManager {
public:
    EntityManager();

    EntityID Create();

    void Destroy(EntityID id);

    bool IsAlive(EntityID id) const;

private:
    uint32_t         _next_id  = 0;
    std::queue<EntityID> _free_list;
    std::vector<bool> _alive;
};
