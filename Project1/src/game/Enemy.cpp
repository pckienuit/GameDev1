#include "Enemy.h"

Enemy::Enemy(const Texture* texture, EntityManager& em) 
    : anim_walk(texture),
      anim_dead(texture)
{
    anim_walk.AddFrame(0, 0, 48, 48, 0.1f);
    anim_walk.AddFrame(48, 0, 48, 48, 0.1f);
    anim_walk.AddFrame(96, 0, 48, 48, 0.1f);
    anim_walk.SetLooping(true);
    
    anim_dead.AddFrame(0, 48, 48, 48, 0.1f);
    anim_dead.SetLooping(false);

    id = em.Create();
    vel_x = PATROL_SPEED;
    active = false;    
}

const Sprite& Enemy::GetSprite(float dt) {
    if (state == EnemyState::Dead) return anim_dead.Update(dt);
    return anim_walk.Update(dt);
}