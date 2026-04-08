#pragma once
#include "Texture.h"

struct Sprite {
    const Texture* texture = nullptr;

    //Region in atlas
    int src_x = 0;
    int src_y = 0;
    int src_w = 0;
    int src_h = 0;

    Sprite(const Texture* tex, int x, int y, int w, int h);
};
