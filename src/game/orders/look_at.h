/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ORDERS_LOOK_AT_H
#define GAME_ORDERS_LOOK_AT_H

#include "game/orders.h"

namespace game {

	class LookAtOrder: public OrderImpl<LookAtOrder, OrderTypeId::look_at,
			ActorEvent::init_order> {
	public:
		LookAtOrder(float3);
		LookAtOrder(Stream&);
		void save(Stream&) const;

		float3 m_target;
	};

}

#endif
