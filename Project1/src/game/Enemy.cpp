#include "Enemy.h"
#include "../renderer/SpriteSheet.h"

// -----------------------------------------------------------------------
// EnemyDef presets — UV coordinates live in Game.cpp::InitSprites()
// -----------------------------------------------------------------------

const EnemyDef EnemyDef::GOOMBA = {
    /* w                 */ 48.0f,
    /* h                 */ 48.0f,
    /* patrol_speed      */ 60.0f,
    /* dead_duration     */ 0.5f,
    /* gravity           */ 1200.0f,
    /* turns_at_edges    */ true,
    /* has_shell         */ false,
    /* shell_wait_time   */ 0.0f,
    /* shell_slide_speed */ 0.0f,
    /* max_slide_bounces */ 0,

    /* walk_frame_count  */ 2,
    /* walk_frames       */ { SpriteID::GoombaWalk0, SpriteID::GoombaWalk1 },
    /* walk_frame_duration */ 0.5f,

    /* dead_frame_count  */ 1,
    /* dead_frames       */ { SpriteID::GoombaDead },
    /* dead_frame_duration */ 0.5f,

    /* shell_frame_count */ 0,
    /* shell_frames      */ {},
    /* shell_frame_duration */ 1.0f,
};

const EnemyDef EnemyDef::KOOPA = {
    /* w                 */ 48.0f,
    /* h                 */ 48.0f,
    /* patrol_speed      */ 60.0f,
    /* dead_duration     */ 0.5f,
    /* gravity           */ 1200.0f,
    /* turns_at_edges    */ true,
    /* has_shell         */ true,
    /* shell_wait_time   */ 5.0f,
    /* shell_slide_speed */ 300.0f,
    /* max_slide_bounces */ 3,

    /* walk_frame_count  */ 2,
    /* walk_frames       */ { SpriteID::KoopaWalk0, SpriteID::KoopaWalk1 },
    /* walk_frame_duration */ 0.3f,

    /* dead_frame_count  */ 1,
    /* dead_frames       */ { SpriteID::KoopaDead },
    /* dead_frame_duration */ 0.5f,

    /* shell_frame_count */ 1,
    /* shell_frames      */ { SpriteID::KoopaShell },
    /* shell_frame_duration */ 1.0f,
};

// -----------------------------------------------------------------------

Enemy::Enemy(const SpriteSheet& sheet, const EnemyDef& def_ref, EntityManager& em)
    : def(&def_ref)
{
    anim_walk = Animation(sheet, def->walk_frames,  def->walk_frame_count,
                                def->walk_frame_duration, true);
    anim_dead = Animation(sheet, def->dead_frames,  def->dead_frame_count,
                                def->dead_frame_duration, false);
    if (def->shell_frame_count > 0) {
        anim_shell = Animation(sheet, def->shell_frames, def->shell_frame_count,
                                     def->shell_frame_duration, true);
    }

    id     = em.Create();
    vel_x  = def->patrol_speed;
    active = false;
}

const Sprite& Enemy::GetSprite(float dt) {
    if (state == EnemyState::Dead)    return anim_dead.Update(dt);
    if (state == EnemyState::Shell)   return anim_shell.Update(dt);
    if (state == EnemyState::Sliding) return anim_dead.Update(dt);
    return anim_walk.Update(dt);
}