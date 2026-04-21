# Kế Hoạch Hoàn Thiện Game Mario Platformer — 3 Levels

## Hiện Trạng Codebase

| Hệ thống | Trạng thái | Chi tiết |
|----------|:----------:|---------|
| Engine (Window, D3D11, SpriteBatch, GameLoop) | ✅ | Fixed timestep 50Hz, DirectX 11 |
| SpriteSheet registry (enum `SpriteID`) | ✅ | Multi-texture, O(1) lookup |
| Tilemap (load from .txt, 48px tiles) | ✅ | Ground, Brick, OneWay, spawn tokens |
| Player controller | ✅ | Accel-based, coyote time, jump buffer, variable jump |
| Enemy system (Goomba, Koopa) | ✅ | Shell → Slide → Bounce, edge turning |
| Collision (AABB sweep, SpatialGrid) | ✅ | CollisionEvent pool |
| HUD (Score digits, Heart lives) | ✅ | Bitmap digit rendering |
| Audio (XAudio2) | ✅ | jump, stomp, hurt, die, coin |
| Camera (dead zone + lerp) | ✅ | Exponential decay follow |
| Objects (Coins animated, Flag win) | ✅ | Animated coins, flag = win condition |
| Game flow (Fade in/out) | ✅ | Fade transitions, game over delay |
| **Multi-level support** | ❌ | Chỉ có 1 level, win → quit |
| **Game screens (Title, Game Over)** | ❌ | Không có |
| **Visual variety (backgrounds, tile types)** | ❌ | Chỉ 1 loại tile |

---

## Tổng Quan 6 Phases — Sắp Xếp Theo Thứ Tự Thực Hiện

> [!TIP]
> Phases sắp xếp theo **dependency order** (phase sau phụ thuộc phase trước). Mức độ phức tạp được đánh giá từ ★☆☆☆☆ (đơn giản) đến ★★★★★ (rất phức tạp).

| Phase | Nội dung | Complexity | Model Đề Xuất | Lý Do Chọn Model |
|:-----:|----------|:----------:|:--------------:|-------------------|
| **1** | Level Management System | ★★★☆☆ | **Claude Sonnet 4.6** | Refactoring cẩn thận: extract logic ra class mới, đảm bảo init order. Sonnet đủ sức, nhanh hơn Opus |
| **2** | Extended Tile Types & Sprites | ★☆☆☆☆ | **Gemini 3 Flash** | Boilerplate: thêm enum values, placeholder defines, update switch. Flash nhanh & rẻ |
| **3** | Background & Parallax | ★★☆☆☆ | **Gemini 3.1 Pro (Low)** | Class mới nhưng logic đơn giản (tiling + parallax math). Pro Low cho chất lượng tốt, cost thấp |
| **4** | New Enemy Types | ★★★★☆ | **Gemini 3.1 Pro (High)** | Logic phức tạp: Piranha oscillation, FlyKoopa wing mechanic, sửa state machine. Cần High effort |
| **5** | UI Screens & Game State Machine | ★★★★★ | **Claude Opus 4.6** | Phức tạp nhất: rewrite core Game loop thành state machine, 7 states, bitmap font. Opus cho accuracy |
| **6** | Final Polish & Balance | ★★☆☆☆ | **GPT OSS 120B (Medium)** | Nhiều thay đổi nhỏ rải rác: timer, death pit, balance tweaks. GPT OSS tốt cho batch edits |

---

## Phase 1: Level Management System ★★★☆☆
**Model: Claude Sonnet 4.6** — Refactoring extraction, đảm bảo init order đúng

### Mục tiêu
Game có thể load bất kỳ level nào từ file, và chuyển sang level tiếp theo khi thắng.

### Files mới

#### [NEW] [LevelDef.h](file:///d:/GameDev1/Project1/src/game/LevelDef.h)

```cpp
#pragma once
#include <string>

struct LevelDef {
    std::string map_file;        // "assets/level1.txt"
    float player_start_x;
    float player_start_y;
    float bg_r, bg_g, bg_b;     // background color per level
};
```

