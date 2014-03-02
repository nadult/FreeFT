/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/orders/change_stance.h"
#include "game/actor.h"

namespace game {

	ChangeStanceOrder::ChangeStanceOrder(Stance::Type target_stance)
		:m_target_stance(target_stance) {
	}

	ChangeStanceOrder::ChangeStanceOrder(Stream &sr)
		:OrderImpl(sr) {
		sr >> m_target_stance;
	}

	void ChangeStanceOrder::save(Stream &sr) const {
		Order::save(sr);
		sr << m_target_stance;
	}
	
	void Actor::handleOrder(ChangeStanceOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		Stance::Type target_stance = order.m_target_stance;

		if(event == ActorEvent::anim_finished) {
			m_stance = (Stance::Type)(target_stance > m_stance? m_stance + 1 : m_stance - 1);
			m_stance = min(max(m_stance, Stance::prone), Stance::standing);
		}

		if(m_stance == target_stance)
			order.finish();
		else
			animate(m_stance < target_stance? ActionId::stance_up : ActionId::stance_down);
	}

}

