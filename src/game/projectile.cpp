#include "game/projectile.h"
#include "game/world.h"

namespace game {

	Projectile::Projectile(const char *sprite_name, int3 pos, int3 target, Entity *spawner)
		:Entity(sprite_name, pos), m_dir(float3(target - pos)), m_spawner(spawner) {
			m_dir *= 1.0f / length(m_dir);
			setDir(m_dir.xz());
			m_speed = 200.0f;
//			printf("Spawning projectile at: (%.0f %.0f %.0f) -> %.2f %.2f\n",
//					this->pos().x, this->pos().y, this->pos().z, m_dir.x, m_dir.z);
		}

	void Projectile::think() {
		float time_delta = m_world->timeDelta();
		float3 new_pos = pos() + m_dir * time_delta * m_speed;
		IBox new_box = IBox(int3(new_pos), int3(new_pos) + bboxSize() + int3(1, 1, 1));

		//TODO: better collision detection
		if(m_world->isInside(new_box) && !m_world->isColliding(new_box, m_spawner))
			setPos(new_pos);
		else {
			float3 min_pos = pos(), max_pos = new_pos;
			float3 proj_size(1, 1, 1);

			for(int n = 0; n < 10; n++) {
				float3 mid_pos = (min_pos + max_pos) * 0.5f;
				if(m_world->isColliding(IBox((int3)mid_pos, (int3)(mid_pos + proj_size)), m_spawner))
					max_pos = mid_pos;
				else
					min_pos = mid_pos;
			}

			m_world->spawnProjectileImpact(0, (int3)min_pos);
			remove();
		}
	}

	ProjectileImpact::ProjectileImpact(const char *sprite_name, int3 pos)
		:Entity(sprite_name, pos) { }

	void ProjectileImpact::onAnimFinished() {
		remove();
	}


}
