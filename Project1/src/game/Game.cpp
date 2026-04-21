#include "Game.h"
#include <string>
#include <cmath>

Game::Game() : _window("Mario Engine", 800, 600),
                _renderer(_window),
                _sprite_batch(_renderer.GetDevice(), _renderer.GetContext()),
                _sprite_sheet(_tex_registry, _renderer.GetDevice()),
                _score_renderer(_sprite_sheet),
                _entity_manager(),
                _tilemap(0, 0, 0),
                _collision_system(0, 0, CELL_SIZE, MAX_EVENTS),
                _camera(800.0f, 600.0f),
                _player(200.0f, 100.0f, _sprite_sheet, _entity_manager),
                _dummy_id(_entity_manager.Create()),
                _dummy_aabb({ 100.0f, 200.0f, 48.0f, 96.0f }),
                _enemy_manager(_entity_manager),
                _fade_texture(_renderer.GetDevice(), 255, 255, 255, 255),
                _fade_sprite(&_fade_texture, 0.0f, 0.0f,1.0f, 1.0f)
{
    // Audio — non-fatal: game runs silently if device is unavailable
    const bool audio_ok = _sound_manager.Init();
    OutputDebugStringA(audio_ok ? "[Sound] XAudio2 OK\n" : "[Sound] XAudio2 FAILED\n");

    auto log_load = [](bool ok, const char* name) {
        OutputDebugStringA(ok
            ? ("[Sound] Loaded: " + std::string(name) + "\n").c_str()
            : ("[Sound] FAILED: " + std::string(name) + "\n").c_str());
    };
    log_load(_sound_manager.Load(SoundID::Jump,  "assets/sounds/jump.wav"),  "jump.wav");
    log_load(_sound_manager.Load(SoundID::Stomp, "assets/sounds/stomp.wav"), "stomp.wav");
    log_load(_sound_manager.Load(SoundID::Hurt,  "assets/sounds/hurt.wav"),  "hurt.wav");
    log_load(_sound_manager.Load(SoundID::Die,   "assets/sounds/die.wav"),   "die.wav");
    log_load(_sound_manager.Load(SoundID::Coin,  "assets/sounds/coin.wav"),  "coin.wav");

    // -----------------------------------------------------------------------
    // Central sprite registry — ALL UV coordinates live here, nowhere else
    // -----------------------------------------------------------------------

    // Tilemap
    _sprite_sheet.Define(SpriteID::BrickTile,  "assets/misc.png", 273, 9, 16, 16);
    _sprite_sheet.Define(SpriteID::GroundTile, "assets/misc.png", 0, 0, 16, 16); // PLACEHOLDER
    _sprite_sheet.Define(SpriteID::QBlockTile0,"assets/misc.png", 0, 0, 16, 16); // PLACEHOLDER
    _sprite_sheet.Define(SpriteID::PipeTL,     "assets/misc.png", 0, 0, 16, 16); // PLACEHOLDER
    _sprite_sheet.Define(SpriteID::PipeTR,     "assets/misc.png", 0, 0, 16, 16); // PLACEHOLDER
    _sprite_sheet.Define(SpriteID::PipeL,      "assets/misc.png", 0, 0, 16, 16); // PLACEHOLDER
    _sprite_sheet.Define(SpriteID::PipeR,      "assets/misc.png", 0, 0, 16, 16); // PLACEHOLDER
    
    // Backgrounds
    _sprite_sheet.Define(SpriteID::BgMountain, "assets/misc.png", 0, 0, 256, 128); // PLACEHOLDER
    _sprite_sheet.Define(SpriteID::BgClouds,   "assets/misc.png", 0, 0, 256, 128); // PLACEHOLDER
    _sprite_sheet.Define(SpriteID::BgTrees,    "assets/misc.png", 0, 0, 256, 128); // PLACEHOLDER
    _sprite_sheet.Define(SpriteID::BgCastle,   "assets/misc.png", 0, 0, 256, 128); // PLACEHOLDER
    _sprite_sheet.Define(SpriteID::BgStars,    "assets/misc.png", 0, 0, 256, 128); // PLACEHOLDER

    // Player — mario.png
    _sprite_sheet.Define(SpriteID::MarioIdle,  "assets/mario.png", 245, 154, 16, 26);
    _sprite_sheet.Define(SpriteID::MarioWalk0, "assets/mario.png", 275, 154, 16, 26);
    _sprite_sheet.Define(SpriteID::MarioWalk1, "assets/mario.png", 305, 154, 16, 26);
    _sprite_sheet.Define(SpriteID::MarioWalk2, "assets/mario.png", 335, 154, 16, 26);
    _sprite_sheet.Define(SpriteID::MarioJump,  "assets/mario.png", 246, 233, 16, 26);

    // Goomba — enemies.png
    _sprite_sheet.Define(SpriteID::GoombaWalk0, "assets/enemies.png",  5, 14, 16, 16);
    _sprite_sheet.Define(SpriteID::GoombaWalk1, "assets/enemies.png", 25, 14, 16, 16);
    _sprite_sheet.Define(SpriteID::GoombaDead,  "assets/enemies.png", 45, 21, 16,  8);

    // Koopa — enemies.png
    _sprite_sheet.Define(SpriteID::KoopaWalk0, "assets/enemies.png",  6, 130, 16, 26);
    _sprite_sheet.Define(SpriteID::KoopaWalk1, "assets/enemies.png", 28, 130, 16, 26);
    _sprite_sheet.Define(SpriteID::KoopaDead,  "assets/enemies.png", 50, 139, 16, 16);
    _sprite_sheet.Define(SpriteID::KoopaShell, "assets/enemies.png", 70, 139, 16, 16);

    // FlyKoopa — enemies.png (PLACEHOLDERS)
    _sprite_sheet.Define(SpriteID::FlyKoopaWalk0, "assets/enemies.png", 0, 0, 16, 26); // PLACEHOLDER
    _sprite_sheet.Define(SpriteID::FlyKoopaWalk1, "assets/enemies.png", 0, 0, 16, 26); // PLACEHOLDER
    _sprite_sheet.Define(SpriteID::FlyKoopaWing,  "assets/enemies.png", 0, 0, 16, 26); // PLACEHOLDER

    // Piranha Plant — enemies.png (PLACEHOLDERS)
    _sprite_sheet.Define(SpriteID::PiranhaUp0, "assets/enemies.png", 0, 0, 16, 24); // PLACEHOLDER
    _sprite_sheet.Define(SpriteID::PiranhaUp1, "assets/enemies.png", 0, 0, 16, 24); // PLACEHOLDER

    // Objects
    _sprite_sheet.Define(SpriteID::Flag, "assets/flag.png", 0, 1268, 160, 160);
    _sprite_sheet.Define(SpriteID::Coin0, "assets/misc.png", 303, 99,  10,  16);
    _sprite_sheet.Define(SpriteID::Coin1, "assets/misc.png", 321, 99,  10,  16);
    _sprite_sheet.Define(SpriteID::Coin2, "assets/misc.png", 339, 99,  10,  16);

    // HUD — misc.png
    // Digits 0-4 at y=224, digits 5-9 at y=240; gap of 2px skipped for frame 0
    std::string digit_texture_path = "assets/misc.png";
    _sprite_sheet.Define(SpriteID::Digit0, digit_texture_path, 496, 225, 8, 13); //digit 0
    _sprite_sheet.Define(SpriteID::Digit1, digit_texture_path, 505, 225, 8, 13); //digit 1
    _sprite_sheet.Define(SpriteID::Digit2, digit_texture_path, 514, 225, 8, 13); //digit 2
    _sprite_sheet.Define(SpriteID::Digit3, digit_texture_path, 523, 225, 8, 13); //digit 3
    _sprite_sheet.Define(SpriteID::Digit4, digit_texture_path, 532, 225, 8, 13); //digit 4
    _sprite_sheet.Define(SpriteID::Digit5, digit_texture_path, 496, 239, 8, 13); //digit 5
    _sprite_sheet.Define(SpriteID::Digit6, digit_texture_path, 505, 239, 8, 13); //digit 6
    _sprite_sheet.Define(SpriteID::Digit7, digit_texture_path, 514, 239, 8, 13); //digit 7
    _sprite_sheet.Define(SpriteID::Digit8, digit_texture_path, 523, 239, 8, 13); //digit 8
    _sprite_sheet.Define(SpriteID::Digit9, digit_texture_path, 532, 239, 8, 13); //digit 9

    _sprite_sheet.Define(SpriteID::Heart, "assets/misc.png", 596, 192, 10, 10);

    // Bitmap font A-Z — misc.png (PLACEHOLDERS — user maps real coords)
    for (int i = 0; i < 26; ++i) {
        SpriteID id = static_cast<SpriteID>(static_cast<int>(SpriteID::LetterA) + i);
        _sprite_sheet.Define(id, "assets/misc.png", 0, 0, 8, 13); // PLACEHOLDER
    }

    // -----------------------------------------------------------------------

    _coin_anim = Animation(_sprite_sheet, {SpriteID::Coin0,
                                          SpriteID::Coin1,
                                          SpriteID::Coin2}, 0.3f, true);

    // Start at Title screen — don't load level yet
    _state       = GameState::Title;
    _fade_alpha  = 0.0f;
}

