/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/turret.h"
#include "game/sprite.h"
#include "game/projectile.h"
#include "game/tile.h"
#include "game/world.h"
#include "game/weapon.h"
#include "game/game_mode.h"
#include "game/all_orders.h"
#include "sys/data_sheet.h"
#include "net/socket.h"
#include <cmath>
#include <cstdio>
#include "gfx/scene_renderer.h"

//#define DEBUG_SHOOTING

namespace game {

	static const EnumMap<TurretAction, const char*> s_seq_names = {
		"StandArmed",
		"StandAttackUnarmedTwo",
		"StandAttackUnarmedThree",
		"StandUnarming",
		"StandArming",
		"StandUnarmed",
		"Death",
		"DeathElectrify",
		"DeathExplode"
	};

	struct TurretSound {
		const char *name;
		int index;
	};
   	static const EnumMap<TurretSoundId, TurretSound> s_sounds = {
		{ "death", 0 },
		{ "death", 1 },
		{ "attack", 1 },
		{ "attack", 2 },
		{ "arming", 0 },
		{ "unarming", 0 }
	};

	TurretProto::TurretProto(const TupleParser &parser) :ProtoImpl(parser) {
		ASSERT(!is_dummy);
		ASSERT(sprite);

		hit_points = parser.get<float>("hit_points");
	
		for(auto taction : all<TurretAction>()) {
			int id = sprite->findSequence(s_seq_names[taction]);
			anim_idx[taction] = id == -1 || id > 254? invalid_anim_id : id;
		}
		ASSERT(anim_idx[TurretAction::idle] != invalid_anim_id);
		
		string sound_prefix = parser("sound_prefix");
		for(auto tsound : all<TurretSoundId>()) {
			string sound_name = sound_prefix + s_sounds[tsound].name;
			sound_idx[tsound] = SoundId(sound_name.c_str(), s_sounds[tsound].index);
		}
	}
		
	bool TurretProto::canHide() const {
		return	anim_idx[TurretAction::hiding] != invalid_anim_id &&
				anim_idx[TurretAction::showing] != invalid_anim_id &&
				anim_idx[TurretAction::hidden] != invalid_anim_id;
	}

	Turret::Turret(Stream &sr)
	  :EntityImpl(sr) {
		m_hit_points = decodeInt(sr);
		sr.unpack(m_target_angle, m_action);
	}

	Turret::Turret(const XMLNode &node)
	  :EntityImpl(node), m_target_angle(dirAngle()) {
		m_hit_points = m_proto.hit_points;
		animate(TurretAction::idle);
	}

	Turret::Turret(const Proto &proto)
		:EntityImpl(proto), m_target_angle(dirAngle()) {
		m_hit_points = m_proto.hit_points;
		animate(TurretAction::idle);
	}

	XMLNode Turret::save(XMLNode &parent) const {
		XMLNode node = EntityImpl::save(parent);
		return node;
	}

	void Turret::save(Stream &sr) const {
		EntityImpl::save(sr);
		encodeInt(sr, m_hit_points);
		sr.pack(m_target_angle, m_action);
	}
		
	FlagsType Turret::flags() const {
		return Flags::turret | Flags::dynamic_entity | Flags::colliding;
	}
	
	const FBox Turret::boundingBox() const {
		int3 bbox_size = sprite().bboxSize();
		if(m_action == TurretAction::hidden)
			bbox_size.y = 1;
		//TODO: when dead&exploded some turrets are hidden

		return FBox(pos(), pos() + float3(bbox_size));
	}
		
	DeathId Turret::deathType(DamageType damage_type, float damage, const float3 &force_vec) const {
		float damage_rate = damage / float(m_proto.hit_points);

		if(damage_type == DamageType::electric)
			return DeathId::electrify;
		if(damage_type == DamageType::explosive && damage_rate > 0.2f)
			return DeathId::explode;
		return DeathId::normal;
	}

