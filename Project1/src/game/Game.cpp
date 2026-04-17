#include "Game.h"

Game::Game() : _window("Mario Engine", 800, 600),
                _renderer(_window),
                _sprite_batch(_renderer.GetDevice(), _renderer.GetContext()),
                _brick_texture(_renderer.GetDevice(), "assets/brick.png"),
                _mario_texture(_renderer.GetDevice(), "assets/mario.png"),
                _misc_texture(_renderer.GetDevice(), "assets/misc.png"),
                _score_renderer(&_misc_texture),
                _entity_manager(),
                _tilemap(0, 0, 0),
                _collision_system(WORLD_W, WORLD_H, CELL_SIZE, MAX_EVENTS),
                _camera(800.0f, 600.0f),
                _player(200.0f, 100.0f, &_mario_texture, _entity_manager),
                _brick_sprite(&_brick_texture, 0, 0, _brick_texture.GetWidth(), _brick_texture.GetHeight()),
                _mario_sprite(&_mario_texture, 0, 0, _mario_texture.GetWidth(), _mario_texture.GetHeight()),
                _dummy_id(_entity_manager.Create()),
                _dummy_aabb({ 100.0f, 200.0f, 48.0f, 96.0f }),
                _goomba_texture(_renderer.GetDevice(), "assets/enemies.png"),
                _koopa_texture(_renderer.GetDevice(), "assets/enemies.png"),
                _enemy_manager(_entity_manager),
                _flag_texture(_renderer.GetDevice(), "assets/flag.png", 0x93, 0xBB, 0xEC),
                _flag_sprite(&_flag_texture, 0, 1268, 160, 160)
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

    _tilemap.LoadFromFile("assets/level1.txt");
    _collision_system.Resize(static_cast<int>(_tilemap.GetWidth()), static_cast<int>(_tilemap.GetHeight()), CELL_SIZE);
    for (const SpawnInfo& spawn : _tilemap.GetSpawnPoints()) {
        if (spawn.token == 'G') {
            _enemy_manager.Spawn(EnemyDef::GOOMBA, &_goomba_texture,
                                 static_cast<float>(spawn.x), static_cast<float>(spawn.y));
        }
        if (spawn.token == 'K') {
            _enemy_manager.Spawn(EnemyDef::KOOPA, &_koopa_texture,
                                 static_cast<float>(spawn.x), static_cast<float>(spawn.y));
        }
        if (spawn.token == 'F') {
            constexpr float FLAG_W = 96.0f;
            constexpr float FLAG_H = 96.0f;
            _flag_aabb = { static_cast<float>(spawn.x), static_cast<float>(spawn.y), FLAG_W, FLAG_H };
        }
    }
}