void Game::LoadLevel(const LevelDef& level) {
    // ---- Clear dynamic state ----
    _enemy_manager.ClearAll();
    _coins.clear();
    _flag_aabb = { -9999.0f, 0.0f, 0.0f, 0.0f };

    // ---- Background color & layers ----
    _bg_r = level.bg_r;
    _bg_g = level.bg_g;
    _bg_b = level.bg_b;

    _background.Clear();
    
    // Setup specific background styles depending on level index
    int lvl_idx = _level_manager.GetLevelIndex();
    if (lvl_idx == 0) {
        // Level 1 - Mountains & Clouds
        _background.AddLayer(SpriteID::BgClouds,   0.2f, 50.0f,  256.0f, 128.0f);
        _background.AddLayer(SpriteID::BgMountain, 0.5f, 200.0f, 256.0f, 128.0f);
        _background.AddLayer(SpriteID::BgTrees,    0.8f, 350.0f, 256.0f, 128.0f);
    } 
    else if (lvl_idx == 1) {
        // Level 2 - Nighttime (Stars)
        _background.AddLayer(SpriteID::BgStars, 0.1f, 0.0f, 256.0f, 128.0f);
        _background.AddLayer(SpriteID::BgTrees, 0.6f, 300.0f, 256.0f, 128.0f);
    }
    else {
        // Level 3 - Sunset Castle
        _background.AddLayer(SpriteID::BgClouds, 0.2f, 100.0f,  256.0f, 128.0f);
        _background.AddLayer(SpriteID::BgCastle, 0.5f, 200.0f, 256.0f, 128.0f);
    }

    // ---- Reset player position (keep score & lives) ----
    _player.SetPosition(level.player_start_x, level.player_start_y);

    // ---- Reset camera ----
    _camera.Reset();

    // ---- Load tilemap ----
    _tilemap.LoadFromFile(level.map_file);
    _collision_system.Resize(static_cast<int>(_tilemap.GetWidth()),
                             static_cast<int>(_tilemap.GetHeight()), CELL_SIZE);

    // ---- Spawn entities from map ----
    constexpr float FLAG_W = 96.0f;
    constexpr float FLAG_H = 96.0f;
    constexpr float COIN_W = 32.0f;
    constexpr float COIN_H = 32.0f;

    for (const SpawnInfo& spawn : _tilemap.GetSpawnPoints()) {
        const float sx = static_cast<float>(spawn.x);
        const float sy = static_cast<float>(spawn.y);
        if (spawn.token == 'G') {
            _enemy_manager.Spawn(EnemyDef::GOOMBA, _sprite_sheet, sx, sy);
        } else if (spawn.token == 'K') {
            _enemy_manager.Spawn(EnemyDef::KOOPA, _sprite_sheet, sx, sy);
        } else if (spawn.token == 'P') {
            _enemy_manager.Spawn(EnemyDef::PIRANHA, _sprite_sheet, sx, sy);
        } else if (spawn.token == 'W') {
            _enemy_manager.Spawn(EnemyDef::FLY_KOOPA, _sprite_sheet, sx, sy);
        } else if (spawn.token == 'F') {
            _flag_aabb = { sx, sy, FLAG_W, FLAG_H };
        } else if (spawn.token == 'C') {
            _coins.push_back({ sx, sy, COIN_W, COIN_H });
        }
    }

    _prev_grounded = true;
    _prev_hurt     = false;
}

