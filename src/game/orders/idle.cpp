/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */


#include "game/orders/idle.h"
#include "game/actor.h"
#include "game/turret.h"

namespace game {

	IdleOrder::IdleOrder() :m_fancy_anim_time(2.0f) {
	}

	IdleOrder::IdleOrder(Stream &sr) :OrderImpl(sr) {
		sr.unpack(m_fancy_anim_time);
	}

	void IdleOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		sr.pack(m_fancy_anim_time);
	}

	void IdleOrder::cancel() {
		Order::cancel();
	}
		
	bool Actor::handleOrder(IdleOrder &order, EntityEvent event, const EntityEventParams &params) {
		if(order.needCancel())
			return false;

		if(m_action == Action::idle)
			order.m_fancy_anim_time -= timeDelta();

		if(event == EntityEvent::anim_finished || event == EntityEvent::init_order) {
			if(order.m_fancy_anim_time < 0.0f) {
				order.m_fancy_anim_time = 1.5f;
				bool fidget = rand() % 10 == 0; //TODO: multiplayer mode?

				if(fidget) {
					if(!animate(Action::fidget))
						fidget = false;
				}

				if(!fidget)
					if(!animate(Action::breathe))
						animate(Action::idle);
			}
			else
				animate(Action::idle);
		}

		return true;
	}

	bool Turret::handleOrder(IdleOrder &order, EntityEvent event, const EntityEventParams &params) {
		bool is_canceling = order.needCancel();

		if(is_canceling && m_action == TurretAction::idle)
			return false;

		if(m_action == TurretAction::hidden && is_canceling) {
			replicateSound(m_proto.sound_idx[TurretSoundId::arming], pos());
			animate(TurretAction::showing);
		}

		if(m_action == TurretAction::idle && !is_canceling) {
			order.m_fancy_anim_time -= timeDelta();
			if(order.m_fancy_anim_time < 0.0f && m_proto.canHide()) {
				replicateSound(m_proto.sound_idx[TurretSoundId::unarming], pos());
				animate(TurretAction::hiding);
				order.m_fancy_anim_time = 5.0f;
			}
		}

		if(event == EntityEvent::init_order) {
			order.m_fancy_anim_time = 5.0f;
			animate(TurretAction::idle);
		}

		if(event == EntityEvent::anim_finished) {
			TurretAction next =
				m_action == TurretAction::hiding? TurretAction::hidden :
				m_action == TurretAction::hidden? TurretAction::hidden : TurretAction::idle;
			animate(next);
		}

		return true;
	}


}
