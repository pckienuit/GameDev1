#include "ScoreRenderer.h"
#include <string>

// Native pixel widths for each letter A-Z (index 0 = 'A', etc.)
// Taken from the sprite sheet mapping in Game.cpp
static constexpr int CHAR_WIDTHS_AZ[26] = {
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, // A-J: all 7px
    7, 7, 8, 7, 7, 7, 8, 7, 7, 7, // K-T: KLM=7/8, N-P=7, Q=8, R-T=7
    7, 7, 7, 8, 7                 // U-Z: U-V=7, W=7, X=7, Y=8, Z=7
};

// Native pixel widths for digits 0-9
static constexpr int CHAR_WIDTHS_DIGITS[10] = {
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

int ScoreRenderer::GetCharWidth(char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return CHAR_WIDTHS_AZ[ch - 'A'];
    }
    if (ch >= 'a' && ch <= 'z') {
        return CHAR_WIDTHS_AZ[ch - 'a'];
    }
    if (ch >= '0' && ch <= '9') {
        return CHAR_WIDTHS_DIGITS[ch - '0'];
    }
    return -1; // unsupported
}

float ScoreRenderer::MeasureTextWidth(const std::string& text, float scale) {
    if (text.empty()) return 0.0f;

    float width = 0.0f;
    const float base_gap = DIGIT_DGAP * (scale / DIGIT_SCALE);

    for (size_t i = 0; i < text.size(); ++i) {
        char ch = text[i];
        if (ch == ' ') {
            width += DIGIT_W * scale + base_gap;
            continue;
        }
        int char_w = GetCharWidth(ch);
        if (char_w < 0) char_w = DIGIT_W;
        width += char_w * scale + base_gap;
    }
    return width;
}

ScoreRenderer::ScoreRenderer(const SpriteSheet& sheet) : _sheet(sheet) {}


void ScoreRenderer::Draw(SpriteBatch& batch, int score, float screen_x, float screen_y,
                          float cam_x, float cam_y) const {
    std::string s = std::to_string(score);
    for (size_t i = 0; i < s.size(); ++i) {
        float draw_x = cam_x + screen_x + i * (DIGIT_W * DIGIT_SCALE + DIGIT_DGAP);
        float draw_y = cam_y + screen_y;
        SpriteID id  = static_cast<SpriteID>(static_cast<int>(SpriteID::Digit0) + (s[i] - '0'));
        batch.Draw(draw_x, draw_y, DIGIT_W * DIGIT_SCALE, DIGIT_H * DIGIT_SCALE,
                   _sheet.Get(id), 1.0f, 1.0f, 1.0f, 1.0f);
    }
}


void ScoreRenderer::DrawLives(SpriteBatch& batch, int lives,
                               float screen_x, float screen_y,
                               float cam_x, float cam_y) const {
    int   draw_lives     = std::clamp(lives, 0, 9);
    float draw_x         = cam_x + screen_x;
    float draw_y         = cam_y + screen_y;
    float heart_scaled_w = HEART_W * DIGIT_SCALE;
    float heart_scaled_h = HEART_H * DIGIT_SCALE;
    float digit_scaled_h = DIGIT_H * DIGIT_SCALE;

    // Center heart vertically against the taller digit
    float heart_offset_y = (digit_scaled_h - heart_scaled_h) / 2.0f;

    batch.Draw(draw_x, draw_y + heart_offset_y,
               heart_scaled_w, heart_scaled_h,
               _sheet.Get(SpriteID::Heart), 1.0f, 1.0f, 1.0f, 1.0f);

    float digit_x = draw_x + heart_scaled_w + HEART_GAP;
    SpriteID id   = static_cast<SpriteID>(static_cast<int>(SpriteID::Digit0) + draw_lives);
    batch.Draw(digit_x, draw_y,
               DIGIT_W * DIGIT_SCALE, digit_scaled_h,
               _sheet.Get(id), 1.0f, 1.0f, 1.0f, 1.0f);
}

void ScoreRenderer::DrawText(SpriteBatch& batch, const std::string& text,
                              float screen_x, float screen_y,
                              float cam_x, float cam_y, float scale) const {
    float cursor_x = cam_x + screen_x;
    float cursor_y = cam_y + screen_y;
    const float char_h = DIGIT_H * scale;
    const float base_gap = DIGIT_DGAP * (scale / DIGIT_SCALE);

    for (size_t i = 0; i < text.size(); ++i) {
        char ch = text[i];

        if (ch == ' ') {
            cursor_x += DIGIT_W * scale + base_gap;
            continue;
        }

        SpriteID id = SpriteID::Count; // invalid sentinel
        int char_w = DIGIT_W;

        if (ch >= 'A' && ch <= 'Z') {
            id = static_cast<SpriteID>(static_cast<int>(SpriteID::LetterA) + (ch - 'A'));
            char_w = CHAR_WIDTHS_AZ[ch - 'A'];
        } else if (ch >= 'a' && ch <= 'z') {
            id = static_cast<SpriteID>(static_cast<int>(SpriteID::LetterA) + (ch - 'a'));
            char_w = CHAR_WIDTHS_AZ[ch - 'a'];
        } else if (ch >= '0' && ch <= '9') {
            id = static_cast<SpriteID>(static_cast<int>(SpriteID::Digit0) + (ch - '0'));
            char_w = CHAR_WIDTHS_DIGITS[ch - '0'];
        }

        if (id != SpriteID::Count && _sheet.Has(id)) {
            batch.Draw(cursor_x, cursor_y, char_w * scale, char_h,
                       _sheet.Get(id), 1.0f, 1.0f, 1.0f, 1.0f);
        }

        cursor_x += char_w * scale + base_gap;
    }
}
