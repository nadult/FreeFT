/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/orders/attack.h"
#include "game/actor.h"
#include "game/world.h"

namespace game {

	AttackOrder::AttackOrder(AttackMode::Type mode, EntityRef target)
		:m_mode(mode), m_target(target), m_target_pos(0, 0, 0), m_burst_mode(0), m_burst_off(0, 0, 0), m_is_kick_weapon(false), m_is_followup(false) {
	}

	AttackOrder::AttackOrder(AttackMode::Type mode, const float3 &target_pos)
		:m_mode(mode), m_target_pos(target_pos), m_burst_mode(0), m_burst_off(0, 0, 0), m_is_kick_weapon(false), m_is_followup(false) {
	}

	AttackOrder::AttackOrder(Stream &sr) :OrderImpl(sr) {
		sr >> m_mode >> m_target >> m_target_pos >> m_is_kick_weapon >> m_is_followup;
		sr >> m_burst_mode >> m_burst_off;
	}

	void AttackOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		sr << m_mode << m_target << m_target_pos << m_is_kick_weapon << m_is_followup;
		sr << m_burst_mode << m_burst_off;
	}

	bool Actor::handleOrder(AttackOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		if(event == ActorEvent::init_order) {
			Weapon weapon = m_inventory.weapon();
			if(!m_proto.canUseWeapon(weapon.classId(), m_stance)) {
				printf("Cant use weapon: %s\n", weapon.proto().id.c_str());
				return false;
			}

			//TODO: get target ptr
			if(order.m_target)
				order.m_target_pos = refBBox(order.m_target).center();
			lookAt(order.m_target_pos);
	
			AttackMode::Type mode = order.m_mode;

			uint modes = weapon.attackModes();
			if(mode != AttackMode::undefined)	
				modes &= AttackMode::toFlags(mode);
			mode = AttackModeFlags::getFirst(modes);
		
			if(mode == AttackMode::undefined) {
				if(weapon.canKick() && order.m_mode == AttackMode::kick && m_actor.kick_weapon.isValid()) {
					order.m_is_kick_weapon = true;
					weapon = Weapon(*m_actor.kick_weapon);
				}
				else
					return false;
			}
			else
				order.m_mode = mode;
			
			float max_range = weapon.range(order.m_mode);
			FBox target_box = order.m_target? refBBox(order.m_target) : FBox(order.m_target_pos, order.m_target_pos);
			float dist = distance(boundingBox(), target_box);

			if(dist > max_range * 0.9f && order.m_target && !order.m_is_followup) {
				order.m_is_followup = true;
				POrder track_order = new TrackOrder(order.m_target, max_range * 0.9f, true);
				track_order->setFollowup(order.clone());
				setOrder(std::move(track_order));
				return false;
			}

			int anim_id = m_proto.attackAnimId(order.m_mode, m_stance, weapon.classId());
			if(anim_id == -1)
				return false;

			playSequence(anim_id);
			m_action = Action::attack;
		}
			
		Weapon weapon = order.m_is_kick_weapon? Weapon(*m_actor.kick_weapon) : m_inventory.weapon();

		if(AttackMode::isRanged(order.m_mode)) {
			if(event == ActorEvent::fire) {
				AttackMode::Type mode = order.m_mode;
				if(order.m_target)
					order.m_target_pos = refBBox(order.m_target).center();

				if(mode != AttackMode::single && mode != AttackMode::burst)
					return true;

				if(mode == AttackMode::burst) {
					order.m_burst_mode = 1;
					order.m_burst_off = params.fire_offset;
				}
				else {
					fireProjectile(params.fire_offset, order.m_target_pos, weapon, 0.0f);
				}
			}
			if(event == ActorEvent::next_frame && order.m_burst_mode) {
				order.m_burst_mode++;
				fireProjectile(order.m_burst_off, (float3)order.m_target_pos, weapon, 0.05f);
				if(order.m_burst_mode > 15)
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
