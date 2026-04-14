#pragma once
#include <algorithm>

struct AABB {
    float x, y;   // top-left corner (pixels)
    float w, h;   // width, height
    
    float Right()  const { return x + w; }
    float Bottom() const { return y + h; }
    
    static bool Overlaps(const AABB& a, const AABB& b);
    
    struct HitInfo {
        float normal_x, normal_y;  
        float depth;               
    };
    
    static bool GetHitInfo(const AABB& a, const AABB& b, HitInfo& out);
    
    struct SweptResult {
        float hit_time;    // [0,1]
        float normal_x;
        float normal_y;
    };
    
    static SweptResult Swept(const AABB& a, float vel_x, float vel_y, const AABB& b);
};



