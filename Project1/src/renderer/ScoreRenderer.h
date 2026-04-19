#pragma once
#include "Sprite.h"
#include "Texture.h"
#include "SpriteBatch.h"
#include "SpriteSheet.h"
#include <algorithm>
#include <string>

class ScoreRenderer {
public:
    explicit ScoreRenderer(const Texture* font_texture);

    void Draw(SpriteBatch& batch, int score, float screen_x, float screen_y, 
              float cam_x, float cam_y) const;

    void DrawLives(SpriteBatch& batch, int lives, float screen_x, float screen_y, 
                   float cam_x, float cam_y) const;

private:
    static constexpr int   DIGIT_W    = 8;
    static constexpr int   DIGIT_H    = 14;
    static constexpr float DIGIT_SCALE = 2.5f;
    static constexpr int   DIGIT_DGAP = 4;
    static constexpr int   DIGIT_GAP  = 2;

    SpriteSheet            _sheet;

    static constexpr int   HEART_W    = 10;
    static constexpr int   HEART_H    = 10;
    static constexpr int   HEART_GAP  = 4;
};