#### [NEW] [LevelManager.h](file:///d:/GameDev1/Project1/src/game/LevelManager.h) + `.cpp`

```cpp
class LevelManager {
public:
    LevelManager();
    const LevelDef& GetCurrent() const;
    bool HasNextLevel() const;
    void NextLevel();
    void Reset();           // quay về level 1
    int  GetLevelIndex() const;  // 0-based
    int  GetTotalLevels() const;
private:
    std::vector<LevelDef> _levels;
    int _current = 0;
};
```

Constructor define 3 levels:
```cpp
_levels = {
    { "assets/level1.txt", 200.0f, 100.0f, 0.40f, 0.60f, 1.00f },  // Blue sky
    { "assets/level2.txt", 100.0f, 100.0f, 0.15f, 0.10f, 0.25f },  // Night
    { "assets/level3.txt", 100.0f, 100.0f, 0.85f, 0.45f, 0.20f },  // Sunset
};
```

#### [NEW] [level2.txt](file:///d:/GameDev1/Project1/assets/level2.txt) — 60×15 grid

Wider map, more platforms, more gaps, more enemies. (Map data trong file riêng)

#### [NEW] [level3.txt](file:///d:/GameDev1/Project1/assets/level3.txt) — 70×15 grid

Hardest level, many gaps, vertical sections, dense enemies. (Map data trong file riêng)

### Files sửa

#### [MODIFY] [Game.h](file:///d:/GameDev1/Project1/src/game/Game.h)

- `#include "LevelManager.h"`
- Thêm member: `LevelManager _level_manager`
- Thêm method: `void LoadLevel(const LevelDef& level)`
- Bỏ `constexpr WORLD_W`, `WORLD_H` — tính từ tilemap

#### [MODIFY] [Game.cpp](file:///d:/GameDev1/Project1/src/game/Game.cpp)

- **Extract** logic load level (lines 89–112) ra `LoadLevel()`
- Constructor gọi `LoadLevel(_level_manager.GetCurrent())`
- `LoadLevel()` cần: clear enemies, clear coins, clear flag, reload tilemap, resize collision, re-spawn enemies + objects
- Win logic: thay vì quit → `_level_manager.NextLevel()` + `LoadLevel()`
- Nếu `!HasNextLevel()` → quit (tạm thời, Phase 5 sẽ show Victory screen)
- **Player reset**: giữ `_score` và `_lives`, reset position

> [!WARNING]
> **Init order quan trọng:** `_player` phụ thuộc `_sprite_sheet`, `_enemy_manager` phụ thuộc `_entity_manager`. Khi `LoadLevel()`, phải reset player position TRƯỚC khi spawn enemies.

### 🧪 Test Manual — Phase 1

| # | Bước | Expected |
|---|------|----------|
| 1 | Build & Run | Game khởi động, level 1 y hệt hiện tại |
| 2 | Chạy đến Flag level 1 | Fade out → Level 2 load → Fade in |
| 3 | Kiểm tra level 2 | Map layout khác, nhiều enemy hơn |
| 4 | Background color | Level 1 xanh, Level 2 tối, Level 3 cam |
| 5 | Score khi chuyển level | Score giữ nguyên (không reset) |
| 6 | Lives khi chuyển level | Lives giữ nguyên |
| 7 | Game Over ở level 2 | Fade out → quit (tạm) |
| 8 | Clear level 3 | Quit (tạm — Phase 5 sẽ show Victory) |
| 9 | Player position | Spawn ở vị trí đúng mỗi level |  
| 10 | Enemies mới | Enemies spawn đúng vị trí map mới |

---

## Phase 2: Extended Tile Types & Sprite Placeholders ★☆☆☆☆
**Model: Gemini 3 Flash** — Boilerplate additions, thêm enums và placeholder defines

### Mục tiêu
Thêm visual variety cho mỗi level bằng tile types mới + sprite definitions.

### Files sửa

#### [MODIFY] [Tilemap.h](file:///d:/GameDev1/Project1/src/tilemap/Tilemap.h)

