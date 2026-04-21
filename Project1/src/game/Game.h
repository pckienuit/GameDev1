#pragma once
#include "../core/Window.h"
#include "../core/GameLoop.h"
#include "../core/Input.h"
#include "../core/SoundManager.h"
#include "../renderer/Renderer.h"
#include "../renderer/SpriteBatch.h"
#include "../renderer/Texture.h"
#include "../renderer/TextureRegistry.h"
#include "../renderer/SpriteSheet.h"
#include "../renderer/Camera.h"
#include "../ecs/EntityManager.h"
#include "../tilemap/Tilemap.h"
#include "../collision/CollisionSystem.h"
#include "Player.h"
#include "EnemyManager.h"
#include "../renderer/ScoreRenderer.h"
#include "LevelManager.h"
#include "../collision/AABB.h"
#include "../renderer/Background.h"

// -----------------------------------------------------------------------
// Game State Machine
// -----------------------------------------------------------------------
enum class GameState {
    Title,          // "MARIO GAME" + "PRESS ENTER TO START"
    LevelIntro,     // "WORLD X" countdown
    Playing,        // gameplay
    Dying,          // death animation running
    GameOver,       // "GAME OVER" + "PRESS ENTER TO RETRY"
    LevelComplete,  // win transition (fade out → next level)
    Victory,        // "YOU WIN" + final score
};

class Game {
public:
    Game();

    // Returns false when the window signals quit
    bool Update();
    void Render();

    // Load/reload a level: clears state, loads map, spawns entities
    void LoadLevel(const LevelDef& level);

private:
    static constexpr float DT        = static_cast<float>(GameLoop::FIXED_DT);
    static constexpr int   CELL_SIZE  = 32;
    static constexpr int MAX_EVENTS = 1024;

    Window                _window;
    GameLoop              _game_loop;
    Input                 _input;
    SoundManager          _sound_manager;

    Renderer              _renderer;
    SpriteBatch           _sprite_batch;

    TextureRegistry       _tex_registry;
    SpriteSheet           _sprite_sheet;  // central sprite registry

    ScoreRenderer         _score_renderer;  // reads from _sprite_sheet

    Background            _background;

    LevelManager          _level_manager;

    // Per-level background color (set by LoadLevel)
    float                 _bg_r = 0.40f;
    float                 _bg_g = 0.60f;
    float                 _bg_b = 1.00f;

    int                   _score = 0;
    bool                  _prev_grounded   = true;
    bool                  _prev_hurt       = false;

    // --- State machine ---
    GameState             _state       = GameState::Title;
    float                 _state_timer = 0.0f;
    bool                  _die_sound_played = false;

    EntityManager         _entity_manager;
    Tilemap               _tilemap;

    CollisionSystem       _collision_system;

    Camera                _camera;
    Player                _player;
    EntityID              _dummy_id;
    AABB                  _dummy_aabb;

    EnemyManager          _enemy_manager;

    // Flag (win condition)
    AABB                  _flag_aabb = {-9999.0f, 0.0f, 0.0f, 0.0f};

    // Coins
    std::vector<AABB>     _coins;
    Animation             _coin_anim;

    // Fade overlay
    Texture               _fade_texture;
    Sprite                _fade_sprite;
    float                 _fade_alpha = 1.0f;
    static constexpr float FADE_SPEED = 1.5f;

    // --- Level Timer ---
    static constexpr float LEVEL_TIME = 300.0f;
    float                 _level_timer = LEVEL_TIME;
    bool                  _timer_bonus_shown = false;
    float                 _bonus_display_timer = 0.0f;

    // State-specific update helpers
    void UpdateTitle(float real_dt);
    void UpdateLevelIntro(float real_dt);
    void UpdatePlaying(float real_dt);
    void UpdateTimer(float real_dt);
    void UpdateDying(float real_dt);
    void UpdateGameOver(float real_dt);
    void UpdateLevelComplete(float real_dt);
    void UpdateVictory(float real_dt);

    // State-specific render helpers
    void RenderWorld();
    void RenderHUD();
    void RenderFade();
    void RenderTimer();
    void RenderTimeBonus();
    void RenderCenteredText(const std::string& text, float y, float scale = 3.5f);
};