bool Game::Update() {
    if (!_window.ProcessMessages()) return false;

    // Game over delay: play die sound once, then wait before quitting
    const bool game_over_now = _player.IsGameOver();
    if (!_prev_game_over && game_over_now) {
        _sound_manager.Play(SoundID::Die);
        _game_over_timer = 2.5f;  // seconds to show death fall
    }
    _prev_game_over = game_over_now;

    // Win delay: freeze gameplay, wait 2s then quit
    if (_is_won) {
        const float real_dt = static_cast<float>(_game_loop.Tick());
        _win_timer -= real_dt;
        if (_win_timer <= 0.0f) return false;
        return true;  // keep rendering the winning frame
    }

    if (game_over_now) {
        const float real_dt = static_cast<float>(_game_loop.Tick());
        _game_over_timer -= real_dt;
        if (_game_over_timer <= 0.0f) return false;

        // Still run physics so Mario falls (Player::Move handles _game_over free-fall)
        while (_game_loop.ShouldUpdate()) {
            _player.Move(DT, _tilemap);
            _game_loop.ConsumeUpdate();
        }
        return true;
    }

    _game_loop.Tick();
    
    const CollisionEventPool& pool = _collision_system.GetEvents();

    while (_game_loop.ShouldUpdate()) {
        _input.Poll();
        _player.TickInvincibility(DT);
        _player.PrepareVelocity(DT, _input.IsHeld(Action::MoveLeft), _input.IsHeld(Action::MoveRight), _input.IsPressed(Action::Jump), _input.IsHeld(Action::Jump));
        _enemy_manager.Update(DT, _tilemap);
        _collision_system.BeginFrame();
        _collision_system.Register(_player.GetID(), _player.GetAABB(), _player.GetVelX(), _player.GetVelY());
        _collision_system.Register(_dummy_id, _dummy_aabb, 0.0f, 0.0f);  // static dummy
        _enemy_manager.RegisterAll(_collision_system);
        _collision_system.Detect();
        _enemy_manager.HandleCollisions(pool, _player.GetID(), _player);

        // Play stomp sound when score is added
        const int stomped = _enemy_manager.PopScore();
        if (stomped > 0) {
            _score += stomped;
            _sound_manager.Play(SoundID::Stomp);
        }

        for (int i = 0; i < pool.Count(); ++i) {
            const CollisionEvent& ev = pool.Get(i);
            
            if (ev.entity_a == _player.GetID()) {
                _player.ClampVelocityAlongNormal(ev.normal_x, ev.normal_y, ev.hit_time);
            }
            if (ev.entity_b == _player.GetID()) {
                _player.ClampVelocityAlongNormal(-ev.normal_x, -ev.normal_y, ev.hit_time);
            }
        }
        _player.Move(DT, _tilemap);

        // Jump sound: grounded → airborne edge
        const bool grounded_now = _player.IsGrounded();
        if (_prev_grounded && !grounded_now && _input.IsPressed(Action::Jump)) {
            _sound_manager.Play(SoundID::Jump);
        }
        _prev_grounded = grounded_now;

        // Hurt sound: invincibility rising edge (player just got hit)
        const bool hurt_now = _player.IsInvincible();
        if (!_prev_hurt && hurt_now) {
            _sound_manager.Play(SoundID::Hurt);
        }
        _prev_hurt = hurt_now;


        // Win: player touches flag AABB
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
        _sprite_batch.Draw(_dummy_aabb.x, _dummy_aabb.y, _dummy_aabb.w, _dummy_aabb.h, _brick_sprite, 1.0f, 0.2f, 0.2f, 1.0f);
        if (_player.ShouldRender()) {
            _sprite_batch.Draw(_player.GetX(), _player.GetY(), _player.GetW(), _player.GetH(), _player.GetSprite(DT), 1.0f, 1.0f, 1.0f, 1.0f, _player.IsFacingLeft());
        }

        _enemy_manager.RenderAll(_sprite_batch, DT);
        
        for (int row = 0; row < _tilemap.GetRows(); ++row) {
            for (int col = 0; col < _tilemap.GetCols(); ++col) {
                const auto& tile = _tilemap.GetTile(col, row);
                int t_size = _tilemap.GetTileSize();
                int pixel_x = col * t_size;
                int pixel_y = row * t_size;
                
                if (tile.type != TileType::Empty) {
                    _sprite_batch.Draw(pixel_x, pixel_y, (float)t_size, (float)t_size, _brick_sprite, 1.0f, 1.0f, 1.0f, 1.0f);
                }
            }
        }
    _score_renderer.Draw(_sprite_batch, _score, 10.0f, 10.0f, _camera.GetX(), _camera.GetY());
    _score_renderer.DrawLives(_sprite_batch, _player.GetLives(), 10.0f, 50.0f,_camera.GetX(), _camera.GetY());

    // Flag
    if (_flag_aabb.x > -9000.0f) {
        _sprite_batch.Draw(_flag_aabb.x, _flag_aabb.y, _flag_aabb.w, _flag_aabb.h,
                           _flag_sprite, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    _sprite_batch.End();
    _renderer.EndFrame();
}