```cpp
enum class TileType : uint8_t {
    Empty   = 0,
    Ground  = 1,
    Brick   = 2,
    OneWay  = 3,
    QBlock  = 4,   // Question block (solid)
    Pipe    = 5,   // Pipe segment (solid)
};
```

#### [MODIFY] [Tilemap.cpp](file:///d:/GameDev1/Project1/src/tilemap/Tilemap.cpp)

Cập nhật `IsSolid()` để include types mới.

#### [MODIFY] [SpriteID.h](file:///d:/GameDev1/Project1/src/renderer/SpriteID.h)

Thêm:
```cpp
// Tiles
GroundTile,
QBlockTile0, QBlockTile1,
PipeTL, PipeTR, PipeL, PipeR,
// Decorations
Cloud, Bush, Hill,
```

#### [MODIFY] [Game.cpp](file:///d:/GameDev1/Project1/src/game/Game.cpp)

Thêm placeholder sprite definitions:
```cpp
// === PLACEHOLDER — USER WILL MAP COORDS ===
_sprite_sheet.Define(SpriteID::GroundTile,  "assets/misc.png", 0, 0, 16, 16);  // PLACEHOLDER
_sprite_sheet.Define(SpriteID::QBlockTile0, "assets/misc.png", 0, 0, 16, 16);  // PLACEHOLDER
// ... etc
```

Cập nhật render loop: chọn sprite theo `TileType`.

### 🧪 Test Manual — Phase 2

| # | Bước | Expected |
|---|------|----------|
| 1 | Build | Compile thành công, không crash |
| 2 | Level 1 tiles | Ground tiles render (placeholder OK) |
| 3 | QBlock collision | Player đứng vững trên QBlock (solid) |
| 4 | Pipe collision | Player đứng vững trên Pipe (solid) |
| 5 | No assert crash | Tất cả placeholder sprite render — không assert |

---

## Phase 3: Background & Parallax Layers ★★☆☆☆
**Model: Gemini 3.1 Pro (Low)** — Class mới nhưng math đơn giản

### Mục tiêu
Background layers mỗi level khác nhau, scroll parallax.

### Files mới

#### [NEW] [Background.h](file:///d:/GameDev1/Project1/src/renderer/Background.h) + `.cpp`

```cpp
struct BgLayer {
    SpriteID sprite;
    float    parallax_factor;  // 0.0 = static, 1.0 = full camera movement
    float    y_offset;
    float    tile_width;
};

class Background {
public:
    void Clear();
    void AddLayer(SpriteID sprite, float parallax, float y_offset, float tile_w);
    void Render(SpriteBatch& batch, const SpriteSheet& sheet,
                float cam_x, float cam_y, float screen_w, float screen_h);
private:
    std::vector<BgLayer> _layers;
};
```

### Files sửa

#### [MODIFY] [SpriteID.h](file:///d:/GameDev1/Project1/src/renderer/SpriteID.h)

```cpp
BgMountain, BgClouds, BgTrees, BgCastle, BgStars,
```

#### [MODIFY] [Game.h](file:///d:/GameDev1/Project1/src/game/Game.h) + [Game.cpp](file:///d:/GameDev1/Project1/src/game/Game.cpp)

- Thêm `Background _background`
- Trong `LoadLevel()`: clear + add layers theo level
- Render background **trước** tilemap

### 🧪 Test Manual — Phase 3

| # | Bước | Expected |
|---|------|----------|
| 1 | Level 1 background | Mountains + clouds visible |
| 2 | Parallax | Background gần scroll nhanh hơn background xa |
| 3 | Level chuyển | Background thay đổi khi sang level 2, 3 |
| 4 | FPS | Vẫn ≥ 50 FPS |

---

## Phase 4: New Enemy Types ★★★★☆
**Model: Gemini 3.1 Pro (High)** — Logic phức tạp, state machine mới, đòi hỏi reasoning cao

### Mục tiêu
Thêm Piranha Plant (pipe enemy) và Flying Koopa cho level 2 & 3.

### Files sửa

#### [MODIFY] [SpriteID.h](file:///d:/GameDev1/Project1/src/renderer/SpriteID.h)

