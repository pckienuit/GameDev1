#include "Player.h"

Player::Player(float start_x, float start_y):
    _pos_x(start_x),
    _pos_y(start_y),
    _vel_x(0.0f),
    _vel_y(0.0f),
    _is_grounded(false)
{}

void Player::Update(float dt, bool move_left, bool move_right, bool jump_pressed, const Tilemap& tilemap) {
    _is_grounded = false;
    _vel_x = 0.0f;

    if (move_left)  _vel_x = -SPEED;
    if (move_right) _vel_x =  SPEED;

    // === MOVE & RESOLVE X ===
    _pos_x += _vel_x * dt;

    if (_vel_x > 0.0f) {
        //Check player right edge
        int col_right = tilemap.PixelToCol(_pos_x + PLAYER_W);
        int row_top   = tilemap.PixelToRow(_pos_y + P_DISTANCE);
        int row_bot   = tilemap.PixelToRow(_pos_y + PLAYER_H - P_DISTANCE);

        if (tilemap.IsSolid(col_right, row_top) || tilemap.IsSolid(col_right, row_bot)) {
            _pos_x = col_right * tilemap.GetTileSize() - PLAYER_W;
            _vel_x = 0;
        }
    }
    else if (_vel_x < 0.0f) {
        //Check player left edge
        int col_left = tilemap.PixelToCol(_pos_x);
        int row_top   = tilemap.PixelToRow(_pos_y + P_DISTANCE);
        int row_bot   = tilemap.PixelToRow(_pos_y + PLAYER_H - P_DISTANCE);

        if (tilemap.IsSolid(col_left, row_top) || tilemap.IsSolid(col_left, row_bot)) {
            _pos_x = (col_left + 1) * tilemap.GetTileSize();
            _vel_x = 0;
        }
    }

    // === MOVE & RESOLVE Y ===
    _vel_y += GRAVITY * dt;
    _pos_y += _vel_y * dt;

    if (_vel_y > 0.0f) {
        // Check falling
        int tile_row = tilemap.PixelToRow(_pos_y + PLAYER_H);
        int col_left = tilemap.PixelToCol(_pos_x + P_DISTANCE);
        int col_right = tilemap.PixelToCol(_pos_x + PLAYER_W - P_DISTANCE);

        if (tilemap.IsSolid(col_left, tile_row) || tilemap.IsSolid(col_right, tile_row)) {
            _pos_y = tile_row * tilemap.GetTileSize() - PLAYER_H;
            _vel_y = 0;
            _is_grounded = true;
        }
    }
    else if (_vel_y < 0.0f) {
        // Check jumping
        int tile_row = tilemap.PixelToRow(_pos_y);
        int col_left = tilemap.PixelToCol(_pos_x + P_DISTANCE);
        int col_right = tilemap.PixelToCol(_pos_x + PLAYER_W - P_DISTANCE);
        if (tilemap.IsSolid(col_left, tile_row) || tilemap.IsSolid(col_right, tile_row)) {
            _pos_y = (tile_row+1) * tilemap.GetTileSize();
            _vel_y = 0;
        }
    }

    // Jump on ground only
    if (jump_pressed && _is_grounded)
        _vel_y = -JUMP_SPEED;
}