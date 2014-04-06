/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/projectile.h"
#include "sys/xml.h"
#include "game/world.h"

namespace game {

	ProjectileProto::ProjectileProto(const TupleParser &parser) :ProtoImpl(parser) {
		impact = parser("impact_id");
		blend_angles = toBool(parser("blend_angles"));
		speed = toFloat(parser("speed"));
		max_range = toFloat(parser("max_range"));
	}

	void ProjectileProto::link() {
		impact.link();
	}

	Projectile::Projectile(const ProjectileProto &proto, float initial_angle, const float3 &dir, EntityRef spawner, float damage_mod)
		:EntityImpl(proto), m_dir(dir), m_spawner(spawner), m_distance(0.0f), m_damage_mod(damage_mod) {
			m_dir *= 1.0f / length(m_dir);
			setDirAngle(initial_angle);
			m_target_angle = vectorToAngle(m_dir.xz());
			if(!proto.blend_angles)
				setDirAngle(m_target_angle);
			m_speed = proto.speed;
			m_frame_count = 0;
	//		printf("Spawning projectile at: (%.0f %.0f %.0f) -> %.2f %.2f\n",
	//				this->pos().x, this->pos().y, this->pos().z, m_dir.x, m_dir.z);
	}

	Projectile::Projectile(Stream &sr) :EntityImpl(sr) {
		sr.unpack(m_dir, m_speed, m_frame_count, m_target_angle, m_damage_mod);
		sr >> m_spawner;
	}

	void Projectile::save(Stream &sr) const {
		EntityImpl::save(sr);
		sr.pack(m_dir, m_speed, m_frame_count, m_target_angle, m_damage_mod);
		sr << m_spawner;
	}
		
	XMLNode Projectile::save(XMLNode& parent) const {
		return Entity::save(parent);
	}
	
	void Projectile::nextFrame() {
		Entity::nextFrame();
		setDirAngle(blendAngles(dirAngle(), m_target_angle, constant::pi * 0.01f));
		m_frame_count++;
	}

	void Projectile::think() {
		float time_delta = timeDelta();
		Ray ray(pos(), m_dir);
		float ray_pos = m_speed * time_delta;

		if(m_frame_count < 2)
			return;

		Intersection isect = trace(Segment(ray, 0.0f, ray_pos), {Flags::all | Flags::colliding, m_spawner});
		float3 new_pos = ray.at(min(isect.distance(), ray_pos));
		m_distance += length(new_pos - pos());
		setPos(new_pos);

		//TODO: also remove when is out of map
		if(m_distance > m_proto.max_range)
			remove();

		if(isect.distance() < ray_pos) {
			if(m_proto.impact.isValid() && !isClient()) {
				EntityRef ref = world()->toEntityRef(isect);
				addNewEntity<Impact>(new_pos, *m_proto.impact, m_spawner, ref, m_damage_mod);
			}
			remove();
		}
	}

}
