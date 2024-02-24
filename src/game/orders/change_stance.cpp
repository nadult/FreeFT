// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/orders/change_stance.h"
#include "game/actor.h"

namespace game {

ChangeStanceOrder::ChangeStanceOrder(Stance target_stance)
	: m_target_stance(target_stance), m_stance_up(false) {}

ChangeStanceOrder::ChangeStanceOrder(MemoryStream &sr) : OrderImpl(sr) {
	sr >> m_target_stance >> m_stance_up;
}

void ChangeStanceOrder::save(MemoryStream &sr) const {
	Order::save(sr);
	sr << m_target_stance << m_stance_up;
}

bool Actor::handleOrder(ChangeStanceOrder &order, EntityEvent event,
						const EntityEventParams &params) {
	Stance target_stance = order.m_target_stance;

	if(event == EntityEvent::anim_finished) {
		auto stance = order.m_stance_up ? (int)m_stance + 1 : (int)m_stance - 1;
		m_stance = (Stance)clamp(stance, (int)Stance::prone, (int)Stance::stand);
	}
	if(event == EntityEvent::anim_finished || event == EntityEvent::init_order) {
		if(m_stance == target_stance)
			order.finish();
		else {
			order.m_stance_up = m_stance < target_stance;
			if(!animate(order.m_stance_up ? Action::stance_up : Action::stance_down))
				return false;
		}
	}

	return true;
}

}
