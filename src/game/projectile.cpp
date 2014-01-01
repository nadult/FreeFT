/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/projectile.h"
#include "game/world.h"
#include "sys/xml.h"

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

	Projectile::Projectile(ProjectileTypeId::Type type, float speed, const float3 &pos, const float3 &target, Entity *spawner)
		:Entity(s_projectile_names[type]), m_dir(target - pos), m_spawner(spawner), m_type(type) {
			m_dir *= 1.0f / length(m_dir);
			setPos(pos);
			setDir(m_dir.xz());
			m_speed = speed;
//			printf("Spawning projectile at: (%.0f %.0f %.0f) -> %.2f %.2f\n",
//					this->pos().x, this->pos().y, this->pos().z, m_dir.x, m_dir.z);
	}

	Projectile::Projectile(Stream &sr) {
		sr.unpack(m_type, m_dir, m_speed);
		Entity::initialize(s_projectile_names[m_type]);
		loadEntityParams(sr);
	}

	void Projectile::save(Stream &sr) const {
		sr.pack(m_type, m_dir, m_speed);
		saveEntityParams(sr);
	}
		
	XMLNode Projectile::save(XMLNode& parent) const {
		return Entity::save(parent);
	}
	
	Entity *Projectile::clone() const {
		return new Projectile(*this);
	}

	void Projectile::think() {
		World *world = Entity::world();

		float time_delta = world->timeDelta();
		Ray ray(pos(), m_dir);
		float ray_pos = m_speed * time_delta;

		Intersection isect = world->trace(Segment(ray, 0.0f, ray_pos));
		float3 new_pos = ray.at(min(isect.distance(), ray_pos));
		setPos(new_pos);

		if(isect.distance() < ray_pos) {
			if(s_impact_names[m_type]) {
				world->addEntity(new Impact(s_impact_names[m_type], new_pos));

				if(isect.isEntity()) {
					float damage = 100.0f;
					isect.entity()->onImpact(type(), damage);
				}
			}
			remove();
		}
	}

	Impact::Impact(const char *sprite_name, const float3 &pos)
		:Entity(sprite_name) {
		setPos(pos);
	}
	
	Impact::Impact(Stream &sr) :Entity(sr) {
	}

	void Impact::save(Stream &sr) const {
		Entity::save(sr);
	}
	
	XMLNode Impact::save(XMLNode& parent) const {
		return Entity::save(parent);
	}

	Entity *Impact::clone() const {
		return new Impact(*this);
	}

	void Impact::onAnimFinished() {
		remove();
	}


}