```cpp
PiranhaUp0, PiranhaUp1,
FlyKoopaWalk0, FlyKoopaWalk1, FlyKoopaWing,
```

#### [MODIFY] [Enemy.h](file:///d:/GameDev1/Project1/src/game/Enemy.h)

- Thêm `EnemyState::Rising` cho Piranha (up/down cycle)
- Thêm fields: `float origin_y`, `float oscillation_amp`, `bool unstomp-able`
- Thêm presets: `EnemyDef::PIRANHA`, `EnemyDef::FLY_KOOPA`

#### [MODIFY] [Enemy.cpp](file:///d:/GameDev1/Project1/src/game/Enemy.cpp)

Define new presets:
```cpp
const EnemyDef EnemyDef::PIRANHA = {
    48.0f, 64.0f,                    // w, h
    0.0f,                            // patrol_speed (stationary)  
    0.5f,                            // dead_duration
    0.0f,                            // gravity (none — controlled by oscillation)
    false,                           // turns_at_edges
    false,                           // has_shell
    // ... walk_frames = { PiranhaUp0, PiranhaUp1 }
};
```

#### [MODIFY] [EnemyManager.cpp](file:///d:/GameDev1/Project1/src/game/EnemyManager.cpp)

**Piranha logic** (trong `Update()`):
- Oscillation lên/xuống theo sin wave hoặc timer
- Không bị gravity
- `origin_y` lưu vị trí gốc (pipe top)

**FlyKoopa logic**:
- Giống Koopa + vertical oscillation (sin wave)
- Khi bị stomp: mất cánh → become normal Koopa (`patrol_speed`, `has_shell=true`, `gravity=1200`)

**HandleCollisions** update:
- Piranha: luôn gây damage khi chạm (không stomp-able)
- FlyKoopa stomp → downgrade to Koopa

#### [MODIFY] [Game.cpp](file:///d:/GameDev1/Project1/src/game/Game.cpp)

Thêm placeholder sprite definitions + spawn tokens `'P'`, `'W'`.
Update level2.txt, level3.txt với `P` và `W` tokens.

### 🧪 Test Manual — Phase 4

| # | Bước | Expected |
|---|------|----------|
| 1 | Level 2: Piranha | Xuất hiện, di chuyển lên/xuống |
| 2 | Chạm Piranha | Player bị hurt |
| 3 | Stomp Piranha | KHÔNG thể stomp — vẫn bị hurt |
| 4 | Level 3: FlyKoopa | Bay lên/xuống + di chuyển ngang |
| 5 | Stomp FlyKoopa | Mất cánh → trở thành Koopa bình thường |
| 6 | Stomp Koopa (ex-FlyKoopa) | Shell mechanic hoạt động đúng |
| 7 | Collision register | Enemy mới register vào CollisionSystem |
| 8 | Screen exit | Enemy ngoài camera bị deactivate |

---

## Phase 5: UI Screens & Game State Machine ★★★★★
**Model: Claude Opus 4.6** — Phức tạp nhất: rewrite core loop, 7 game states, bitmap font

### Mục tiêu
Title screen, level intros, game over/retry, victory screen + bitmap font A-Z.

### Files sửa

#### [MODIFY] [SpriteID.h](file:///d:/GameDev1/Project1/src/renderer/SpriteID.h)

Thêm bitmap font:
```cpp
LetterA, LetterB, LetterC, ... LetterZ,
```

#### [MODIFY] [ScoreRenderer.h](file:///d:/GameDev1/Project1/src/renderer/ScoreRenderer.h) + `.cpp`

Thêm `DrawText()`:
```cpp
void DrawText(SpriteBatch& batch, const std::string& text,
              float screen_x, float screen_y,
              float cam_x, float cam_y, float scale = 2.5f) const;
```

Logic: A-Z → `SpriteID::LetterA + (ch - 'A')`, digits → existing, space → skip.

#### [MODIFY] [Game.h](file:///d:/GameDev1/Project1/src/game/Game.h)

