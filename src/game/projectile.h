#ifndef GAME_PROJECTILE_H
#define GAME_PROJECTILE_H

#include "game/entity.h"

namespace game {

   	//TODO: derived from Entity mainly for sprite animation handling,
	//maybe this class should be more lightweight
	class Projectile: public Entity {
	public:
		Projectile(const char *sprite_name, int3 pos, int3 target, Entity *spawner);
		virtual ColliderType colliderType() const { return collider_none; }

	protected:
		virtual void think();
		friend class World;

	private:
		Entity *m_spawner; //TODO: make a EntityRef class which will be zeroed when entity is destroyed
		float3 m_dir;
		float m_speed;
	};

	class ProjectileImpact: public Entity {
	public:
		ProjectileImpact(const char *sprite_name, int3 pos);
		virtual ColliderType colliderType() const { return collider_none; }
	
	protected:
		virtual void onAnimFinished();
	};

	typedef std::unique_ptr<Projectile> PProjectile;
	typedef std::unique_ptr<ProjectileImpact> PProjectileImpact;

}


#endif