// =======================================================================
// UPDATE — State machine dispatcher
// =======================================================================

bool Game::Update() {
    if (!_window.ProcessMessages()) return false;
    const float real_dt = static_cast<float>(_game_loop.Tick());

    switch (_state) {
        case GameState::Title:         UpdateTitle(real_dt);         break;
        case GameState::LevelIntro:    UpdateLevelIntro(real_dt);    break;
        case GameState::Playing:       UpdatePlaying(real_dt);       break;
        case GameState::Dying:         UpdateDying(real_dt);         break;
        case GameState::GameOver:      UpdateGameOver(real_dt);      break;
        case GameState::LevelComplete: UpdateLevelComplete(real_dt); break;
        case GameState::Victory:       UpdateVictory(real_dt);       break;
    }

    return true;
}

// -----------------------------------------------------------------------
// Title: wait for Enter
// -----------------------------------------------------------------------
void Game::UpdateTitle(float real_dt) {
    (void)real_dt;
    _input.Poll();
    if (_input.IsPressed(Action::Jump)) {   // Enter / Space
        _level_manager.Reset();
        _score = 0;
        _player.FullReset(_level_manager.GetCurrent().player_start_x,
                          _level_manager.GetCurrent().player_start_y);
        LoadLevel(_level_manager.GetCurrent());
        _state       = GameState::LevelIntro;
        _state_timer = 2.0f;
        _fade_alpha  = 0.0f;
    }
}

