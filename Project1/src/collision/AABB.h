#pragma once
#include <algorithm>

struct AABB {
    float x, y;   // top-left corner (pixels)
    float w, h;   // width, height

    float Right()  const { return x + w; }
    float Bottom() const { return y + h; }

    // Fast boolean check — dùng để reject sớm
    static bool Overlaps(const AABB& a, const AABB& b);

    struct HitInfo {
        float normal_x, normal_y;  // unit normal (collision direction from A)
        float depth;               // penetration depth (pixels)
    };

    // Full collision info — chỉ gọi khi Overlaps() == true
    // Returns false nếu không overlap
    static bool GetHitInfo(const AABB& a, const AABB& b, HitInfo& out);
};