	void Turret::onImpact(DamageType damage_type, float damage, const float3 &force, EntityRef source) {
		float resistance = 0.7f;
		damage *= 1.0f - resistance;

		if(isDying()) {
			m_hit_points -= int(damage);
			return;
		}
			
		m_hit_points -= int(damage);
		if(m_hit_points <= 0.0f) {
			DeathId death_id = deathType(damage_type, damage, force);
			setOrder(new DieOrder(death_id), true);
			onKill(ref(), source);
		}
		else {
			//TODO: sound
		}
	
		ThinkingEntity::onImpact(damage_type, damage, force, source);
	}

	static const float feel_distance = 30.0;
	static const float max_distance = 300.0;

	bool Turret::canSee(EntityRef target_ref, bool simple_test) {
		const Entity *target = refEntity(target_ref);
		if(!target || isDead())
			return false;

		const FBox &box = boundingBox();
		const FBox &target_box = target->boundingBox();
		float dist = distance(box, target_box);
		float3 eye_pos = asXZY(box.center().xz(), box.y() + box.height() * 0.75f);

		if(dist > max_distance)
			return false;

		if(simple_test)
			return true;

		//TODO: move to ThinkingEntity?
		return world()->isVisible(eye_pos, target_ref, ref(), target->typeId() == EntityId::actor? 4 : 3);
	}

	bool Turret::setOrder(POrder &&order, bool force) {
		if(order && m_order)
			m_order->cancel();

		return ThinkingEntity::setOrder(std::move(order), force);
	}

	void Turret::think() {
		ThinkingEntity::think();
	}

	// sets direction
	void Turret::lookAt(const float3 &pos, bool at_once) { //TODO: rounding
		float3 center = boundingBox().center();
		float2 dir(pos.x - center.x, pos.z - center.z);
		float len = length(dir);
		if(len < fconstant::epsilon)
			return;

		dir = dir / len;
		m_target_angle = vectorToAngle(dir);
		if(at_once)
			setDirAngle(m_target_angle);
	}

	void Turret::nextFrame() {
		setDirAngle(blendAngles(dirAngle(), m_target_angle, fconstant::pi / 4.0f));
		ThinkingEntity::nextFrame();
	}
		
	bool Turret::isDying() const {
		return m_order && m_order->typeId() == OrderTypeId::die;
	}
		
	bool Turret::isDead() const {
		return m_order && m_order->typeId() == OrderTypeId::die &&
					static_cast<DieOrder*>(m_order.get())->m_is_dead;
	}
		
	bool Turret::animateDeath(DeathId death) {
		auto anim_id =
			death == DeathId::explode? TurretAction::death_explode :
			death == DeathId::electrify? TurretAction::death_electrify : TurretAction::death;
		int seq_id = m_proto.anim_idx[anim_id];
		if(seq_id == TurretProto::invalid_anim_id)
			return false;

		playSequence(seq_id);
		m_action = TurretAction::death;
		return true;
	}

	bool Turret::animate(TurretAction action) {
		int seq_id = m_proto.anim_idx[action];
		if(seq_id == TurretProto::invalid_anim_id)
			return false;

		m_action = action;
		playSequence(seq_id);
		return true;

		return true;
	}

	void Turret::fireProjectile(const FBox &target_box, const Weapon &weapon, float randomness) {
		if(isClient())
			return;

		Ray3F best_ray = *computeBestShootingRay(target_box, weapon).asRay();

		if(randomness > 0.0f) {
			float3 dir = perturbVector(best_ray.dir(), random(), random(), randomness);
			best_ray = Ray3F(best_ray.origin(), dir);
		}

#ifdef DEBUG_SHOOTING
		{
			m_aiming_lines.clear();
			m_aiming_lines.push_back(best_ray.origin());
			Intersection isect = trace(best_ray, {Flags::all | Flags::colliding, ref()});
			m_aiming_lines.push_back(best_ray.origin() + best_ray.dir() * isect.distance());
		}
#endif

		//TODO: spawned projectiles should be centered
		if( const ProjectileProto *proj_proto = weapon.projectileProto() )
			addNewEntity<Projectile>(best_ray.origin(), *proj_proto, actualDirAngle(), best_ray.dir(), ref(), weapon.proto().damage_mod);
	}
		
}
