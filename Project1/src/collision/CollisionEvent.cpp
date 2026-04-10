#include "CollisionEvent.h"

CollisionEventPool::CollisionEventPool(int capacity): 
    _events(capacity), 
    _count(0) 
{}

CollisionEvent* CollisionEventPool::Acquire() {
    if (_count < _events.size()) {
        return &_events[_count++];
    }
    return nullptr;
}

void CollisionEventPool::Reset() {
    _count = 0;
}

int CollisionEventPool::Count() const {
    return _count;
}

const CollisionEvent& CollisionEventPool::Get(int idx) const {
    return _events[idx];
}