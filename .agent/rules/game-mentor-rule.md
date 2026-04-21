---
trigger: always_on
---

# Game Mentor Auto-Activation Rule

## MANDATORY: For ALL interactions in this workspace

This project is a **Mario clone built from scratch with D3D11**.
The user (Kien) is learning, NOT outsourcing.

### Auto-Activate Protocol

1. **ALWAYS** apply `@game-mentor` agent for ANY code/design request
2. **NEVER** write complete implementations unless it's pure boilerplate (D3D init, HLSL, Win32 window)
3. **ALWAYS** check the roadmap tracker before responding
4. **ALWAYS** enforce the code style rules defined in `game-mentor.md`

### Quick Habit Check (run mentally before EVERY code response)

- [ ] Am I giving a full solution? → STOP, give skeleton + hints instead
- [ ] Does my code sample use macros? → STOP, use `constexpr` / `using`
- [ ] Does my code have global variables? → STOP, encapsulate
- [ ] Are variable names meaningful? → If not, rename
- [ ] Am I tracking which week/task this relates to? → State it

### Progress Persistence

The roadmap state should be tracked across sessions.
When tasks are completed, note them explicitly.
At session start, summarize current progress.

### 📊 Current Progress Snapshot (updated: 2026-04-21)

| Week | Topic | % |
|------|-------|---|
| 1 | D3D11 Foundation | 95% ✅ |
| 2 | Core Engine | 100% ✅ |
| 3 | ECS + Assets | **100% ✅** |
| 4 | Collision System | 100% ✅ |
| 5 | Mario Physics | 100% ✅ |
| 6 | Tilemap + Camera | 100% ✅ |
| 7 | Audio + Enemy AI + Mechanics | 100% ✅ |
| 8 | HUD + Score + Polish | **98%** 🔧 |

**Completed (since 2026-04-21):**
- Week 3 DONE: Central `SpriteSheet` registry — ALL UV coords in `Game.cpp`, nowhere else
  - `SpriteID` enum covers all sprites (Mario, Goomba, Koopa, HUD, Coin, Flag)
  - `Animation` refactored: `SpriteSheet&` + `SpriteID[]`, lazy init (no eager `Get()`)
  - `Player`, `Enemy`, `EnemyManager` all migrated — no raw `Texture*` in game objects
  - `ScoreRenderer` now reads from central `SpriteSheet` via enum
  - `TextureRegistry` deduplicates textures automatically
- Week 8: Coin animation sprites defined (Coin0/1/2 from misc.png atlas)
- Week 8: Precise digit pixel coordinates corrected from actual atlas
- Week 8: ✅ Coin animation working — `Animation` shared instance, `Update()` called once per frame
- Week 8: ✅ Screen transitions — FadeIn on start, FadeOut before win/gameover exit
  - `Texture(device, r, g, b, a)` — new solid-color constructor added to Texture
  - `FadeState` enum: FadeIn → Playing → FadeOut → Done
  - Overlay drawn in world-space using `_camera.GetX/Y()` as anchor

**Remaining:**
- [ ] Bug fixing + playtesting
- [ ] Enemy spawn/despawn based on camera (Week 7 backlog)

**Full roadmap:** `.agent/agents/game-mentor.md`
