// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/orders/attack.h"
#include "game/actor.h"
#include "game/orders/track.h"
#include "game/turret.h"

namespace game {

AttackOrder::AttackOrder(Maybe<AttackMode> mode, EntityRef target)
	: m_mode(mode), m_target(target), m_target_pos(0, 0, 0), m_burst_mode(0),
	  m_is_kick_weapon(false), m_is_followup(false) {}

AttackOrder::AttackOrder(Maybe<AttackMode> mode, const float3 &target_pos)
	: m_mode(mode), m_target_pos(target_pos), m_burst_mode(0), m_is_kick_weapon(false),
	  m_is_followup(false) {}

AttackOrder::AttackOrder(MemoryStream &sr) : OrderImpl(sr) {
	u8 flags;
	sr >> flags >> m_mode;
	m_burst_mode = decodeInt(sr);
	m_is_kick_weapon = flags & 1;
	m_is_followup = flags & 2;
	if(flags & 4)
		m_target.load(sr);
	else
		sr >> m_target_pos;
}

void AttackOrder::save(MemoryStream &sr) const {
	OrderImpl::save(sr);
	u8 flags = (m_is_kick_weapon ? 1 : 0) | (m_is_followup ? 2 : 0) | (m_target ? 4 : 0);
	sr << flags << m_mode;
	encodeInt(sr, m_burst_mode);
	if(m_target)
		m_target.save(sr);
	else
		sr << m_target_pos;
}

bool Actor::handleOrder(AttackOrder &order, EntityEvent event, const EntityEventParams &params) {
	const Entity *target = refEntity(order.m_target);

	if(event == EntityEvent::init_order) {
		Weapon weapon = m_inventory.weapon();
		if(!m_proto.canUseWeapon(weapon.classId(), m_stance)) {
			printf("Can't use weapon: %s\n", weapon.proto().id.c_str());
			return failOrder();
		}

		auto mode = validateAttackMode(order.m_mode);
		if(!mode)
			return failOrder();
		if(weapon.needAmmo() && !m_inventory.ammo().count) {
			replicateSound(weapon.soundId(WeaponSoundType::out_of_ammo), pos());
			return failOrder();
		}

		order.m_is_kick_weapon = mode == AttackMode::kick;
		order.m_mode = mode;
	}
	DASSERT(order.m_mode);
	auto mode = *order.m_mode;

	Weapon weapon = order.m_is_kick_weapon ? Weapon(*m_actor.kick_weapon) : m_inventory.weapon();

	FBox target_box;
	if(target) {
		target_box = target->boundingBox();

		//TODO: this is a skill, should be randomized a bit
		if(const Actor *atarget = refEntity<Actor>(order.m_target)) {
			for(int n = 0; n < 4; n++) {
				float dist = distance(boundingBox(), target_box);
				float tdist = weapon.estimateProjectileTime(dist);
				target_box = target->boundingBox() + atarget->estimateMove(tdist);
			}
		}
		order.m_target_pos = target_box.center();
	} else
		target_box = FBox(order.m_target_pos, order.m_target_pos);

	if(event == EntityEvent::init_order) {
		float max_range = weapon.range(mode);
		float dist = distance(boundingBox(), target_box);
		lookAt(target_box.center());

		if(dist > max_range * 0.9f && order.m_target) {
			if(order.m_is_followup)
				return failOrder();

			order.m_is_followup = true;
			POrder track_order(new TrackOrder(order.m_target, max_range * 0.9f, true));
			track_order->setFollowup(POrder(order.clone()));
			setOrder(std::move(track_order));
			return false;
		}

		int anim_id = m_proto.attackAnimId(mode, m_stance, weapon.classId());
		if(anim_id == -1)
			return failOrder();

		playSequence(anim_id);
		m_action = Action::attack;
	}

	if(isRanged(mode)) {
		float inaccuracy = this->inaccuracy(weapon);

		if(event == EntityEvent::fire) {
			if(mode != AttackMode::single && mode != AttackMode::burst)
				return true;

			if(mode == AttackMode::burst) {
				order.m_burst_mode = 1;
			} else {
				if(weapon.needAmmo() && !m_inventory.useAmmo(1))
					return false;
				fireProjectile(target_box, weapon, inaccuracy);
			}
		}
		if(event == EntityEvent::next_frame && order.m_burst_mode) {
			order.m_burst_mode++;
			if(weapon.needAmmo() && m_inventory.useAmmo(1))
				fireProjectile(target_box, weapon, inaccuracy * (1.0f + 0.1f * order.m_burst_mode));
			if(order.m_burst_mode > weapon.proto().burst_ammo)
				order.m_burst_mode = 0;
		}

	} else {
		if(event == EntityEvent::hit) {
			if(weapon.needAmmo() && !m_inventory.useAmmo(1))
				return false;
			makeImpact(order.m_target, weapon);
		}
	}

	if(event == EntityEvent::anim_finished) {
		order.finish();
	}
	if(event == EntityEvent::sound) {
		SoundId sound_id = weapon.soundId(mode == AttackMode::burst ? WeaponSoundType::fire_burst :
																	  WeaponSoundType::normal);
		replicateSound(sound_id, pos(), isRanged(mode) ? SoundType::shooting : SoundType::normal);
	}

	return true;
}

bool Turret::handleOrder(AttackOrder &order, EntityEvent event, const EntityEventParams &params) {
	if(event == EntityEvent::init_order) {
		if(!isOneOf(order.m_mode, AttackMode::single, AttackMode::burst))
			return failOrder();
	}

	const Entity *target = refEntity(order.m_target);
	const Weapon weapon(findProto("_turret_gun", ProtoId::weapon));
	DASSERT(order.m_mode);
	AttackMode mode = *order.m_mode;

	FBox target_box;
	if(target) {
		target_box = target->boundingBox();

		//TODO: this is a skill, should be randomized a bit
		if(const Actor *atarget = refEntity<Actor>(order.m_target)) {
			for(int n = 0; n < 4; n++) {
				float dist = distance(boundingBox(), target_box);
				float tdist = weapon.estimateProjectileTime(dist);
				target_box = target->boundingBox() + atarget->estimateMove(tdist);
			}
		}
		order.m_target_pos = target_box.center();
	} else
		target_box = FBox(order.m_target_pos, order.m_target_pos);

	if(event == EntityEvent::init_order) {
		float max_range = weapon.range(mode);
		float dist = distance(boundingBox(), target_box);
		lookAt(target_box.center());

		if(dist > max_range)
			return failOrder();

		if(!animate(mode == AttackMode::single ? TurretAction::attack_single :
												 TurretAction::attack_burst))
			return failOrder();
	}

	{
		float inaccuracy = this->inaccuracy(weapon);
		//TODO: using ammo

		if(event == EntityEvent::fire) {

			if(mode != AttackMode::single && mode != AttackMode::burst)
				return true;

			if(mode == AttackMode::burst) {
				order.m_burst_mode = 1;
			} else {
				//	if(weapon.needAmmo() && !m_inventory.useAmmo(1))
				//		return false;
				fireProjectile(target_box, weapon, inaccuracy);
			}
		}
		if(event == EntityEvent::next_frame && order.m_burst_mode) {
			order.m_burst_mode++;
			//	if(weapon.needAmmo() && m_inventory.useAmmo(1))
			fireProjectile(target_box, weapon, inaccuracy * (1.0f + 0.1f * order.m_burst_mode));
			if(order.m_burst_mode > weapon.proto().burst_ammo)
				order.m_burst_mode = 0;
		}
	}

	if(event == EntityEvent::anim_finished) {
		order.finish();
	}
	if(event == EntityEvent::sound) {
		SoundId sound_id =
			m_proto.sound_idx[mode == AttackMode::single ? TurretSoundId::attack_single :
														   TurretSoundId::attack_burst];
		replicateSound(sound_id, pos(), SoundType::shooting);
	}

	return true;
}

}
