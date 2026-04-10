#include "AABB.h"

bool AABB::Overlaps(const AABB& a, const AABB& b) {
    if (a.Right()  <= b.x || 
        b.Right()  <= a.x ||
        a.Bottom() <= b.y || 
        b.Bottom() <= a.y) { return false; }
    return true;
}

bool AABB::GetHitInfo(const AABB& a, const AABB& b, HitInfo& out) {
    if (!Overlaps(a, b)) { return false; }

    // Minimum penetration depth on each axis
    float overlap_x = std::min(a.Right() - b.x, b.Right() - a.x);
    float overlap_y = std::min(a.Bottom() - b.y, b.Bottom() - a.y);

    if (overlap_x < overlap_y) {
        // Push along X axis
        out.normal_x = (a.x < b.x) ? -1.0f : 1.0f;
        out.normal_y = 0.0f;
        out.depth    = overlap_x;
    } else {
        // Push along Y axis
        out.normal_x = 0.0f;
        out.normal_y = (a.y < b.y) ? -1.0f : 1.0f;
        out.depth    = overlap_y;
    }

    return true;
}
