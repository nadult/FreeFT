#ifndef GAME_PROJECTILE_H
#define GAME_PROJECTILE_H

#include "game/entity.h"

namespace game {

	namespace ProjectileType {
		enum Type {
			bullet,
			plasma,
			laser,
			rocket,

			count,
		};

		const char *toString(Type);
		Type fromString(const char*);
	};

   	//TODO: derived from Entity mainly for sprite animation handling,
	//maybe this class should be more lightweight
	class Projectile: public Entity {
	public:
		Projectile(ProjectileType::Type type, const float3 &pos, const float3 &target, Entity *spawner);
		virtual ColliderFlags colliderType() const { return collider_none; }
		virtual EntityFlags entityType() const { return entity_projectile; }

		ProjectileType::Type type() const { return m_type; }

	protected:
		virtual void think();
		friend class World;

	private:
		ProjectileType::Type m_type;
		Entity *m_spawner; //TODO: make a EntityRef class which will be zeroed when entity is destroyed
		float3 m_dir;
		float m_speed;
	};

	class ProjectileImpact: public Entity {
	public:
		ProjectileImpact(const char *sprite_name, const float3 &pos);
		virtual ColliderFlags colliderType() const { return collider_none; }
		virtual EntityFlags entityType() const { return entity_impact; }
	
	protected:
		virtual void onAnimFinished();
	};

	typedef std::unique_ptr<Projectile> PProjectile;
	typedef std::unique_ptr<ProjectileImpact> PProjectileImpact;

}


#endif