// -----------------------------------------------------------------------
// LevelIntro: "WORLD X" for 2 seconds
// -----------------------------------------------------------------------
void Game::UpdateLevelIntro(float real_dt) {
    _state_timer -= real_dt;
    if (_state_timer <= 0.0f) {
        _state      = GameState::Playing;
        _fade_alpha = 1.0f; // fade in from black
    }
}

// -----------------------------------------------------------------------
// Playing: the original gameplay logic
// -----------------------------------------------------------------------
void Game::UpdatePlaying(float real_dt) {
    // Fade in
    if (_fade_alpha > 0.0f) {
        _fade_alpha -= FADE_SPEED * real_dt;
        if (_fade_alpha < 0.0f) _fade_alpha = 0.0f;
    }

    const CollisionEventPool& pool = _collision_system.GetEvents();

    while (_game_loop.ShouldUpdate()) {
        _input.Poll();
        _player.TickInvincibility(DT);
        _player.PrepareVelocity(DT,
            _input.IsHeld(Action::MoveLeft), _input.IsHeld(Action::MoveRight),
            _input.IsPressed(Action::Jump),  _input.IsHeld(Action::Jump));
        _enemy_manager.Update(DT, _tilemap);
        _collision_system.BeginFrame();
        _collision_system.Register(_player.GetID(), _player.GetAABB(), _player.GetVelX(), _player.GetVelY());
        _collision_system.Register(_dummy_id, _dummy_aabb, 0.0f, 0.0f);
        _enemy_manager.RegisterAll(_collision_system);
        _collision_system.Detect();
        _enemy_manager.HandleCollisions(pool, _player.GetID(), _player, _sprite_sheet);

        // Collect coins
        for (auto& coin : _coins) {
            if (coin.x < -9000.0f) continue;
            if (AABB::Overlaps(_player.GetAABB(), coin)) {
                coin.x = -9999.0f;
                _score += 10;
                _sound_manager.Play(SoundID::Coin);
            }
        }

        const int stomped = _enemy_manager.PopScore();
        if (stomped > 0) {
            _score += stomped;
            _sound_manager.Play(SoundID::Stomp);
        }

        for (int i = 0; i < pool.Count(); ++i) {
            const CollisionEvent& ev = pool.Get(i);
            if (ev.entity_a == _player.GetID())
                _player.ClampVelocityAlongNormal(ev.normal_x, ev.normal_y, ev.hit_time);
            if (ev.entity_b == _player.GetID())
                _player.ClampVelocityAlongNormal(-ev.normal_x, -ev.normal_y, ev.hit_time);
        }
        _player.Move(DT, _tilemap);

        // Jump sound
        const bool grounded_now = _player.IsGrounded();
        if (_prev_grounded && !grounded_now && _input.IsPressed(Action::Jump))
            _sound_manager.Play(SoundID::Jump);
        _prev_grounded = grounded_now;

        // Hurt sound
        const bool hurt_now = _player.IsInvincible();
        if (!_prev_hurt && hurt_now)
            _sound_manager.Play(SoundID::Hurt);
        _prev_hurt = hurt_now;

        // Win condition
        if (AABB::Overlaps(_player.GetAABB(), _flag_aabb)) {
            _state       = GameState::LevelComplete;
            _state_timer = 2.0f;
            _fade_alpha  = 0.0f;
            _game_loop.ConsumeUpdate();
            return;
        }

        // Check death
        if (_player.IsGameOver()) {
            _state            = GameState::Dying;
            _state_timer      = 2.5f;
            _die_sound_played = false;
            _game_loop.ConsumeUpdate();
            return;
        }

        _camera.Follow(_player.GetX(), _player.GetY(), DT);
        _camera.Clamp(_tilemap.GetWidth(), _tilemap.GetHeight());
        _game_loop.ConsumeUpdate();
    }
}

