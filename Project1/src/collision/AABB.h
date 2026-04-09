#pragma once
#include <algorithm>

struct AABB {
    float x, y;   // top-left corner (pixels)
    float w, h;   // width, height

    float Right()  const { return x + w; }
    float Bottom() const { return y + h; }

    static bool Overlaps(const AABB& a, const AABB& b);

    static bool Resolve(const AABB& a, const AABB& b,
                        float& out_dx, float& out_dy);
};
