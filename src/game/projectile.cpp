#include "game/projectile.h"
#include "game/world.h"

namespace game {

	Projectile::Projectile(const char *sprite_name, const float3 &pos, const float3 &target, Entity *spawner)
		:Entity(sprite_name, pos), m_dir(target - pos), m_spawner(spawner) {
			m_dir *= 1.0f / length(m_dir);
			setDir(m_dir.xz());
			m_speed = 200.0f;
//			printf("Spawning projectile at: (%.0f %.0f %.0f) -> %.2f %.2f\n",
//					this->pos().x, this->pos().y, this->pos().z, m_dir.x, m_dir.z);
		}

	void Projectile::think() {
		float time_delta = m_world->timeDelta();
		Ray ray(pos(), m_dir);
		float ray_pos = m_speed * time_delta;

		Intersection isect = m_world->intersect(Segment(ray, 0.0f, ray_pos));
		float3 new_pos = ray.at(min(isect.distance, ray_pos));
		setPos(new_pos);

		if(isect.distance < ray_pos) {
			m_world->spawnProjectileImpact(0, new_pos);
			remove();
		}
	}

	ProjectileImpact::ProjectileImpact(const char *sprite_name, const float3 &pos)
		:Entity(sprite_name, pos) { }

	void ProjectileImpact::onAnimFinished() {
		remove();
	}


}
