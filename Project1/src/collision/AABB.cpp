#include "AABB.h"
#include <limits>

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

AABB::SweptResult AABB::Swept(const AABB& a, float vel_x, float vel_y, const AABB& b) {
    SweptResult result{ 1.0f, 0.0f, 0.0f };  // default: no hit

    float entry_x, exit_x;
    float entry_y, exit_y;

    if (vel_x > 0.0f) {
        entry_x = b.x       - a.Right();   
        exit_x  = b.Right() - a.x;
    } else if (vel_x < 0.0f) {
        entry_x = b.Right() - a.x;         
        exit_x  = b.x       - a.Right();
    } else {
        entry_x = -std::numeric_limits<float>::infinity();
        exit_x  =  std::numeric_limits<float>::infinity();
    }

    if (vel_y > 0.0f) {
        entry_y = b.y        - a.Bottom();
        exit_y  = b.Bottom() - a.y;
    } else if (vel_y < 0.0f) {
        entry_y = b.Bottom() - a.y;
        exit_y  = b.y        - a.Bottom();
    } else {
        entry_y = -std::numeric_limits<float>::infinity();
        exit_y  =  std::numeric_limits<float>::infinity();
    }

    //calculate time by distance / velocity
    float t_entry_x, t_exit_x;
    float t_entry_y, t_exit_y;

    if (vel_x != 0.0f) {
        t_entry_x = entry_x / vel_x;
        t_exit_x  = exit_x  / vel_x;
    } else {
        t_entry_x = -std::numeric_limits<float>::infinity();
        t_exit_x  =  std::numeric_limits<float>::infinity();
    }

    if (vel_y != 0.0f) {
        t_entry_y = entry_y / vel_y;
        t_exit_y  = exit_y  / vel_y;
    } else {
        t_entry_y = -std::numeric_limits<float>::infinity();
        t_exit_y  =  std::numeric_limits<float>::infinity();
    }

    float t_entry = std::max(t_entry_x, t_entry_y);
    float t_exit  = std::min(t_exit_x,  t_exit_y);

    if (t_entry > t_exit)   return result;
    if (t_entry < 0.0f && t_exit < 0.0f) return result;
    if (t_entry > 1.0f)     return result;

    result.hit_time = std::max(t_entry, 0.0f);

    if (t_entry_x > t_entry_y) {
        result.normal_x = (vel_x > 0.0f) ? -1.0f : 1.0f;
        result.normal_y = 0.0f;
    } else {
        result.normal_x = 0.0f;
        result.normal_y = (vel_y > 0.0f) ? -1.0f : 1.0f;
    }

    return result;
}