State machine:
```cpp
enum class GameState {
    Title,          // "PRESS ENTER TO START"
    LevelIntro,     // "WORLD 1" countdown
    Playing,        // gameplay
    Dying,          // death animation đang chạy
    GameOver,       // "GAME OVER — PRESS ENTER TO RETRY"
    LevelComplete,  // win transition
    Victory,        // "YOU WIN" + final score
};

GameState _state = GameState::Title;
float     _state_timer = 0.0f;
```

#### [MODIFY] [Game.cpp](file:///d:/GameDev1/Project1/src/game/Game.cpp)

**Full refactor** `Update()` và `Render()` thành state machine:

| State | Update Logic | Render | Transition |
|-------|-------------|--------|------------|
| **Title** | Wait for Enter | "MARIO GAME" + "PRESS ENTER" | Enter → LevelIntro |
| **LevelIntro** | Timer 2s countdown | "WORLD X" centered | Timer → Playing |
| **Playing** | Logic hiện tại | Gameplay render hiện tại | Win → LevelComplete, Die → Dying |
| **Dying** | Death physics + timer | Death animation | lives>0 → LevelIntro (same level), lives≤0 → GameOver |
| **GameOver** | Wait for Enter | "GAME OVER" + "PRESS ENTER" | Enter → Title (full reset) |
| **LevelComplete** | Fade timer | Fade out → load next | next exists → LevelIntro, else → Victory |
| **Victory** | Wait for Enter | "YOU WIN" + final score | Enter → quit |

> [!CAUTION]
> **Đây là phase phức tạp nhất.** Game::Update() hiện tại mix gameplay + win/gameover logic thành 1 function duy nhất. Cần tách rõ ràng thành state handlers riêng biệt. Sai 1 transition = game stuck.

### 🧪 Test Manual — Phase 5

| # | Bước | Expected |
|---|------|----------|
| 1 | Launch | Title screen: "MARIO GAME" + "PRESS ENTER" |
| 2 | Nhấn Enter | → "WORLD 1" hiện 2 giây |
| 3 | Chờ 2s | → Gameplay bắt đầu |
| 4 | Chơi bình thường | Như cũ |
| 5 | Die (còn lives) | → Replay cùng level, lives -1, score giữ |
| 6 | Die (hết lives) | → "GAME OVER" screen |
| 7 | Enter ở Game Over | → Title, score=0, lives=9, level=1 |
| 8 | Clear level 1 | → Fade → "WORLD 2" |
| 9 | Clear tất cả 3 levels | → "YOU WIN" + final score |
| 10 | Enter ở Victory | Quit game |
| 11 | Text rendering | A-Z render đúng (placeholder texture) |

---

## Phase 6: Final Polish & Balance ★★☆☆☆
**Model: GPT OSS 120B (Medium)** — Nhiều edits nhỏ rải rác, batch changes

### Mục tiêu
Timer, death pit, difficulty balance, minor polish.

### Files sửa

#### [MODIFY] [Game.h](file:///d:/GameDev1/Project1/src/game/Game.h) + [Game.cpp](file:///d:/GameDev1/Project1/src/game/Game.cpp)

**Timer per level:**
```cpp
float _level_timer = 300.0f;  // 5 phút
```
Hết giờ → die. Clear level → time bonus: `_score += (int)_level_timer * 10`.

**Death pit:**
```cpp
if (_player.GetY() > _tilemap.GetHeight() + 200.0f) {
    // Player fell off — instant death
    _player.Hurt(); // or force game over
}
```

#### [MODIFY] [ScoreRenderer.h](file:///d:/GameDev1/Project1/src/renderer/ScoreRenderer.h) + `.cpp`

Thêm `DrawTimer()` — hiển thị timer ở góc phải HUD.

#### [MODIFY] Level data files

Cân bằng difficulty:
- Level 1: ít gap, ít enemy, nhiều coin → Easy
- Level 2: gap trung bình, thêm Piranha → Medium
- Level 3: nhiều gap, nhiều enemy types → Hard

### 🧪 Test Manual — Phase 6

