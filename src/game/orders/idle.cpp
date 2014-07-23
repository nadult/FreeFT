/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */


#include "game/orders/idle.h"
#include "game/actor.h"

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
		finish();
	}
		
	bool Actor::handleOrder(IdleOrder &order, EntityEvent::Type event, const EntityEventParams &params) {
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

}
