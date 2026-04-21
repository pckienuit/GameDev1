#pragma once
#include "SpriteBatch.h"
#include "SpriteSheet.h"
#include <algorithm>
#include <string>

class ScoreRenderer {
public:
    explicit ScoreRenderer(const SpriteSheet& sheet);

    void Draw(SpriteBatch& batch, int score, float screen_x, float screen_y,
              float cam_x, float cam_y) const;

    void DrawLives(SpriteBatch& batch, int lives, float screen_x, float screen_y,
                   float cam_x, float cam_y) const;

    // Bitmap text: supports A-Z, 0-9, space
    void DrawText(SpriteBatch& batch, const std::string& text,
                  float screen_x, float screen_y,
                  float cam_x, float cam_y, float scale = 2.5f) const;

private:
    static constexpr int   DIGIT_W     = 8;
    static constexpr int   DIGIT_H     = 14;
    static constexpr float DIGIT_SCALE = 2.5f;
    static constexpr int   DIGIT_DGAP  = 4;

    static constexpr int   HEART_W     = 10;
    static constexpr int   HEART_H     = 10;
    static constexpr int   HEART_GAP   = 4;

    const SpriteSheet&     _sheet;
};
