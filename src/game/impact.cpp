// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/impact.h"

namespace game {

	ImpactProto::ImpactProto(const TupleParser &parser) :ProtoImpl(parser) {
		damage = parser.get<float>("damage");
		damage_type = fromString<DamageType>(parser("damage_type"));
		force = parser.get<float>("force");
		range = parser.get<float>("range");
		type = fromString<ImpactType>(parser("type_id"));
		sound_idx = SoundId(parser("sound_id"));
		is_invisible = sprite_name == "impactfx/Projectile Invisi";
	}

	Impact::Impact(const ImpactProto &proto, EntityRef source, EntityRef target, float damage_mod)
		:EntityImpl(proto), m_played_sound(false), m_applied_damage(false), m_source(source), m_target(target), m_damage_mod(damage_mod) {
	}

	Impact::Impact(MemoryStream &sr) :EntityImpl(sr), m_played_sound(false) {
		m_source.load(sr);
		m_target.load(sr);
		sr.unpack(m_damage_mod, m_applied_damage);
	}

	void Impact::save(MemoryStream &sr) const {
		EntityImpl::save(sr);
		m_source.save(sr);
		m_target.save(sr);
		sr.pack(m_damage_mod, m_applied_damage);
	}
	
	XmlNode Impact::save(XmlNode parent) const {
		return Entity::save(parent);
	}
		
	void Impact::addToRender(SceneRenderer &out, Color color) const {
		if(!m_target || proto().type != ImpactType::ranged)
			Entity::addToRender(out, color);
	}

	void Impact::onAnimFinished() {
		remove();
	}
		
	void Impact::applyDamage() {
		if(isClient())
			return;

		Entity *source = refEntity(m_source);

		if(m_proto.type == ImpactType::area || m_proto.type == ImpactType::area_safe) {
			float3 center = boundingBox().center();
			FBox bbox(center - float3(1.0f, 1.0f, 1.0f) * m_proto.range, center + float3(1.0f, 1.0f, 1.0f) * m_proto.range);

			vector<ObjectRef> entities;
			findAll(entities, bbox, Flags::entity);

			for(int n = 0; n < (int)entities.size(); n++) {
				Entity *entity = refEntity(entities[n]);
				if(m_proto.type == ImpactType::area_safe && entity->ref() == m_source)
					continue;

				float dist = distance(FBox(center, center), entity->boundingBox()) / m_proto.range;
				float strength = dist < 0.0f? 0.0f : (1 - dist) * (1.0f - dist);

				if(strength > 0.0f) {
					if(distance(center, entity->boundingBox().center()) < big_epsilon) {
						entity->onImpact(m_proto.damage_type, strength * m_proto.damage * m_damage_mod, float3(), m_source);
					}
					else {
						Segment3F segment(center, entity->boundingBox().center());

						if(!trace(segment, {Flags::tile | Flags::colliding, entities[n]})) {
							//TODO: decrease damage if blocked by another entity	
							//printf("dist: %f | damage: %f  | force: %f\n", dist, m_proto.damage * m_damage_mod * strength, m_proto.force * strength);
							auto ray = *segment.asRay();
							float3 force = ray.dir() * m_proto.force * strength * m_damage_mod;
							entity->onImpact(m_proto.damage_type, strength * m_proto.damage * m_damage_mod, force, m_source);
						}
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

					target->onImpact(m_proto.damage_type, m_proto.damage * m_damage_mod, force, m_source);
				}
			}
		}
	}

	void Impact::think() {
		if(!m_applied_damage) {
			applyDamage();
			m_applied_damage = true;
		}

		if(!m_played_sound) {
			playSound(m_proto.sound_idx, pos(), m_proto.type == ImpactType::area? SoundType::explosion : SoundType::normal);
			m_played_sound = true;
		}
		
		if(m_proto.is_invisible)
			remove();
	}

}

