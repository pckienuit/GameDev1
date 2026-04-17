#pragma once
#include "../core/Window.h"
#include "../core/GameLoop.h"
#include "../core/Input.h"
#include "../core/SoundManager.h"
#include "../renderer/Renderer.h"
#include "../renderer/SpriteBatch.h"
#include "../renderer/Texture.h"
#include "../renderer/Camera.h"
#include "../ecs/EntityManager.h"
#include "../tilemap/Tilemap.h"
#include "../collision/CollisionSystem.h"
#include "Player.h"
#include "EnemyManager.h"
#include "../renderer/ScoreRenderer.h"
#include "../collision/AABB.h"

class Game {
public:
    Game();

    // Returns false when the window signals quit
    bool Update();
    void Render();

private:
    static constexpr float DT = static_cast<float>(GameLoop::FIXED_DT);
    static constexpr int WORLD_W = 2560;
    static constexpr int WORLD_H = 240;
    static constexpr int CELL_SIZE = 32;
    static constexpr int MAX_EVENTS = 1024;

    Window                _window;
    GameLoop              _game_loop;
    Input                 _input;
    SoundManager          _sound_manager;

    Renderer              _renderer;
    SpriteBatch           _sprite_batch;

    Texture               _brick_texture;
    Texture               _mario_texture;   
    Texture               _misc_texture;     // font + misc sprites
    ScoreRenderer         _score_renderer;   // depends on _misc_texture

    int                   _score = 0;
    bool                  _prev_grounded   = true;
    bool                  _prev_hurt       = false;
    bool                  _prev_game_over  = false;
    float                 _game_over_timer = -1.0f;

    // Win state
    bool                  _is_won          = false;
    float                 _win_timer       = -1.0f;  // countdown before quit

    EntityManager         _entity_manager;
    Tilemap               _tilemap;

    CollisionSystem       _collision_system;

    Camera                _camera;
    Player                _player;
    Sprite                _brick_sprite;
    Sprite                _mario_sprite;
    EntityID              _dummy_id;
    AABB                  _dummy_aabb;

    Texture               _goomba_texture;
    Texture               _koopa_texture;   // TODO: swap to "assets/koopa.png" when available
    EnemyManager          _enemy_manager;

    // Flag (win condition)
    Texture               _flag_texture;
    Sprite                _flag_sprite;
    AABB                  _flag_aabb = {-9999.0f, 0.0f, 0.0f, 0.0f};  // invalid until loaded
};
