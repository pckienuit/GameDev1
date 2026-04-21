#include "Game.h"
#include <string>

Game::Game() : _window("Mario Engine", 800, 600),
                _renderer(_window),
                _sprite_batch(_renderer.GetDevice(), _renderer.GetContext()),
                _sprite_sheet(_tex_registry, _renderer.GetDevice()),
                _score_renderer(_sprite_sheet),
                _entity_manager(),
                _tilemap(0, 0, 0),
                _collision_system(WORLD_W, WORLD_H, CELL_SIZE, MAX_EVENTS),
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
    _sprite_sheet.Define(SpriteID::BrickTile, "assets/brick.png", 0, 0, 16, 16);

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
    _sprite_sheet.Define(SpriteID::Digit8, digit_texture_path, 523, 239, 8, 14); //digit 8
    _sprite_sheet.Define(SpriteID::Digit9, digit_texture_path, 532, 239, 8, 14); //digit 9

    _sprite_sheet.Define(SpriteID::Heart, "assets/misc.png", 596, 192, 10, 10);

    // -----------------------------------------------------------------------

    _coin_anim = Animation(_sprite_sheet, {SpriteID::Coin0, 
                                          SpriteID::Coin1,
                                          SpriteID::Coin2}, 0.3f, true);
    

    _tilemap.LoadFromFile("assets/level1.txt");
    _collision_system.Resize(static_cast<int>(_tilemap.GetWidth()),
                             static_cast<int>(_tilemap.GetHeight()), CELL_SIZE);

    for (const SpawnInfo& spawn : _tilemap.GetSpawnPoints()) {
        if (spawn.token == 'G') {
            _enemy_manager.Spawn(EnemyDef::GOOMBA, _sprite_sheet,
                                 static_cast<float>(spawn.x), static_cast<float>(spawn.y));
        }
        if (spawn.token == 'K') {
            _enemy_manager.Spawn(EnemyDef::KOOPA, _sprite_sheet,
                                 static_cast<float>(spawn.x), static_cast<float>(spawn.y));
        }
        if (spawn.token == 'F') {
            constexpr float FLAG_W = 96.0f;
            constexpr float FLAG_H = 96.0f;
            _flag_aabb = { static_cast<float>(spawn.x), static_cast<float>(spawn.y), FLAG_W, FLAG_H };
        }
        if (spawn.token == 'C') {
            constexpr float COIN_W = 32.0f;
            constexpr float COIN_H = 32.0f;
            _coins.push_back({ static_cast<float>(spawn.x), static_cast<float>(spawn.y), COIN_W, COIN_H });
        }
    }
}

bool Game::Update() {
    if (!_window.ProcessMessages()) return false;
    const float real_dt = static_cast<float>(_game_loop.Tick());

    if (_fade_state == FadeState::FadeIn) {
        _fade_alpha -= FADE_SPEED * real_dt;
        if (_fade_alpha <= 0.0f) {
            _fade_alpha  = 0.0f;
            _fade_state  = FadeState::Playing;
        }
    }
    if (_fade_state == FadeState::FadeOut) {
        _fade_alpha += FADE_SPEED * real_dt;
        if (_fade_alpha >= 1.0f) {
            _fade_alpha = 1.0f;
            _fade_state = FadeState::Done;
        }
    }

    // Game over delay: play die sound once, then wait before quitting
    const bool game_over_now = _player.IsGameOver();
    if (!_prev_game_over && game_over_now) {
        _sound_manager.Play(SoundID::Die);
        _game_over_timer = 2.5f;  // seconds to show death fall
    }
    _prev_game_over = game_over_now;

    // Win delay: freeze gameplay, wait 2s then quit
    if (_is_won) {
        _win_timer -= real_dt;
        if (_win_timer <= 0.0f) {
            if (_fade_state != FadeState::FadeOut && _fade_state != FadeState::Done)
                _fade_state = FadeState::FadeOut;
            if (_fade_state == FadeState::Done) return false;
        }
        return true;  // keep rendering the winning frame
    }

    if (game_over_now) {
        _game_over_timer -= real_dt;
        if (_game_over_timer <= 0.0f) {
            if (_fade_state != FadeState::FadeOut && _fade_state != FadeState::Done)
                _fade_state = FadeState::FadeOut;
            if (_fade_state == FadeState::Done) return false;
        }

        // Still run physics so Mario falls
        while (_game_loop.ShouldUpdate()) {
            _player.Move(DT, _tilemap);
            _game_loop.ConsumeUpdate();
        }
        return true;
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
        _enemy_manager.HandleCollisions(pool, _player.GetID(), _player);

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
        if (!_is_won && AABB::Overlaps(_player.GetAABB(), _flag_aabb)) {
            _is_won    = true;
            _win_timer = 2.0f;
        }

        _camera.Follow(_player.GetX(), _player.GetY(), DT);
        _camera.Clamp(_tilemap.GetWidth(), _tilemap.GetHeight());
        _game_loop.ConsumeUpdate();
    }
    return true;
}

void Game::Render() {
    _renderer.BeginFrame(0.1f, 0.1f, 0.2f);
    _sprite_batch.Begin(800.0f, 600.0f, _camera.GetX(), _camera.GetY());

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
                if (tile.type != TileType::Empty) {
                    int t_size  = _tilemap.GetTileSize();
                    _sprite_batch.Draw(col * t_size, row * t_size,
                                       static_cast<float>(t_size), static_cast<float>(t_size),
                                       _sprite_sheet.Get(SpriteID::BrickTile), 1.0f, 1.0f, 1.0f, 1.0f);
                }
            }
        }

    // HUD (screen-space — drawn after world sprites)
    _score_renderer.Draw(_sprite_batch, _score, 10.0f, 10.0f, _camera.GetX(), _camera.GetY());
    _score_renderer.DrawLives(_sprite_batch, _player.GetLives(), 10.0f, 50.0f, _camera.GetX(), _camera.GetY());

    // Flag
    if (_flag_aabb.x > -9000.0f) {
        _sprite_batch.Draw(_flag_aabb.x, _flag_aabb.y, _flag_aabb.w, _flag_aabb.h,
                           _sprite_sheet.Get(SpriteID::Flag), 1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (_fade_alpha > 0.001f) {
        _sprite_batch.Draw(
            _camera.GetX(), _camera.GetY(),
            _window.GetWidth(), _window.GetHeight(),
            _fade_sprite,
            0.0f, 0.0f, 0.0f, _fade_alpha
        );
    }

    _sprite_batch.End();
    _renderer.EndFrame();
}