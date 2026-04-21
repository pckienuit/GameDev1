#include "Background.h"
#include <cmath>

void Background::Clear() {
    _layers.clear();
}

void Background::AddLayer(SpriteID sprite, float parallax, float y_offset, float tile_w, float tile_h) {
    _layers.push_back({sprite, parallax, y_offset, tile_w, tile_h});
}

void Background::Render(SpriteBatch& batch, const SpriteSheet& sheet,
                        float cam_x, float cam_y, float screen_w, float screen_h) {
    (void)cam_y;        // In this simple parallax, we only scroll horizontally
    (void)screen_h; 

    for (const auto& layer : _layers) {
        if (!sheet.Has(layer.sprite)) continue;
        const Sprite& sprite = sheet.Get(layer.sprite);

        // Calculate how much the layer has scrolled
        float scroll_x = cam_x * layer.parallax_factor;

        // Find the starting x position that is just left of the left screen edge
        // This ensures seamless tiling
        float start_x = std::floor((cam_x - scroll_x) / layer.tile_width) * layer.tile_width;
        float draw_x = start_x + scroll_x;

        // Draw enough tiles to cover the screen horizontally, plus one extra for smooth scrolling
        while (draw_x < cam_x + screen_w + layer.tile_width) {
            batch.Draw(draw_x, layer.y_offset, layer.tile_width, layer.tile_height,
                       sprite, 1.0f, 1.0f, 1.0f, 1.0f);
            draw_x += layer.tile_width;
        }
    }
}
