/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */


#include "game/orders/idle.h"
#include "game/actor.h"

namespace game {

	IdleOrder::IdleOrder() {
	}

	IdleOrder::IdleOrder(Stream &sr) :OrderImpl(sr) {
	}

	void IdleOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
	}

	void IdleOrder::cancel() {
		Order::cancel();
		finish();
	}
		
	bool Actor::handleOrder(IdleOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		if(order.needCancel())
			return false;
		if(event == ActorEvent::anim_finished || event==ActorEvent::init_order) {
			animate(Action::idle);
		}

		return true;
	}

}
