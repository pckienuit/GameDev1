#pragma once
#include "Sprite.h"
#include "Texture.h"
#include "SpriteBatch.h"

class ScoreRenderer {
public:
    explicit ScoreRenderer(const Texture* font_texture);

    void Draw(SpriteBatch& batch, int score, float screen_x, float screen_y, 
              float cam_x, float cam_y) const;

private:
    static constexpr int   DIGIT_W    = 8;
    static constexpr int   DIGIT_H    = 12;
    static constexpr float DIGIT_SCALE = 2.0f;
    static constexpr int   DIGIT_GAP  = 2;

    Sprite _digits[10];  // index = digit value
};
