#pragma once
#include <vector>
#include <cstdint>
#include "EntityManager.h"

template<typename T>
class ComponentStore {
public:
    void Add(EntityID id, const T& component);

    void Remove(EntityID id);

    T& Get(EntityID id);

    bool Has(EntityID id) const;

private:
    static constexpr size_t INVALID = SIZE_MAX;

    std::vector<T>        _dense;           // packed data
    std::vector<EntityID> _dense_to_entity; // dense index → EntityID
    std::vector<size_t>   _sparse;          // EntityID → dense index
};

template<typename T>
void ComponentStore<T>::Add(EntityID id, const T& component) {
    if (Has(id)) {
        _dense[_sparse[id]] = component;
    } else {
        _dense.push_back(component);
        _dense_to_entity.push_back(id);

        if (id >= _sparse.size()) {
            _sparse.resize(id + 1, INVALID);
        }

        _sparse[id] = _dense.size() - 1;
    }
}


template<typename T>
void ComponentStore<T>::Remove(EntityID id) {
    if (!Has(id)) return;

    size_t idx = _sparse[id];
    size_t last_idx = _dense.size() - 1;

    // Swap-and-pop
    _dense[idx] = _dense[last_idx];
    _dense_to_entity[idx] = _dense_to_entity[last_idx];

    _sparse[_dense_to_entity[last_idx]] = idx;
    _sparse[id] = INVALID;

    _dense.pop_back();
    _dense_to_entity.pop_back();
}

template<typename T>
T& ComponentStore<T>::Get(EntityID id) {
    return _dense[_sparse[id]];
}

template<typename T>
bool ComponentStore<T>::Has(EntityID id) const {
    return id < _sparse.size() && _sparse[id] != INVALID;
}
