/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/orders/die.h"
#include "game/actor.h"
#include "game/world.h"

namespace game {

	DieOrder::DieOrder(DeathTypeId::Type death_id) :m_death_id(death_id), m_is_dead(false) {
	}

	DieOrder::DieOrder(Stream &sr) :OrderImpl(sr) {
		sr >> m_death_id >> m_is_dead;
	}

	void DieOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		sr << m_death_id << m_is_dead;
	}

	bool Actor::handleOrder(DieOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		if(event == ActorEvent::init_order) {
			if(!animateDeath(order.m_death_id))
				animateDeath(DeathTypeId::normal);
		}
		if(event == ActorEvent::sound) {
			SoundId sound_id = m_actor.death_sounds[order.m_death_id];
			if(sound_id == -1)
				sound_id = m_actor.death_sounds[DeathTypeId::normal];
			world()->playSound(sound_id, pos());

			if(m_actor.is_alive)
				world()->playSound(m_actor.human_death_sounds[order.m_death_id], pos());
		}
		if(event == ActorEvent::anim_finished)
			order.m_is_dead = true;

		return true;
	}

}
