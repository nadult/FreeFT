/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/projectile.h"
#include "game/world.h"
#include "sys/xml.h"

namespace game {

	ImpactDesc::ImpactDesc(const TupleParser &parser) :Tuple(parser) {
		sprite_name = parser("sprite_name");
	}

	ProjectileDesc::ProjectileDesc(const TupleParser &parser) :Tuple(parser) {
		sprite_name = parser("sprite_name");
		impact_ref = parser("impact_id");
		blend_angles = toBool(parser("blend_angles"));
		speed = toFloat(parser("speed"));
	}

	void ProjectileDesc::connect() {
		if(!impact_ref.id().empty())
			impact_ref.connect();
	}

	Projectile::Projectile(const ProjectileDesc &desc, const float3 &pos,
							float initial_angle, const float3 &target, Entity *spawner)
		:Entity(desc.sprite_name.c_str()), m_dir(target - pos), m_spawner(spawner), m_desc(&desc) {
			m_dir *= 1.0f / length(m_dir);
			setPos(pos);
			setDirAngle(initial_angle);
			m_target_angle = vectorToAngle(m_dir.xz());
			if(!desc.blend_angles)
				setDirAngle(m_target_angle);
			m_speed = desc.speed;
			m_frame_count = 0;
//			printf("Spawning projectile at: (%.0f %.0f %.0f) -> %.2f %.2f\n",
//					this->pos().x, this->pos().y, this->pos().z, m_dir.x, m_dir.z);
	}

	Projectile::Projectile(Stream &sr) {
		//TODO: wrong id may cause a crash, should we do smth about it?
		m_desc = &ProjectileDesc::get(sr.decodeInt());
		sr.unpack(m_dir, m_speed, m_frame_count, m_target_angle);
		sr >> m_spawner;
		Entity::initialize(m_desc->sprite_name.c_str());
		loadEntityParams(sr);
	}

	void Projectile::save(Stream &sr) const {
		sr.encodeInt(m_desc->idx);
		sr.pack(m_dir, m_speed, m_frame_count, m_target_angle);
		sr << m_spawner;
		saveEntityParams(sr);
	}
		
	XMLNode Projectile::save(XMLNode& parent) const {
		return Entity::save(parent);
	}
	
	Entity *Projectile::clone() const {
		return new Projectile(*this);
	}

	void Projectile::nextFrame() {
		Entity::nextFrame();
		setDirAngle(blendAngles(dirAngle(), m_target_angle, constant::pi * 0.01f));
		m_frame_count++;
	}

	void Projectile::think() {
		World *world = Entity::world();

		float time_delta = world->timeDelta();
		Ray ray(pos(), m_dir);
		float ray_pos = m_speed * time_delta;

		if(m_frame_count < 2)
			return;

		Intersection isect = world->trace(Segment(ray, 0.0f, ray_pos), m_spawner.get(), collider_solid);
		float3 new_pos = ray.at(min(isect.distance(), ray_pos));
		setPos(new_pos);

		if(isect.distance() < ray_pos) {
			if(m_desc->impact_ref.isValid()) {
				world->addEntity(new Impact(*(const ImpactDesc*)m_desc->impact_ref, new_pos));

				if(isect.isEntity()) {
					float damage = 100.0f;
					isect.entity()->onImpact(0, damage);
				}
			}
			remove();
		}
	}

	Impact::Impact(const ImpactDesc &desc, const float3 &pos)
		:Entity(desc.sprite_name.c_str()), m_desc(&desc) {
		setPos(pos);
	}
	
	Impact::Impact(Stream &sr) {
		//TODO: wrong id may cause a crash, should we do smth about it?
		m_desc = &ImpactDesc::get(sr.decodeInt());
		Entity::initialize(m_desc->sprite_name.c_str());
		loadEntityParams(sr);
	}

	void Impact::save(Stream &sr) const {
		sr.encodeInt(m_desc->idx);
		saveEntityParams(sr);
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
