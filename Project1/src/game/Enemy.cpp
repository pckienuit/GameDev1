#include "Enemy.h"

Enemy::Enemy(const Texture* texture, EntityManager& em) 
    : anim_walk(texture),
      anim_dead(texture)
{
    anim_walk.AddFrame(5, 14, 16, 16, 0.5);
    anim_walk.AddFrame(25, 14, 16, 16, 0.5f);
    anim_walk.SetLooping(true);
    
    anim_dead.AddFrame(45, 21, 16, 8, 0.5f);
    anim_dead.SetLooping(false);

    id = em.Create();
    vel_x = PATROL_SPEED;
    active = false;    
}

const Sprite& Enemy::GetSprite(float dt) {
    if (state == EnemyState::Dead) return anim_dead.Update(dt);
    return anim_walk.Update(dt);
}