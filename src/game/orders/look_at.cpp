// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/orders/look_at.h"
#include "game/actor.h"
#include "game/turret.h"

namespace game {

LookAtOrder::LookAtOrder(float3 target) : m_target(target) {}

LookAtOrder::LookAtOrder(MemoryStream &sr) { sr >> m_target; }

void LookAtOrder::save(MemoryStream &sr) const { sr << m_target; }

bool Actor::handleOrder(LookAtOrder &order, EntityEvent event, const EntityEventParams &params) {
	float3 center = boundingBox().center();
	if(length(order.m_target - center) >= 1.0f)
		lookAt(order.m_target);
	return false;
}

bool Turret::handleOrder(LookAtOrder &order, EntityEvent event, const EntityEventParams &params) {
	float3 center = boundingBox().center();
	if(length(order.m_target - center) >= 1.0f)
		lookAt(order.m_target);
	return false;
}

}
