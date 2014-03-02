/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ORDERS_IDLE_H
#define GAME_ORDERS_IDLE_H

#include "game/orders.h"

namespace game {

	class IdleOrder: public OrderImpl<IdleOrder, OrderTypeId::idle,
			ActorEvent::init_order | ActorEvent::anim_finished> {
	public:
		IdleOrder();
		IdleOrder(Stream&);

		void save(Stream&) const;
		void cancel();

		// last breathe time, etc.
	};

}

#endif