| # | Bước | Expected |
|---|------|----------|
| 1 | Timer visible | Góc phải HUD hiện countdown |
| 2 | Hết giờ | Player die |
| 3 | Clear level | Time bonus cộng vào score |
| 4 | Rơi xuống vực | Player die ngay |
| 5 | Level 1 | Easy — clear mà không mất lives |
| 6 | Level 3 | Hard — mất ít nhất 1-2 lives |
| 7 | Full playthrough | Title → Win: tất cả flow smooth |
| 8 | Game Over → Retry | Full reset hoạt động đúng |

---

## Tổng Hợp — Decision Matrix

### Model Assignment Summary

```
┌────────────────────────────────────────────────────────────────────────┐
│  Phase    Complexity   Model                     Estimated Effort     │
│  ─────   ──────────   ────────────────────────   ──────────────────   │
│    1      ★★★☆☆      Claude Sonnet 4.6          ~1 session           │
│    2      ★☆☆☆☆      Gemini 3 Flash             ~1 session (fast)    │
│    3      ★★☆☆☆      Gemini 3.1 Pro (Low)       ~1 session           │
│    4      ★★★★☆      Gemini 3.1 Pro (High)      ~1–2 sessions        │
│    5      ★★★★★      Claude Opus 4.6            ~2 sessions          │
│    6      ★★☆☆☆      GPT OSS 120B (Medium)      ~1 session           │
└────────────────────────────────────────────────────────────────────────┘
```

### Tại Sao Chọn Model Này?

| Model | Điểm mạnh | Phase phù hợp |
|-------|-----------|---------------|
| **Gemini 3 Flash** | Nhanh, rẻ, tốt cho boilerplate | Phase 2: thêm enum, defines |
| **Gemini 3.1 Pro (Low)** | Chất lượng tốt, cost thấp, đủ cho logic đơn giản | Phase 3: parallax class mới |
| **Gemini 3.1 Pro (High)** | Reasoning mạnh, xử lý state machine phức tạp | Phase 4: enemy behaviors mới |
| **Claude Sonnet 4.6** | Cẩn thận với refactoring, hiểu init order | Phase 1: extract + restructure |
| **Claude Opus 4.6** | Accuracy cao nhất cho architectural changes | Phase 5: core loop rewrite |
| **GPT OSS 120B** | Tốt cho batch small edits rải rác | Phase 6: scattered polish changes |

### Files Tổng Hợp

| File | Ph1 | Ph2 | Ph3 | Ph4 | Ph5 | Ph6 |
|------|:---:|:---:|:---:|:---:|:---:|:---:|
| `Game.h` | ✏️ | | ✏️ | | ✏️ | ✏️ |
| `Game.cpp` | ✏️ | ✏️ | ✏️ | ✏️ | ✏️ | ✏️ |
| `SpriteID.h` | | ✏️ | ✏️ | ✏️ | ✏️ | |
| `Tilemap.h` | | ✏️ | | | | |
| `Tilemap.cpp` | | ✏️ | | | | |
| `Enemy.h` | | | | ✏️ | | |
| `Enemy.cpp` | | | | ✏️ | | |
| `EnemyManager.cpp` | | | | ✏️ | | |
| `ScoreRenderer.h` | | | | | ✏️ | ✏️ |
| `ScoreRenderer.cpp` | | | | | ✏️ | ✏️ |
| `LevelDef.h` | 🆕 | | | | | |
| `LevelManager.h/.cpp` | 🆕 | | | | | |
| `level2.txt` | 🆕 | | | ✏️ | | ✏️ |
| `level3.txt` | 🆕 | | | ✏️ | | ✏️ |
| `Background.h/.cpp` | | | 🆕 | | | |

---

## Open Questions

> [!IMPORTANT]
> **Texture files cho level 2 & 3:** Dùng chung `misc.png` + `enemies.png` hay cần thêm texture files mới (vd: `underground.png`, `castle.png`)?

> [!IMPORTANT]
> **Sound effects mới:** Cần thêm SFX cho Piranha, FlyKoopa, level complete, victory? Hay giữ sound set hiện tại?

> [!IMPORTANT]
> **Death pits:** Level 2 & 3 có nên có vực không đáy (gap trong ground row) không? Level 1 hiện tại có tường bao kín.
