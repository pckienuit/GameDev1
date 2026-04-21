#include "EnemyManager.h"

EnemyManager::EnemyManager(EntityManager& em)
    : _em(em)
{
    _enemies.resize(128);
}

int EnemyManager::FindFreeSlot() const {
    for (int i = 0; i < (int)_enemies.size(); ++i) {
        if (!_enemies[i].active) return i;
    }
    return -1;
}

void EnemyManager::MoveEnemy(Enemy& e, float dt, const Tilemap& tilemap) {
    constexpr float P = 1.0f;
    const float W = e.def->w;
    const float H = e.def->h;

    // === X-AXIS ===
    e.pos_x += e.vel_x * dt;
    if (e.vel_x > 0.0f) {
        int col   = tilemap.PixelToCol(e.pos_x + W);
        int row_t = tilemap.PixelToRow(e.pos_y + P);
        int row_b = tilemap.PixelToRow(e.pos_y + H - P);
        if (tilemap.IsSolid(col, row_t) || tilemap.IsSolid(col, row_b)) {
            e.pos_x = col * tilemap.GetTileSize() - W;
            e.vel_x = -e.vel_x;
            e.facing_left = true;
            if (e.state == EnemyState::Sliding) {
                if (++e.slide_bounce_count >= e.def->max_slide_bounces) {
                    e.state       = EnemyState::Shell;
                    e.vel_x       = 0.0f;
                    e.shell_timer = e.def->shell_wait_time;
                }
            }
        }
    } else if (e.vel_x < 0.0f) {
        int col   = tilemap.PixelToCol(e.pos_x);
        int row_t = tilemap.PixelToRow(e.pos_y + P);
        int row_b = tilemap.PixelToRow(e.pos_y + H - P);
        if (tilemap.IsSolid(col, row_t) || tilemap.IsSolid(col, row_b)) {
            e.pos_x = (col + 1) * tilemap.GetTileSize();
            e.vel_x = -e.vel_x;
            e.facing_left = false;
            if (e.state == EnemyState::Sliding) {
                if (++e.slide_bounce_count >= e.def->max_slide_bounces) {
                    e.state       = EnemyState::Shell;
                    e.vel_x       = 0.0f;
                    e.shell_timer = e.def->shell_wait_time;
                }
            }
        }
    }

    // === Y-AXIS ===
    e.pos_y += e.vel_y * dt;
    if (e.vel_y > 0.0f) {
        int tile_row  = tilemap.PixelToRow(e.pos_y + H);
        int col_left  = tilemap.PixelToCol(e.pos_x + P);
        int col_mid   = tilemap.PixelToCol(e.pos_x + W * 0.5f);
        int col_right = tilemap.PixelToCol(e.pos_x + W - P);
        if (tilemap.IsBlockingFall(col_left,  tile_row) ||
            tilemap.IsBlockingFall(col_right, tile_row) ||
            tilemap.IsBlockingFall(col_mid,   tile_row)) {
            e.pos_y = tile_row * tilemap.GetTileSize() - H;
            e.vel_y = 0.0f;
        }
    }
}

bool EnemyManager::IsEdgeAhead(const Enemy& e, const Tilemap& tilemap) const {
    const float W = e.def->w;
    const float H = e.def->h;
    const int ground_row  = tilemap.PixelToRow(e.pos_y + H);
    const int left_col    = tilemap.PixelToCol(e.pos_x + 1.0f);
    const int right_col   = tilemap.PixelToCol(e.pos_x + W - 1.0f);

    // Grounded if AT LEAST ONE foot is on solid ground
    const bool grounded = tilemap.IsBlockingFall(left_col,  ground_row) ||
                          tilemap.IsBlockingFall(right_col, ground_row);
    if (!grounded) return false;

    // Edge: the LEADING foot's ground tile is empty
    const int leading_col = (e.vel_x > 0) ? right_col : left_col;
    return !tilemap.IsBlockingFall(leading_col, ground_row);
}

