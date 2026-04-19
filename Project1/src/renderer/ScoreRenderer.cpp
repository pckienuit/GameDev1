#include "ScoreRenderer.h"
#include <string>

ScoreRenderer::ScoreRenderer(const Texture* font_texture) : _sheet(font_texture) {
    for (int i = 0; i < 5; ++i) {
        int gap;
        if (i == 0) gap = 0;
        else gap = DIGIT_GAP;
        _sheet.Define(std::string(1, '0' + i),     496 + i * DIGIT_W + gap, 224, DIGIT_W, DIGIT_H);
        _sheet.Define(std::string(1, '0' + i + 5), 496 + i * DIGIT_W + gap, 240, DIGIT_W, DIGIT_H);
    }
    _sheet.Define("heart", 596, 193, HEART_W, HEART_H);
}

void ScoreRenderer::Draw(SpriteBatch& batch, int score, float screen_x, float screen_y, 
                          float cam_x, float cam_y) const {
    std::string s = std::to_string(score);
    for (size_t i = 0; i < s.size(); ++i) {
        float draw_x = cam_x + screen_x + i * (DIGIT_W * DIGIT_SCALE + DIGIT_DGAP);
        float draw_y = cam_y + screen_y;
        batch.Draw(draw_x, draw_y, DIGIT_W * DIGIT_SCALE, DIGIT_H * DIGIT_SCALE, _sheet.GetChar(s[i]), 1.0f, 1.0f, 1.0f, 1.0f);
    }
}


void ScoreRenderer::DrawLives(SpriteBatch& batch, int lives,
                               float screen_x, float screen_y,
                               float cam_x, float cam_y) const {
    int   draw_lives      = std::clamp(lives, 0, 9);
    float draw_x          = cam_x + screen_x;
    float draw_y          = cam_y + screen_y;
    float heart_scaled_w  = HEART_W * DIGIT_SCALE;
    float heart_scaled_h  = HEART_H * DIGIT_SCALE;
    float digit_scaled_h  = DIGIT_H * DIGIT_SCALE;

    // Center heart vertically against the taller digit
    float heart_offset_y  = (digit_scaled_h - heart_scaled_h) / 2.0f;

    batch.Draw(draw_x, draw_y + heart_offset_y,
               heart_scaled_w, heart_scaled_h,
               _sheet.Get("heart"), 1.0f, 1.0f, 1.0f, 1.0f);

    float digit_x = draw_x + heart_scaled_w + HEART_GAP;
    batch.Draw(digit_x, draw_y,
               DIGIT_W * DIGIT_SCALE, digit_scaled_h,
               _sheet.GetChar(draw_lives + '0'), 1.0f, 1.0f, 1.0f, 1.0f);
}
