#include "Enemy.h"

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

    // Walk: 2 frames from enemies.png
    /* walk_frame_count  */ 2,
    /* walk_frames       */ { AnimFrameDef{5, 14, 16, 16, 0.5f}, AnimFrameDef{25, 14, 16, 16, 0.5f} },

    // Dead: squish frame (shorter height)
    /* dead_frame_count  */ 1,
    /* dead_frames       */ { AnimFrameDef{45, 21, 16, 8, 0.5f} },

    // No shell
    /* shell_frame_count */ 0,
    /* shell_frames      */ {}
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
    /* walk_frames       */ { AnimFrameDef{5, 130, 16, 28, 0.3f},
                              AnimFrameDef{28, 130, 16, 28, 0.3f} },

    /* dead_frame_count  */ 1,
    /* dead_frames       */ { AnimFrameDef{50, 139, 16, 16, 0.5f} },

    /* shell_frame_count */ 1,
    /* shell_frames      */ { AnimFrameDef{70, 139, 16, 16, 1.0f} }
};

// -----------------------------------------------------------------------

Enemy::Enemy(const Texture* texture, const EnemyDef& def_ref, EntityManager& em)
    : anim_walk(texture),
      anim_dead(texture),
      anim_shell(texture),
      def(&def_ref)
{
    for (int i = 0; i < def->walk_frame_count; ++i) {
        const AnimFrameDef& f = def->walk_frames[i];
        anim_walk.AddFrame(f.x, f.y, f.w, f.h, f.duration);
    }
    anim_walk.SetLooping(true);

    for (int i = 0; i < def->dead_frame_count; ++i) {
        const AnimFrameDef& f = def->dead_frames[i];
        anim_dead.AddFrame(f.x, f.y, f.w, f.h, f.duration);
    }
    anim_dead.SetLooping(false);

    for (int i = 0; i < def->shell_frame_count; ++i) {
        const AnimFrameDef& f = def->shell_frames[i];
        anim_shell.AddFrame(f.x, f.y, f.w, f.h, f.duration);
    }
    anim_shell.SetLooping(true);

    id    = em.Create();
    vel_x = def->patrol_speed;
    active = false;
}

const Sprite& Enemy::GetSprite(float dt) {
    if (state == EnemyState::Dead)    return anim_dead.Update(dt);
    if (state == EnemyState::Shell)   return anim_shell.Update(dt);
    if (state == EnemyState::Sliding) return anim_dead.Update(dt);
    return anim_walk.Update(dt);
}