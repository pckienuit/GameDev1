#pragma once
#include <vector>
#include "SpriteSheet.h"
#include "SpriteBatch.h"

struct BgLayer {
    SpriteID sprite;
    float    parallax_factor;  // 0.0 = static (no movement with camera), 1.0 = moves 1:1 with camera 
    float    y_offset;         // position from top of screen
    float    tile_width;       // width to repeat the texture
    float    tile_height;      // height of the texture
};

class Background {
public:
    void Clear();
    void AddLayer(SpriteID sprite, float parallax, float y_offset, float tile_w, float tile_h);
    
    // Render layers. Parallax is calculated using cam_x.
    // Repeats the texture horizontally to fill the screen depending on camera position.
    void Render(SpriteBatch& batch, const SpriteSheet& sheet,
                float cam_x, float cam_y, float screen_w, float screen_h);

private:
    std::vector<BgLayer> _layers;
};
