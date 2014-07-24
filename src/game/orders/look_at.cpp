/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */


#include "game/orders/look_at.h"
#include "game/actor.h"
#include "game/turret.h"

namespace game {

	LookAtOrder::LookAtOrder(float3 target) :m_target(target) { }

	LookAtOrder::LookAtOrder(Stream &sr) {
		sr >> m_target;
	}

	void LookAtOrder::save(Stream &sr) const {
		sr << m_target;
	}

	bool Actor::handleOrder(LookAtOrder &order, EntityEvent::Type event, const EntityEventParams &params) {
		float3 center = boundingBox().center();
		if(length(order.m_target - center) >= 1.0f)
			lookAt(order.m_target);
		return false;
	}

	bool Turret::handleOrder(LookAtOrder &order, EntityEvent::Type event, const EntityEventParams &params) {
		float3 center = boundingBox().center();
		if(length(order.m_target - center) >= 1.0f)
			lookAt(order.m_target);
		return false;
	}


}
