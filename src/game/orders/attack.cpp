/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/orders/attack.h"
#include "game/actor.h"
#include "game/world.h"

namespace game {

	AttackOrder::AttackOrder(AttackMode::Type mode, const int3 &target_pos)
		:m_mode(mode), m_target_pos(target_pos), m_burst_mode(0), m_burst_off(0, 0, 0) {
	}

	AttackOrder::AttackOrder(Stream &sr) :OrderImpl(sr) {
		sr >> m_mode >> m_target_pos;
		sr >> m_burst_mode >> m_burst_off;
	}

	void AttackOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		sr << m_mode << m_target_pos;
		sr << m_burst_mode << m_burst_off;
	}

	bool Actor::handleOrder(AttackOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		if(event == ActorEvent::init_order) {
			const Weapon &weapon = m_inventory.weapon();
			if(!m_proto.canUseWeapon(weapon.classId(), m_stance)) {
				printf("Cant use weapon: %s\n", weapon.proto().id.c_str());
				return false;
			}

			lookAt(order.m_target_pos);
	
			AttackMode::Type mode = order.m_mode;
			uint modes = weapon.attackModes();
			if(mode != AttackMode::undefined)	
				modes &= AttackMode::toFlags(mode);
			order.m_mode = mode = AttackModeFlags::getFirst(modes);
		
			if(mode == AttackMode::undefined)
				return false;
			int anim_id = m_proto.attackAnimId(mode, m_stance, weapon.classId());
			if(anim_id == -1)
				return false;
			playSequence(anim_id);
			m_action = Action::attack;
		}
		if(event == ActorEvent::fire) {
			const Weapon &weapon = m_inventory.weapon();
			AttackMode::Type mode = order.m_mode;

			if(mode != AttackMode::single && mode != AttackMode::burst)
				return true;

			if(mode == AttackMode::burst) {
				order.m_burst_mode = 1;
				order.m_burst_off = params.fire_offset;
			}
			else
				fireProjectile(params.fire_offset, order.m_target_pos, weapon, 0.0f);
		}
		if(event == ActorEvent::next_frame && order.m_burst_mode) {
			order.m_burst_mode++;
			fireProjectile(order.m_burst_off, (float3)order.m_target_pos, m_inventory.weapon(), 0.05f);
			if(order.m_burst_mode > 15)
				order.m_burst_mode = 0;
		}
		if(event == ActorEvent::anim_finished) {
			order.finish();
		}
		if(event == ActorEvent::sound) {
			const Weapon &weapon = m_inventory.weapon();
			//TODO: select firing mode in attack order
			SoundId sound_id = weapon.soundId(order.m_mode == AttackMode::burst? WeaponSoundType::fire_burst : WeaponSoundType::normal);

			if(order.m_mode == AttackMode::kick)
				sound_id = m_actor.kick_sound;

			world()->playSound(sound_id, pos());
		}

		return true;
	}

}
