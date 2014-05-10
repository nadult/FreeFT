/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/orders/attack.h"
#include "game/actor.h"
#include "game/world.h"

namespace game {

	AttackOrder::AttackOrder(AttackMode::Type mode, EntityRef target)
		:m_mode(mode), m_target(target), m_target_pos(0, 0, 0), m_burst_mode(0), m_is_kick_weapon(false), m_is_followup(false) {
	}

	AttackOrder::AttackOrder(AttackMode::Type mode, const float3 &target_pos)
		:m_mode(mode), m_target_pos(target_pos), m_burst_mode(0), m_is_kick_weapon(false), m_is_followup(false) {
	}

	AttackOrder::AttackOrder(Stream &sr) :OrderImpl(sr) {
		u8 flags;
		sr.unpack(flags, m_mode);
		m_burst_mode = sr.decodeInt();
		m_is_kick_weapon = flags & 1;
		m_is_followup = flags & 2;
		if(flags & 4)
			sr >> m_target;
		else
			sr >> m_target_pos;
	}

	void AttackOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		u8 flags =	(m_is_kick_weapon? 1 : 0) |
					(m_is_followup? 2 : 0) |
					(m_target? 4 : 0);
		sr.pack(flags, m_mode);
		sr.encodeInt(m_burst_mode);
		if(m_target)
			sr << m_target;
		else
			sr << m_target_pos;
	}

	bool Actor::handleOrder(AttackOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		const Entity *target = refEntity(order.m_target);

		if(event == ActorEvent::init_order) {
			Weapon weapon = m_inventory.weapon();
			if(!m_proto.canUseWeapon(weapon.classId(), m_stance)) {
				printf("Cant use weapon: %s\n", weapon.proto().id.c_str());
				return failOrder();
			}

			AttackMode::Type mode = validateAttackMode(order.m_mode);
			if(mode == AttackMode::undefined)
				return failOrder();
			
			order.m_is_kick_weapon = mode == AttackMode::kick;
			order.m_mode = mode;
		}
			
		Weapon weapon = order.m_is_kick_weapon? Weapon(*m_actor.kick_weapon) : m_inventory.weapon();

		FBox target_box;
		if(target) {
			target_box = target->boundingBox();

			//TODO: this is a skill, should be randomized a bit
			if( const Actor *atarget = refEntity<Actor>(order.m_target) ) {
				for(int n = 0; n < 4; n++) {
					float dist = distance(boundingBox(), target_box);
					float tdist = weapon.estimateProjectileTime(dist);
					target_box = target->boundingBox() + atarget->estimateMove(tdist);
				}
			}
			order.m_target_pos = target_box.center();
		}
		else
			target_box = FBox(order.m_target_pos, order.m_target_pos);

		if(event == ActorEvent::init_order) {
			float max_range = weapon.range(order.m_mode);
			float dist = distance(boundingBox(), target_box);
			lookAt(target_box.center());

			if(dist > max_range * 0.9f && order.m_target) {
				if(order.m_is_followup)
					return failOrder();

				order.m_is_followup = true;
				POrder track_order = new TrackOrder(order.m_target, max_range * 0.9f, true);
				track_order->setFollowup(order.clone());
				setOrder(std::move(track_order));
				return false;
			}

			int anim_id = m_proto.attackAnimId(order.m_mode, m_stance, weapon.classId());
			if(anim_id == -1)
				return failOrder();

			playSequence(anim_id);
			m_action = Action::attack;
		}

		if(AttackMode::isRanged(order.m_mode)) {
			float inaccuracy = this->inaccuracy(weapon);

			if(event == ActorEvent::fire) {
				AttackMode::Type mode = order.m_mode;

				if(mode != AttackMode::single && mode != AttackMode::burst)
					return true;

				if(mode == AttackMode::burst) {
					order.m_burst_mode = 1;
				}
				else {
					fireProjectile(target_box, weapon, inaccuracy);
				}
			}
			if(event == ActorEvent::next_frame && order.m_burst_mode) {
				order.m_burst_mode++;
				fireProjectile(target_box, weapon, inaccuracy * (1.0f + 0.1f * order.m_burst_mode));
				if(order.m_burst_mode > weapon.proto().burst_ammo)
					order.m_burst_mode = 0;
			}

		}
		else {
			if(event == ActorEvent::hit) {
				makeImpact(order.m_target, weapon);
			}
		}

		if(event == ActorEvent::anim_finished) {
			order.finish();
		}
		if(event == ActorEvent::sound) {
			//TODO: select firing mode in attack order
			SoundId sound_id = weapon.soundId(order.m_mode == AttackMode::burst? WeaponSoundType::fire_burst : WeaponSoundType::normal);
			world()->playSound(sound_id, pos());
		}

		return true;
	}

}
