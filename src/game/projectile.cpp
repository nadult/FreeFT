/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "game/projectile.h"
#include "game/world.h"

namespace game {

	static const char *s_projectile_names[ProjectileTypeId::count] = {
		"impactfx/Projectile Invisi",
		"impactfx/Projectile Plasma",
		"impactfx/Projectile Laser",
		"impactfx/Projectile Rocket",
	};

	static const char *s_impact_names[ProjectileTypeId::count] = {
		"impactfx/RobotSparks",
		"impactfx/Impact Plasma",
		"impactfx/Impact Laser",
		nullptr,				// rocket impact is handled differently
	};

	Projectile::Projectile(ProjectileTypeId::Type type, float speed, const float3 &pos, const float3 &target,
			Entity *spawner)
		:Entity(s_projectile_names[type], pos), m_dir(target - pos), m_spawner(spawner), m_type(type) {
			m_dir *= 1.0f / length(m_dir);
			setDir(m_dir.xz());
			m_speed = speed;
//			printf("Spawning projectile at: (%.0f %.0f %.0f) -> %.2f %.2f\n",
//					this->pos().x, this->pos().y, this->pos().z, m_dir.x, m_dir.z);
	}

	void Projectile::think() {
		float time_delta = m_world->timeDelta();
		Ray ray(pos(), m_dir);
		float ray_pos = m_speed * time_delta;

		Intersection isect = m_world->trace(Segment(ray, 0.0f, ray_pos));
		float3 new_pos = ray.at(min(isect.distance(), ray_pos));
		setPos(new_pos);

		if(isect.distance() < ray_pos) {
			if(s_impact_names[m_type]) {
				PProjectileImpact impact(new ProjectileImpact(s_impact_names[m_type], new_pos));
				m_world->spawnProjectileImpact(std::move(impact));
			}
			remove();
		}
	}

	ProjectileImpact::ProjectileImpact(const char *sprite_name, const float3 &pos)
		:Entity(sprite_name, pos) { }

	void ProjectileImpact::onAnimFinished() {
		remove();
	}


}
