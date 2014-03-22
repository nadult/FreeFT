/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */


#include "game/orders/look_at.h"
#include "game/actor.h"

namespace game {

	LookAtOrder::LookAtOrder(float3 target) :m_target(target) { }

	LookAtOrder::LookAtOrder(Stream &sr) {
		sr >> m_target;
	}

	void LookAtOrder::save(Stream &sr) const {
		sr << m_target;
	}

	bool Actor::handleOrder(LookAtOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		if(length(order.m_target - pos()) >= 10.0f)
			lookAt(order.m_target);
		return false;
	}

}
