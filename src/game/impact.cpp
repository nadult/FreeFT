/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/impact.h"
#include "sys/xml.h"
#include "game/world.h"

namespace game {

	DEFINE_ENUM(ImpactType,
		"ranged",
		"melee",
		"area"
	);

	ImpactProto::ImpactProto(const TupleParser &parser) :ProtoImpl(parser) {
		damage = toFloat(parser("damage"));
		damage_type = DamageType::fromString(parser("damage_type"));
		force = toFloat(parser("force"));
		range = toFloat(parser("range"));
		type = ImpactType::fromString(parser("type_id"));
		sound_idx = SoundId(parser("sound_id"));
		is_invisible = sprite_name == "impactfx/Projectile Invisi";
	}

	Impact::Impact(const ImpactProto &proto, EntityRef source, EntityRef target, float damage_mod)
		:EntityImpl(proto), m_played_sound(false), m_applied_damage(false), m_source(source), m_target(target), m_damage_mod(damage_mod) {
	}

	Impact::Impact(Stream &sr) :EntityImpl(sr), m_played_sound(false) {
		sr >> m_source >> m_target;
		sr.unpack(m_damage_mod, m_applied_damage);
	}

	void Impact::save(Stream &sr) const {
		EntityImpl::save(sr);
		sr << m_source << m_target;
		sr.pack(m_damage_mod, m_applied_damage);
	}
	
	XMLNode Impact::save(XMLNode& parent) const {
		return Entity::save(parent);
	}

	void Impact::onAnimFinished() {
		remove();
	}

	void Impact::think() {
		if(!m_applied_damage) {
			Entity *source = refEntity(m_source);

			if(m_proto.type == ImpactType::area) {
				//TODO: area damage
			}
			else {
				Entity *target = refEntity(m_target);
				
				if(source && target && source != target) {
					float dist = sqrtf(distanceSq(source->boundingBox(), target->boundingBox()));

					if(m_proto.type == ImpactType::ranged || dist <= m_proto.range)
						target->onImpact(m_proto.damage_type, m_proto.damage * m_damage_mod, m_proto.force);
				}
			}
			m_applied_damage = true;
		}

		if(!m_played_sound) {
			world()->playSound(m_proto.sound_idx, pos());
			m_played_sound = true;
		}
		
		if(m_proto.is_invisible)
			remove();
	}

}

