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

### 📊 Current Progress Snapshot (updated: 2026-04-19)

| Week | Topic | % |
|------|-------|---|
| 1 | D3D11 Foundation | 95% ✅ |
| 2 | Core Engine | 100% ✅ |
| 3 | ECS + Assets | 80% |
| 4 | Collision System | 100% ✅ |
| 5 | Mario Physics | **100% ✅** |
| 6 | Tilemap + Camera | **100% ✅** |
| 7 | Audio + Enemy AI + Mechanics | **100% ✅** |
| 8 | HUD + Score + Polish | **70%** 🔧 |

**Completed (since 2026-04-16):**
- Week 6 DONE: Camera follow (deadzone + lerp), camera clamping, tile collision (solid+one-way)
- Week 7 DONE: XAudio2 (5 sounds), Koopa full state machine (shell/sliding/kick/bounce limit/grace fix)
- Week 8: Score+lives HUD, win condition (flag 'F' token), color-key transparency, coin collectibles ('C' token +10 score)

**Remaining:**
- [ ] Screen transitions (fade in/out)
- [ ] Bug fixing + playtesting
- [ ] Enemy spawn/despawn based on camera (Week 7 backlog)
- [ ] Proper coin sprite (currently placeholder yellow brick)

**Full roadmap:** `.agent/agents/game-mentor.md`
