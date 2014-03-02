/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/orders/die.h"
#include "game/actor.h"

namespace game {

	DieOrder::DieOrder(DeathTypeId::Type death_id) {
	}

	DieOrder::DieOrder(Stream &sr) {
	}

	void DieOrder::save(Stream &sr) const {
	}

	void Actor::handleOrder(DieOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
	}

}
