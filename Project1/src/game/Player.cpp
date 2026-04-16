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
    _anim_jump.AddFrame(246, 233, 16, 26, 1.0f);   // Jump
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

void Player::PrepareVelocity(float dt, bool move_left, bool move_right, bool jump_pressed, bool jump_held) {
    //X Vel — acceleration based
    float target_dir = 0.0f;
    if (move_right) { target_dir =  1.0f; _facing_left = false; }
    if (move_left)  { target_dir = -1.0f; _facing_left = true;  }

    if (target_dir != 0.0f) {
        bool  skidding = (_vel_x * target_dir < 0.0f);
        float accel    = (skidding ? SKID_DECEL : ACCELERATION);
        if (!_is_grounded) accel *= AIR_CTRL_FACTOR;
        _vel_x += target_dir * accel * dt;
        _vel_x  = std::clamp(_vel_x, -MAX_SPEED, MAX_SPEED);
    } else {
        float decel_rate = _is_grounded ? DECELERATION : DECELERATION * AIR_CTRL_FACTOR;
        float sign       = (_vel_x > 0.0f) ? 1.0f : -1.0f;
        _vel_x -= sign * min(std::abs(_vel_x), decel_rate * dt);  // no overshoot
    }

    //Jump buffer
    if (jump_pressed) {
        _jump_buffer_timer = JUMP_BUFFER_TIME;
    }
    else {
        _jump_buffer_timer = max(0.0f, _jump_buffer_timer - dt);
    }

    //Coyote time
    if (_is_grounded) {
        _coyote_timer = COYOTE_TIME;
    }
    else {
        _coyote_timer = max(0.0f, _coyote_timer - dt);
    }

    //Y Vel
    _vel_y += GRAVITY * dt;

    if (!jump_held && _vel_y < 0.0f) {
        _vel_y += GRAVITY * (JUMP_CUT_FACTOR - 1.0f) * dt;
    }

    bool wants_jump = _jump_buffer_timer > 0.0f;          // player has intent
    bool can_jump   = _is_grounded || _coyote_timer > 0.0f;  // player has ability
    if (wants_jump && can_jump) {
        _vel_y = -JUMP_SPEED;
        _coyote_timer      = 0.0f;  // consume — prevent double-jump off edge
        _jump_buffer_timer = 0.0f;  // consume buffer
    }
}

void Player::Move(float dt, const Tilemap& tilemap) {
    if (_game_over) {
        _pos_y += _vel_y * dt; //fall only, no tilemap
        return;
    }

    _is_grounded = false;
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
}

void Player::ClampVelocityAlongNormal(float normal_x, float normal_y, float hit_time) {
    float dot = _vel_x * normal_x + _vel_y * normal_y;
    if (dot < 0) {
        _vel_x -= dot * normal_x;
        _vel_y -= dot * normal_y;
    }
}

void Player::Hurt() {
    if (IsInvincible()) return;   // immune
    --_lives;
    if (_lives <= 0) {
        _game_over = true;
        _vel_x = 0;
        _vel_y = 0;
    } else {
        _inv_timer = INVINCIBILITY_TIME;
        _is_dead = false;
    }
}

void Player::TickInvincibility(float dt) {
    if (_inv_timer > 0.0f) {
        _inv_timer -= dt;
    }
}

bool Player::ShouldRender() const {
    if (_is_dead) return false;
    if (_inv_timer <= 0.0f) return true;
    return static_cast<int>(_inv_timer / 0.1f) % 2 == 0;
}