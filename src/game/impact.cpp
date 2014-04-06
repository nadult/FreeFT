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
				float3 center = boundingBox().center();
				FBox bbox(center - float3(1.0f, 1.0f, 1.0f) * m_proto.range, center + float3(1.0f, 1.0f, 1.0f) * m_proto.range);

				vector<ObjectRef> entities;
				findAll(entities, bbox, nullptr, Flags::entity);

				for(int n = 0; n < (int)entities.size(); n++) {
					Entity *entity = refEntity((EntityRef)entities[n]);
					float dist = distance(FBox(center, center), entity->boundingBox()) / m_proto.range;
					float strength = dist < 0.0f? 0.0f : (1 - dist) * (1.0f - dist);

					if(strength > 0.0f) {
						Segment segment(center, entity->boundingBox().center());

						if(!trace(segment, entity, Flags::tile | Flags::colliding)) {
							//TODO: decrease damage if blocked by another entity	
							//printf("dist: %f | damage: %f  | force: %f\n", dist, m_proto.damage * m_damage_mod * strength, m_proto.force * strength);
							float3 force = segment.dir() * m_proto.force * strength;

							entity->onImpact(m_proto.damage_type, strength * m_proto.damage * m_damage_mod, force);
						}
					}
				}
			}
			else {
				Entity *target = refEntity(m_target);
				
				if(source && target && source != target) {
					float dist = distance(source->boundingBox(), target->boundingBox());

					if(m_proto.type == ImpactType::ranged || dist <= m_proto.range) {
						float3 force = target->boundingBox().center() - source->boundingBox().center();
						force = force * m_proto.force / length(force);

						target->onImpact(m_proto.damage_type, m_proto.damage * m_damage_mod, force);
					}
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

