/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/orders/change_stance.h"
#include "game/actor.h"

namespace game {

	ChangeStanceOrder::ChangeStanceOrder(Stance::Type target_stance)
		:m_target_stance(target_stance), m_stance_up(false) {
	}

	ChangeStanceOrder::ChangeStanceOrder(Stream &sr)
		:OrderImpl(sr) {
		sr >> m_target_stance >> m_stance_up;
	}

	void ChangeStanceOrder::save(Stream &sr) const {
		Order::save(sr);
		sr << m_target_stance << m_stance_up;
	}
	
	bool Actor::handleOrder(ChangeStanceOrder &order, EntityEvent::Type event, const EntityEventParams &params) {
		Stance::Type target_stance = order.m_target_stance;

		if(event == EntityEvent::anim_finished) {
			m_stance = (Stance::Type)(order.m_stance_up? m_stance + 1 : m_stance - 1);
			m_stance = min(max(m_stance, Stance::prone), Stance::stand);
		}
		if(event == EntityEvent::anim_finished || event == EntityEvent::init_order) {
			if(m_stance == target_stance)
				order.finish();
			else {
				order.m_stance_up = m_stance < target_stance;
				if(!animate(order.m_stance_up? Action::stance_up : Action::stance_down))
					return false;
			}
		}

		return true;
	}

}