// -----------------------------------------------------------------------
// Dying: death animation, then decide next state
// -----------------------------------------------------------------------
void Game::UpdateDying(float real_dt) {
    if (!_die_sound_played) {
        _sound_manager.Play(SoundID::Die);
        _die_sound_played = true;
    }

    _state_timer -= real_dt;

    // Still run physics so Mario falls
    while (_game_loop.ShouldUpdate()) {
        _player.Move(DT, _tilemap);
        _game_loop.ConsumeUpdate();
    }

    if (_state_timer <= 0.0f) {
        if (_player.GetLives() > 0) {
            // Retry same level
            LoadLevel(_level_manager.GetCurrent());
            _state       = GameState::LevelIntro;
            _state_timer = 2.0f;
            _fade_alpha  = 0.0f;
        } else {
            _state      = GameState::GameOver;
            _fade_alpha = 0.0f;
        }
    }
}

// -----------------------------------------------------------------------
// GameOver: "GAME OVER" — Enter to go back to Title
// -----------------------------------------------------------------------
void Game::UpdateGameOver(float real_dt) {
    (void)real_dt;
    _input.Poll();
    if (_input.IsPressed(Action::Jump)) {
        _state      = GameState::Title;
        _fade_alpha = 0.0f;
    }
}

// -----------------------------------------------------------------------
// LevelComplete: fade out → next level or Victory
// -----------------------------------------------------------------------
void Game::UpdateLevelComplete(float real_dt) {
    _state_timer -= real_dt;

    if (_state_timer <= 0.0f) {
        _fade_alpha += FADE_SPEED * real_dt;
        if (_fade_alpha >= 1.0f) {
            _fade_alpha = 1.0f;
            if (_level_manager.HasNextLevel()) {
                _level_manager.NextLevel();
                LoadLevel(_level_manager.GetCurrent());
                _state       = GameState::LevelIntro;
                _state_timer = 2.0f;
            } else {
                _state      = GameState::Victory;
                _fade_alpha = 0.0f;
            }
        }
    }
}

// -----------------------------------------------------------------------
// Victory: "YOU WIN" — Enter to quit
// -----------------------------------------------------------------------
void Game::UpdateVictory(float real_dt) {
    (void)real_dt;
    _input.Poll();
    if (_input.IsPressed(Action::Jump)) {
        _state      = GameState::Title;
        _fade_alpha = 0.0f;
    }
}

// =======================================================================
// RENDER — State machine dispatcher
// =======================================================================

void Game::Render() {
    _renderer.BeginFrame(_bg_r, _bg_g, _bg_b);
    _sprite_batch.Begin(800.0f, 600.0f, _camera.GetX(), _camera.GetY());

    switch (_state) {
        case GameState::Title:
            RenderCenteredText("MARIO GAME", 180.0f, 5.0f);
            RenderCenteredText("PRESS ENTER TO START", 320.0f, 2.5f);
            break;

        case GameState::LevelIntro: {
            std::string world_text = "WORLD " + std::to_string(_level_manager.GetLevelIndex() + 1);
            RenderCenteredText(world_text, 250.0f, 4.0f);
            break;
        }

        case GameState::Playing:
        case GameState::LevelComplete:
            RenderWorld();
            RenderHUD();
            RenderFade();
            break;

        case GameState::Dying:
            RenderWorld();
            RenderHUD();
            break;

        case GameState::GameOver:
            RenderCenteredText("GAME OVER", 200.0f, 5.0f);
            RenderCenteredText("PRESS ENTER TO RETRY", 340.0f, 2.5f);
            break;

        case GameState::Victory:
            RenderCenteredText("YOU WIN", 180.0f, 5.0f);
            {
                std::string score_text = "SCORE " + std::to_string(_score);
                RenderCenteredText(score_text, 300.0f, 3.0f);
            }
            RenderCenteredText("PRESS ENTER", 400.0f, 2.5f);
            break;
    }

    _sprite_batch.End();
    _renderer.EndFrame();
}

