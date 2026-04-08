#include "EntityManager.h"

EntityManager::EntityManager() {
    _alive.reserve(1024);
}

EntityID EntityManager::Create() {
    EntityID id;
    if (_free_list.empty()) {
        id = _next_id++;
        _alive.push_back(true);
    } else {
        id = _free_list.front();
        _free_list.pop();
        _alive[id] = true;
    }
    return id;
}

void EntityManager::Destroy(EntityID id) {
    if (id < _alive.size() && _alive[id]) {
        _alive[id] = false;
        _free_list.push(id);
    }
}

bool EntityManager::IsAlive(EntityID id) const {
    return id < _alive.size() && _alive[id];
}