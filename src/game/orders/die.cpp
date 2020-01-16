// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/orders/die.h"
#include "game/actor.h"
#include "game/turret.h"

namespace game {

	DieOrder::DieOrder(DeathId death_id) :m_death_id(death_id), m_is_dead(false) {
	}

	DieOrder::DieOrder(MemoryStream &sr) :OrderImpl(sr) {
		sr >> m_death_id >> m_is_dead;
	}

	void DieOrder::save(MemoryStream &sr) const {
		OrderImpl::save(sr);
		sr << m_death_id << m_is_dead;
	}

	bool Actor::handleOrder(DieOrder &order, EntityEvent event, const EntityEventParams &params) {
		bool play_sound = event == EntityEvent::sound;

		if(event == EntityEvent::init_order) {
			bool is_fallen = isOneOf(m_action, Action::fall_forward, Action::fall_back, Action::fallen_back, Action::fallen_forward);
			bool special_death = isOneOf(order.m_death_id, DeathId::explode, DeathId::fire, DeathId::electrify, DeathId::melt);

			if((is_fallen || m_stance == Stance::prone) && !special_death) {
				order.m_death_id = DeathId::normal;
				play_sound = true;
				if(m_stance == Stance::prone && !is_fallen)
					animate(Action::fall_forward);
				else
					order.m_is_dead = true;
			}
			else if(!animateDeath(order.m_death_id))
				animateDeath(DeathId::normal);
		}

		if(play_sound) {
			SoundId sound_id = m_actor.sounds[m_sound_variation].death[order.m_death_id];
			if(sound_id == -1)
				sound_id = m_actor.sounds[m_sound_variation].death[DeathId::normal];
			playSound(sound_id, pos());

			if(m_actor.is_alive)
				playSound(m_actor.human_death_sounds[order.m_death_id], pos());
		}
		if(event == EntityEvent::anim_finished)
			order.m_is_dead = true;

		return true;
	}

	bool Turret::handleOrder(DieOrder &order, EntityEvent event, const EntityEventParams &params) {
		bool play_sound = event == EntityEvent::sound;

		if(event == EntityEvent::init_order) {
			if(!animateDeath(order.m_death_id))
				animateDeath(DeathId::normal);
		}
		if(event == EntityEvent::sound) {
			auto sound_id = order.m_death_id == DeathId::explode? TurretSoundId::death_explode : TurretSoundId::death;
			playSound(m_proto.sound_idx[sound_id], pos());
		}

		if(event == EntityEvent::anim_finished)
			order.m_is_dead = true;

		return true;
	}

}