void EnemyManager::Spawn(const EnemyDef& def, const SpriteSheet& sheet, float x, float y) {
    int idx = FindFreeSlot();
    if (idx == -1) return;

    _enemies[idx] = Enemy(sheet, def, _em);

    Enemy& e    = _enemies[idx];
    e.active    = true;
    e.state     = EnemyState::Patrol;
    e.pos_x     = x;
    e.pos_y     = y;
    e.origin_y  = y;
    e.vel_x     = def.patrol_speed;
    e.vel_y     = 0.0f;
    e.facing_left = (def.patrol_speed < 0.0f);
    e.dead_timer  = 0.0f;
    e.shell_timer = 0.0f;
    e.oscillation_timer = 0.0f;
}

void EnemyManager::Update(float dt, const Tilemap& tilemap) {
    for (auto& e : _enemies) {
        if (!e.active) continue;

        if (e.state == EnemyState::Dead) {
            e.dead_timer -= dt;
            if (e.dead_timer <= 0.0f) {
                e.active = false;
                _em.Destroy(e.id);
            }
            continue;
        }

        // Shell: stationary wait — still needs gravity + ground collision
        if (e.state == EnemyState::Shell) {
            e.shell_timer -= dt;
            if (e.shell_timer <= 0.0f) {
                e.state       = EnemyState::Patrol;
                e.vel_x       = e.def->patrol_speed;
                e.facing_left = (e.vel_x < 0.0f);
            }
            e.vel_y += e.def->gravity * dt;
            MoveEnemy(e, dt, tilemap);
            continue;
        }

        // Edge check BEFORE move — prevent stepping off the ledge
        if (e.state == EnemyState::Patrol && e.def->turns_at_edges) {
            if (IsEdgeAhead(e, tilemap)) {
                e.vel_x       = -e.vel_x;
                e.facing_left = (e.vel_x < 0.0f);
            }
        }

        // Patrol or Sliding
        if (e.def->oscillation_amp > 0.0f) {
            e.oscillation_timer += dt;
            e.pos_y = e.origin_y + std::sin(e.oscillation_timer * e.def->oscillation_speed) * e.def->oscillation_amp;
        } else {
            e.vel_y += e.def->gravity * dt;
        }

        MoveEnemy(e, dt, tilemap);
    }
}

void EnemyManager::RegisterAll(CollisionSystem& collision_system) {
    for (auto& e : _enemies) {
        if (e.active) {
            collision_system.Register(e.id, e.GetAABB(), e.vel_x, e.vel_y);
        }
    }
}

