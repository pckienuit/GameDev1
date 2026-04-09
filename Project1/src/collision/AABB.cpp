#include "AABB.h"

bool AABB::Overlaps(const AABB& a, const AABB& b) {
    if (a.Right()  <= b.x || 
        b.Right()  <= a.x ||
        a.Bottom() <= b.y || 
        b.Bottom() <= a.y) { return false; }
    return true;
}

bool AABB::Resolve(const AABB& a, const AABB& b, float& out_dx, float& out_dy) {
    if (!Overlaps(a, b)) { return false; }
    
    float overlap_x = min(a.Right()-b.x, b.Right()-a.x);
    float overlap_y = min(a.Bottom()-b.y, b.Bottom()-a.y);

    if (overlap_x < overlap_y) {
        out_dx = (a.x < b.x) ? -overlap_x : overlap_x;
        out_dy = 0.0f;
    } else {
        out_dy = (a.y < b.y) ? -overlap_y : overlap_y;
        out_dx = 0.0f;
    }

    return true;
}