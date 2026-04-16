#include "ScoreRenderer.h"
#include <string>

ScoreRenderer::ScoreRenderer(const Texture* font_texture) {
    for (int i = 0; i < 5; ++i) {
        _digits[i]     = Sprite(font_texture, 496 + i * DIGIT_W, 225, DIGIT_W, DIGIT_H);
        _digits[i + 5] = Sprite(font_texture, 496 + i * DIGIT_W, 239, DIGIT_W, DIGIT_H);
    }
}

void ScoreRenderer::Draw(SpriteBatch& batch, int score, float screen_x, float screen_y, 
                          float cam_x, float cam_y) const {
    std::string s = std::to_string(score);
    for (size_t i = 0; i < s.size(); ++i) {
        int digit = s[i] - '0';
        float draw_x = cam_x + screen_x + i * (DIGIT_W * DIGIT_SCALE + DIGIT_GAP);
        float draw_y = cam_y + screen_y;
        batch.Draw(draw_x, draw_y, DIGIT_W * DIGIT_SCALE, DIGIT_H * DIGIT_SCALE, _digits[digit], 1.0f, 1.0f, 1.0f, 1.0f);
    }
}