void EnemyManager::HandleCollisions(const CollisionEventPool& pool, EntityID player_id, Player& player, const SpriteSheet& sheet) {
    for (auto& e : _enemies) {
        if (!e.active || e.state == EnemyState::Dead) continue;

        for (int i = 0; i < pool.Count(); ++i) {
            const CollisionEvent& ev = pool.Get(i);
            bool player_is_a = (ev.entity_a == player_id && ev.entity_b == e.id);
            bool player_is_b = (ev.entity_b == player_id && ev.entity_a == e.id);
            if (!player_is_a && !player_is_b) continue;

            float ny       = player_is_a ? ev.normal_y : -ev.normal_y;
            bool  is_stomp = (ny < -0.5f &&
                              player.GetVelY() > 50.0f &&
                              (player.GetY() + player.GetH()) < (e.pos_y + e.def->h * 0.5f));

            if (e.def->unstomp_able) {
                is_stomp = false;
            }

            if (is_stomp) {
                _pending_score += 100;
                player.Bounce(-400.0f);

                if (e.def->is_flyer) {
                    e.def = &EnemyDef::KOOPA;
                    e.anim_walk = Animation(sheet, e.def->walk_frames, e.def->walk_frame_count, e.def->walk_frame_duration, true);
                    e.anim_dead = Animation(sheet, e.def->dead_frames, e.def->dead_frame_count, e.def->dead_frame_duration, false);
                    if (e.def->shell_frame_count > 0) {
                        e.anim_shell = Animation(sheet, e.def->shell_frames, e.def->shell_frame_count, e.def->shell_frame_duration, true);
                    }
                    e.state = EnemyState::Patrol;
                    e.vel_x = e.facing_left ? -e.def->patrol_speed : e.def->patrol_speed;
                    e.vel_y = 0.0f;
                } else if (!e.def->has_shell || e.state == EnemyState::Sliding) {
                    e.state      = EnemyState::Dead;
                    e.dead_timer = e.def->dead_duration;
                    e.vel_x      = 0.0f;
                    e.vel_y      = 0.0f;
                } else if (e.state == EnemyState::Patrol) {
                    e.state       = EnemyState::Shell;
                    e.vel_x       = 0.0f;
                    e.shell_timer = e.def->shell_wait_time;
                } else if (e.state == EnemyState::Shell) {
                    bool player_left = (player.GetX() + player.GetW() * 0.5f) < (e.pos_x + e.def->w * 0.5f);
                    e.state             = EnemyState::Sliding;
                    e.vel_x             = player_left ? e.def->shell_slide_speed : -e.def->shell_slide_speed;
                    e.facing_left       = !player_left;
                    e.slide_bounce_count = 0;
                }
                break;  // one stomp per enemy per frame
            } else {
                // Side contact
                if (e.state == EnemyState::Shell) {
                    bool player_left = (player.GetX() + player.GetW() * 0.5f) < (e.pos_x + e.def->w * 0.5f);
                    e.state             = EnemyState::Sliding;
                    e.vel_x             = player_left ? e.def->shell_slide_speed : -e.def->shell_slide_speed;
                    e.facing_left       = !player_left;
                    e.slide_bounce_count = 0;
                    break;  // kicked — no further events this frame
                } else if (e.state == EnemyState::Sliding && e.slide_bounce_count == 0) {
                    // Grace window: shell just kicked, still exiting player AABB — not dangerous yet
                    break;
                } else {
                    player.Hurt();
                    break;
                }
            }
        }
    }

    // --- Sliding shell kills other enemies ---
    for (auto& shell : _enemies) {
        if (!shell.active || shell.state != EnemyState::Sliding) continue;

        for (auto& victim : _enemies) {
            if (!victim.active || victim.state == EnemyState::Dead) continue;
            if (&shell == &victim) continue;

            if (AABB::Overlaps(shell.GetAABB(), victim.GetAABB())) {
                _pending_score += 100;
                if (victim.state == EnemyState::Sliding) {
                    // Two sliding shells collide — bounce off each other
                    shell.vel_x  = -shell.vel_x;
                    victim.vel_x = -victim.vel_x;
                } else if (victim.def->has_shell) {
                    victim.state      = EnemyState::Shell;
                    victim.vel_x      = 0.0f;
                    victim.shell_timer = victim.def->shell_wait_time;
                } else {
                    victim.state      = EnemyState::Dead;
                    victim.dead_timer = victim.def->dead_duration;
                    victim.vel_x      = 0.0f;
                    victim.vel_y      = 0.0f;
                }
            }
        }
    }

}

void EnemyManager::RenderAll(SpriteBatch& sprite_batch, float dt) {
    for (auto& e : _enemies) {
        if (e.active) {
            sprite_batch.Draw(e.pos_x, e.pos_y, e.GetW(), e.GetH(),
                              e.GetSprite(dt), 1.0f, 1.0f, 1.0f, 1.0f, !e.IsFacingLeft());
        }
    }
}

int EnemyManager::ActiveCount() const {
    int count = 0;
    for (const auto& e : _enemies) {
        if (e.active) ++count;
    }
    return count;
}

int EnemyManager::PopScore() {
    int s = _pending_score;
    _pending_score = 0;
    return s;
}

void EnemyManager::ClearAll() {
    for (auto& e : _enemies) {
        if (e.active) {
            _em.Destroy(e.id);
            e.active = false;
        }
    }
    _pending_score = 0;
}