// -----------------------------------------------------------------------
// Render helpers
// -----------------------------------------------------------------------

void Game::RenderWorld() {
    // Background layers
    _background.Render(_sprite_batch, _sprite_sheet, _camera.GetX(), _camera.GetY(), 800.0f, 600.0f);

    // Dummy block
    _sprite_batch.Draw(_dummy_aabb.x, _dummy_aabb.y, _dummy_aabb.w, _dummy_aabb.h,
                       _sprite_sheet.Get(SpriteID::BrickTile), 1.0f, 0.2f, 0.2f, 1.0f);

    // Player
    if (_player.ShouldRender()) {
        _sprite_batch.Draw(_player.GetX(), _player.GetY(), _player.GetW(), _player.GetH(),
                           _player.GetSprite(DT), 1.0f, 1.0f, 1.0f, 1.0f, _player.IsFacingLeft());
    }

    // Coins
    const Sprite& coin_sprite = _coin_anim.Update(DT);
    for (const auto& coin : _coins) {
        if (coin.x < -9000.0f) continue;
        _sprite_batch.Draw(coin.x, coin.y, coin.w, coin.h,
                           coin_sprite, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Enemies
    _enemy_manager.RenderAll(_sprite_batch, DT);

    // Tilemap
    for (int row = 0; row < _tilemap.GetRows(); ++row) {
        for (int col = 0; col < _tilemap.GetCols(); ++col) {
            const auto& tile = _tilemap.GetTile(col, row);
            if (tile.type == TileType::Empty) continue;

            SpriteID sid = SpriteID::BrickTile;
            if (tile.type == TileType::Ground) sid = SpriteID::GroundTile;
            else if (tile.type == TileType::QBlock) sid = SpriteID::QBlockTile0;
            else if (tile.type == TileType::Pipe) sid = SpriteID::PipeTL; // Simplified for now

            int t_size = _tilemap.GetTileSize();
            _sprite_batch.Draw(col * t_size, row * t_size,
                               static_cast<float>(t_size), static_cast<float>(t_size),
                               _sprite_sheet.Get(sid), 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    // Flag
    if (_flag_aabb.x > -9000.0f) {
        _sprite_batch.Draw(_flag_aabb.x, _flag_aabb.y, _flag_aabb.w, _flag_aabb.h,
                           _sprite_sheet.Get(SpriteID::Flag), 1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void Game::RenderHUD() {
    _score_renderer.Draw(_sprite_batch, _score, 10.0f, 10.0f, _camera.GetX(), _camera.GetY());
    _score_renderer.DrawLives(_sprite_batch, _player.GetLives(), 10.0f, 50.0f, _camera.GetX(), _camera.GetY());
}

void Game::RenderFade() {
    if (_fade_alpha > 0.001f) {
        _sprite_batch.Draw(
            _camera.GetX(), _camera.GetY(),
            static_cast<float>(_window.GetWidth()), static_cast<float>(_window.GetHeight()),
            _fade_sprite,
            0.0f, 0.0f, 0.0f, _fade_alpha
        );
    }
}

void Game::RenderCenteredText(const std::string& text, float y, float scale) {
    // Approximate centering: each char ~ 8*scale + gap
    const float char_w = 8.0f * scale;
    const float gap    = 4.0f * (scale / 2.5f);
    const float total  = text.size() * char_w + (text.size() - 1) * gap;
    const float x      = (800.0f - total) / 2.0f;
    _score_renderer.DrawText(_sprite_batch, text, x, y,
                             _camera.GetX(), _camera.GetY(), scale);
}