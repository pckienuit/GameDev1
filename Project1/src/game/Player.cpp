#include "Player.h"

Player::Player(float start_x, float start_y, const Texture* texture, EntityManager& entity_manager):
    _pos_x(start_x),
    _pos_y(start_y),
    _vel_x(0.0f),
    _vel_y(0.0f),
    _is_grounded(false),
    _anim_idle(texture),
    _anim_walk(texture),
    _anim_jump(texture),
    _facing_left(false),
    _id(entity_manager.Create())
{
    // Sprite sheet: 256x64, 4 frames each 64x64
    _anim_idle.AddFrame(245, 154, 16, 26, 0.4f);   // Idle
    _anim_walk.AddFrame(275, 154, 16, 26, 0.15f);  // Walk1
    _anim_walk.AddFrame(305, 154, 16, 26, 0.15f);  // Walk2
    _anim_walk.AddFrame(335, 154, 16, 26, 0.15f);  // Walk3
    _anim_jump.AddFrame(390, 154, 16, 26, 1.0f);   // Jump
    _anim_jump.SetLooping(false);
}

void Player::Update(float dt, bool move_left, bool move_right, bool jump_pressed, const Tilemap& tilemap) {
    _is_grounded = false;
    _vel_x = 0.0f;

    if (move_left)  {
        _vel_x = -SPEED;
        _facing_left = true;
    }
    if (move_right) {
        _vel_x =  SPEED;
        _facing_left = false;
    } 
    
    // === MOVE & RESOLVE X ===
    _pos_x += _vel_x * dt;

    if (_vel_x > 0.0f) {
        //Check player right edge
        int col_right = tilemap.PixelToCol(_pos_x + PLAYER_W);
        int row_top   = tilemap.PixelToRow(_pos_y + P_DISTANCE);
        int row_mid   = tilemap.PixelToRow(_pos_y + PLAYER_H*0.5f);
        int row_bot   = tilemap.PixelToRow(_pos_y + PLAYER_H - P_DISTANCE);

        if (tilemap.IsSolid(col_right, row_top) || 
            tilemap.IsSolid(col_right, row_bot)|| 
            tilemap.IsSolid(col_right, row_mid)) {
            _pos_x = col_right * tilemap.GetTileSize() - PLAYER_W;
            _vel_x = 0;
        }
    }
    else if (_vel_x < 0.0f) {
        //Check player left edge
        int col_left = tilemap.PixelToCol(_pos_x);
        int row_top   = tilemap.PixelToRow(_pos_y + P_DISTANCE);
        int row_mid   = tilemap.PixelToRow(_pos_y + PLAYER_H*0.5f);
        int row_bot   = tilemap.PixelToRow(_pos_y + PLAYER_H - P_DISTANCE);

        if (tilemap.IsSolid(col_left, row_top) || 
            tilemap.IsSolid(col_left, row_bot)|| 
            tilemap.IsSolid(col_left, row_mid)) {
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
        int col_mid  = tilemap.PixelToCol(_pos_x + PLAYER_W*0.5);
        int col_right = tilemap.PixelToCol(_pos_x + PLAYER_W - P_DISTANCE);

        if (tilemap.IsSolid(col_left, tile_row) || 
            tilemap.IsSolid(col_right, tile_row)||
            tilemap.IsSolid(col_mid, tile_row)) {
            _pos_y = tile_row * tilemap.GetTileSize() - PLAYER_H;
            _vel_y = 0;
            _is_grounded = true;
        }
    }
    else if (_vel_y < 0.0f) {
        // Check jumping
        int tile_row = tilemap.PixelToRow(_pos_y);
        int col_left = tilemap.PixelToCol(_pos_x + P_DISTANCE);
        int col_mid  = tilemap.PixelToCol(_pos_x + PLAYER_W*0.5);
        int col_right = tilemap.PixelToCol(_pos_x + PLAYER_W - P_DISTANCE);
        if (tilemap.IsSolid(col_left, tile_row) || 
            tilemap.IsSolid(col_right, tile_row)|| 
            tilemap.IsSolid(col_mid, tile_row)) {
            _pos_y = (tile_row+1) * tilemap.GetTileSize();
            _vel_y = 0;
        }
    }

    // Jump on ground only
    if (jump_pressed && _is_grounded)
        _vel_y = -JUMP_SPEED;
}

const  Sprite& Player::GetSprite(float dt) {
    if (!_is_grounded) {
        return _anim_jump.Update(dt);
    }
    if (std::abs(_vel_x) > 10.0f) {
        return _anim_walk.Update(dt);
    }
    return _anim_idle.Update(dt);
}

bool Player::IsFacingLeft() const {
    return _facing_left;
}